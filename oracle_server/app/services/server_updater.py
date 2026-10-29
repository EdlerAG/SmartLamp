import os
import asyncio
import subprocess
from typing import Dict, Any

from app.core.config import settings
from app.core.logger import logger
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.maintenance_service import maintenance_service


class ServerUpdater:
    """
    Автономний модуль самооновлення сервера MagicBox Sovereign Cloud (OTA для Сервера v5.0):
    - Порівнює локальний коміт із GitHub origin/main.
    - Перед оновленням створює консистентний бекап SQLite WAL.
    - Виконує git fetch && git reset --hard origin/main без втрати .env та storage/.
    - Оновлює pip-пакети за потреби.
    - Сповіщає адміністратора в Telegram та ініціює м'який рестарт systemd.
    """

    __slots__ = ("_is_updating", "_repo_dir")

    def __init__(self) -> None:
        self._is_updating: bool = False
        # Визначаємо кореневу папку репозиторію
        self._repo_dir: str = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))

    def _run_git_cmd(self, args: list[str], timeout: float = 30.0) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["git"] + args,
            cwd=self._repo_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
        )

    def check_for_updates(self) -> Dict[str, Any]:
        """Перевіряє наявність нових комітів у віддаленому репозиторії."""
        try:
            # 1. Отримуємо свіжу інформацію з remote
            fetch_res = self._run_git_cmd(["fetch", "origin", "main"], timeout=15.0)
            if fetch_res.returncode != 0:
                logger.warning("Git fetch warning: %s", fetch_res.stderr)

            # 2. Поточний локальний коміт
            cur_hash = self._run_git_cmd(["rev-parse", "HEAD"]).stdout.strip()
            cur_msg = self._run_git_cmd(["log", "-1", "--pretty=%s"]).stdout.strip()

            # 3. Віддалений коміт на GitHub
            remote_hash = self._run_git_cmd(["rev-parse", "origin/main"]).stdout.strip()
            remote_msg = self._run_git_cmd(["log", "-1", "--pretty=%s", "origin/main"]).stdout.strip()

            # 4. Скільки комітів попереду
            behind_cnt = 0
            if cur_hash and remote_hash and cur_hash != remote_hash:
                cnt_res = self._run_git_cmd(["rev-list", "--count", "HEAD..origin/main"])
                try:
                    behind_cnt = int(cnt_res.stdout.strip())
                except ValueError:
                    behind_cnt = 1

            return {
                "ok": True,
                "current_commit": cur_hash[:8],
                "current_message": cur_msg,
                "remote_commit": remote_hash[:8],
                "remote_message": remote_msg,
                "update_available": (cur_hash != remote_hash and behind_cnt > 0),
                "commits_behind": behind_cnt,
            }
        except Exception as e:
            logger.error("check_for_updates error: %s", e)
            return {"ok": False, "error": str(e), "update_available": False}

    async def _async_trigger_restart(self) -> None:
        """Відкладений перезапуск служби systemd через 1.5 секунди."""
        await asyncio.sleep(1.5)
        # Спочатку пробуємо smartlamp.service, потім magicbox.service
        for s_name in ("smartlamp", "magicbox"):
            try:
                subprocess.Popen(["sudo", "systemctl", "restart", s_name])
                logger.info("Triggered systemctl restart for %s", s_name)
                break
            except Exception as e:
                logger.warning("Failed systemctl restart for %s: %s", s_name, e)

    async def perform_server_update(self) -> Dict[str, Any]:
        """Виконує повний цикл самооновлення сервера."""
        if self._is_updating:
            return {"ok": False, "error": "Оновлення вже виконується"}

        self._is_updating = True
        try:
            # 1. Безпечний бекап бази даних перед оновленням
            b_path = maintenance_service.create_database_backup()

            # 2. Стягуємо свіжий код з GitHub
            fetch_res = self._run_git_cmd(["fetch", "origin", "main"], timeout=30.0)
            if fetch_res.returncode != 0:
                self._is_updating = False
                return {"ok": False, "error": f"Git fetch failed: {fetch_res.stderr}"}

            # 3. Жорсткий reset до origin/main (storage/ та .env захищені .gitignore)
            reset_res = self._run_git_cmd(["reset", "--hard", "origin/main"], timeout=30.0)
            if reset_res.returncode != 0:
                self._is_updating = False
                return {"ok": False, "error": f"Git reset failed: {reset_res.stderr}"}

            new_hash = self._run_git_cmd(["rev-parse", "HEAD"]).stdout.strip()[:8]
            new_msg = self._run_git_cmd(["log", "-1", "--pretty=%s"]).stdout.strip()

            # 4. Перевірка та оновлення pip-пакетів (якщо активний Linux venv)
            venv_pip = os.path.join(self._repo_dir, "oracle_server", ".venv", "bin", "pip")
            req_file = os.path.join(self._repo_dir, "oracle_server", "requirements.txt")
            if os.path.exists(venv_pip) and os.path.exists(req_file):
                try:
                    subprocess.run(
                        [venv_pip, "install", "--no-cache-dir", "-q", "-r", req_file],
                        timeout=90.0,
                    )
                except Exception as pe:
                    logger.warning("pip requirements install warning: %s", pe)

            # 5. Фіксація в журналі подій
            db_service.log_event("admin", "server_ota", "update", {
                "commit": new_hash,
                "message": new_msg,
                "backup": os.path.basename(b_path) if b_path else "none",
            })

            # 6. Сповіщення в Telegram адміна
            if settings.TG_BOT_TOKEN and state_service.admin_id:
                from app.services.bot_service import bot_service
                notice = (
                    f"🔄 <b>Сервер SmartLamp успішно оновлено!</b>\n"
                    f"━━━━━━━━━━━━━━━━━━\n"
                    f"• Коміт: <code>{new_hash}</code>\n"
                    f"• Опис: <i>{new_msg}</i>\n"
                    f"• Резервна копія: <code>{os.path.basename(b_path)}</code>\n\n"
                    f"⚡ Перезапуск служби у фоні (2-3 сек)..."
                )
                await bot_service._api("sendMessage", {
                    "chat_id": state_service.admin_id,
                    "text": notice,
                    "parse_mode": "HTML",
                })

            # 7. Плануємо перезапуск служби
            asyncio.create_task(self._async_trigger_restart())

            self._is_updating = False
            return {
                "ok": True,
                "message": f"Оновлено до {new_hash}: {new_msg}. Перезапуск...",
                "commit": new_hash,
            }
        except Exception as e:
            self._is_updating = False
            logger.error("perform_server_update error: %s", e)
            return {"ok": False, "error": str(e)}


server_updater = ServerUpdater()
