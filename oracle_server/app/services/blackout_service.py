# Файл: oracle_server/app/services/blackout_service.py
import asyncio
import time
from typing import Dict, Any, Optional
from app.core.config import settings
from app.core.http_client import http_pool
from app.core.events import realtime_hub
from app.services.database_service import db_service
from app.services.state_service import state_service


class BlackoutService:
    """
    Подієвий монітор живлення з 0.00% CPU у простої:
    Використовує одноразовий таймер дедлайну замість нескінченного циклу опитування.
    """

    __slots__ = ("in_blackout", "blackout_started_at", "_deadline_handle")

    def __init__(self) -> None:
        self.in_blackout: bool = False
        self.blackout_started_at: float = 0.0
        self._deadline_handle: Optional[asyncio.TimerHandle] = None

    async def send_tg_alert(self, text: str, chat_id: str = "") -> None:
        target = chat_id or state_service.admin_id
        if not settings.TG_BOT_TOKEN or not target:
            return
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/sendMessage"
        try:
            await http_pool.get().post(url, json={"chat_id": target, "text": text}, timeout=6.0)
        except Exception:
            pass

    def _arm_deadline_timer(self) -> None:
        """Переставляє одноразовий таймер відключення на BLACKOUT_TIMEOUT_SEC вперед (0% CPU)."""
        try:
            loop = asyncio.get_running_loop()
            if self._deadline_handle is not None:
                self._deadline_handle.cancel()
            self._deadline_handle = loop.call_later(
                settings.BLACKOUT_TIMEOUT_SEC,
                lambda: asyncio.create_task(self._on_deadline_expired()),
            )
        except RuntimeError:
            pass

    async def _on_deadline_expired(self) -> None:
        """Викликається рівно 1 раз, коли від шкатулки немає сигналу понад BLACKOUT_TIMEOUT_SEC."""
        if self.in_blackout or state_service.last_seen <= 0:
            return
        age = time.time() - state_service.last_seen
        if age < settings.BLACKOUT_TIMEOUT_SEC - 0.5:
            self._arm_deadline_timer()
            return

        self.in_blackout = True
        self.blackout_started_at = state_service.last_seen
        last_t = state_service.latest_telemetry.get("t", "--")
        last_h = state_service.latest_telemetry.get("h", "--")
        await self.send_tg_alert(
            f"🔌 Зник зв'язок зі шкатулкою MagicBox ({int(age)} с).\n"
            f"Ймовірно, вимкнули світло. Усі нові повідомлення, гіфки та реакції збережено в чергу!\n"
            f"🌡 Останні показники в кімнаті: {last_t}°C, {last_h}%."
        )
        await realtime_hub.broadcast_to_dashboards({"event": "blackout", "in_blackout": True})

    async def on_box_ping(self, prev_seen: float, ack: bool = False, ping_type: int = 0) -> None:
        now = time.time()
        self._arm_deadline_timer()

        if self.in_blackout and self.blackout_started_at > 0:
            duration = round(now - self.blackout_started_at, 1)
            db_service.record_outage(self.blackout_started_at, now, duration)
            self.in_blackout = False
            mins = max(1, int(duration // 60))
            await self.send_tg_alert(
                f"⚡ Світло та домашній Wi-Fi увімкнули! Шкатулка онлайн.\n"
                f"⏱ Тривалість відключення: ~{mins} хв ({int(duration)} с)."
            )

        if ack:
            await self.send_tg_alert("✅ Настя торкнулася кришки шкатулки — твоє повідомлення прочитано! 🔥")

        if ping_type > 0:
            ping_names = {
                2: "🔥 Настя натиснула кнопку на шкатулці і кинула тобі ВОГНИК!",
                3: "⚡ Настя натиснула кнопку на шкатулці і бахнула БЛИСКАВКОЮ!",
                4: "🐱 Настя натиснула кнопку на шкатулці: МЕМНИЙ КУСЬ (=^._.^=)!",
                5: "👽 Настя натиснула кнопку на шкатулці: «Пороблено! UFO Beam»!",
                6: "👑 Настя передає тобі КОРОЛІВСЬКИЙ ПРИВІТ зі шкатулки!",
            }
            await self.send_tg_alert(ping_names.get(ping_type, "✨ Сигнал зі шкатулки!"))

        await realtime_hub.broadcast_to_dashboards({
            "event": "telemetry",
            "online": True,
            "telemetry": state_service.latest_telemetry,
        })

    def shutdown(self) -> None:
        if self._deadline_handle is not None:
            self._deadline_handle.cancel()
            self._deadline_handle = None

    def get_status(self) -> Dict[str, Any]:
        age = state_service.seconds_since_seen()
        return {
            "in_blackout": self.in_blackout,
            "offline_seconds": age if self.in_blackout else 0.0,
            "recent_outages": db_service.get_recent_outages(limit=30),
        }


blackout_service = BlackoutService()