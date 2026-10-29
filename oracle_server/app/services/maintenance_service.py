# Файл: oracle_server/app/services/maintenance_service.py
import os
import sqlite3
import datetime
from typing import Dict, Any, Optional
import httpx

from app.core.config import settings
from app.core.logger import logger
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.spotify_service import spotify_service
from app.services.google_calendar_service import google_calendar_service


class MaintenanceService:
    """
    Фаза 6: Автоматичне обслуговування, бекапи та діагностичний дашборд здоров'я.
    """

    __slots__ = ("_last_backup_week",)

    def __init__(self) -> None:
        self._last_backup_week: int = -1

    def create_database_backup(self) -> str:
        """Створює консистентний SQLite бекап без зупинки запису (Zero-Downtime VACUUM INTO)."""
        backup_dir = os.path.join(settings.STORAGE_DIR, "backups")
        os.makedirs(backup_dir, exist_ok=True)

        stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        target_path = os.path.join(backup_dir, f"magicbox_backup_{stamp}.db")

        try:
            # Використовуємо окреме тимчасове з'єднання для безпечного VACUUM INTO
            conn = sqlite3.connect(settings.DB_PATH, timeout=30.0)
            conn.execute(f"VACUUM INTO '{target_path}';")
            conn.close()
            logger.info("Database backup created successfully: %s", target_path)

            # Ротація бекапів: залишаємо лише останні 4 архіви
            files = sorted(
                [os.path.join(backup_dir, f) for f in os.listdir(backup_dir) if f.endswith(".db")],
                key=os.path.getmtime,
            )
            while len(files) > 4:
                old = files.pop(0)
                try:
                    os.remove(old)
                except OSError:
                    pass

            return target_path
        except Exception as e:
            logger.error("Failed to create database backup: %s", e)
            return ""

    async def send_weekly_backup_to_admin(self) -> bool:
        """Надсилає файл бекапу бази даних в особистий чат Telegram адміністратора (Ярослава)."""
        admin_id = state_service.admin_id
        if not settings.TG_BOT_TOKEN or not admin_id:
            return False

        b_path = self.create_database_backup()
        if not b_path or not os.path.exists(b_path):
            return False

        fname = os.path.basename(b_path)
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/sendDocument"

        try:
            async with httpx.AsyncClient(timeout=45.0) as client:
                with open(b_path, "rb") as f:
                    files = {"document": (fname, f, "application/octet-stream")}
                    data = {
                        "chat_id": admin_id,
                        "caption": f"📦 <b>Щотижневий автоматичний бекап MagicBox</b>\nДата: <code>{datetime.datetime.now().strftime('%d.%m.%Y %H:%M')}</code>",
                        "parse_mode": "HTML",
                    }
                    res = await client.post(url, data=data, files=files)
                    if res.status_code == 200:
                        logger.info("Weekly backup sent to admin Telegram successfully")
                        return True
                    logger.warning("Telegram sendDocument failed (%d): %s", res.status_code, res.text)
                    return False
        except Exception as e:
            logger.error("Failed sending backup to Telegram: %s", e)
            return False

    def get_health_report(self) -> Dict[str, Any]:
        """Формує повний звіт про здоров'я та стан системи для віддаленого моніторингу."""
        t = state_service.latest_telemetry
        outages = db_service.get_recent_outages(limit=10)

        # Розрахунок аптайму та відключень
        total_outage_sec = sum(o.get("duration_sec", 0.0) for o in outages)

        return {
            "server": {
                "version": settings.VERSION,
                "online": True,
                "app_env": settings.APP_ENV,
                "resources": memory_guard.get_telemetry(),
            },
            "box": {
                "is_online": state_service.is_online(),
                "seconds_since_last_ping": state_service.seconds_since_seen(),
                "firmware_version": t.get("fw", "v4.5"),
                "wifi_rssi_dbm": t.get("rssi", -60),
                "room_temperature": t.get("t", 0.0),
                "room_humidity": t.get("h", 0.0),
                "room_pressure_mm": t.get("p", 750.0),
                "psram_free_bytes": t.get("psram_free", 0),
                "heap_free_bytes": t.get("heap", 0),
                "light_mode": t.get("m_name", ""),
                "volume": t.get("vol", 12),
                "is_muted": bool(t.get("muted", 0)),
            },
            "integrations": {
                "spotify_linked": spotify_service.is_linked(),
                "google_calendar_linked": google_calendar_service.is_linked(),
                "gemini_api_configured": bool(settings.GEMINI_API_KEY),
                "tts_provider": db_service.get_setting("tts_provider", settings.TTS_PROVIDER),
            },
            "power_analytics": {
                "in_blackout": blackout_service.in_blackout,
                "recent_outages_count": len(outages),
                "total_outage_time_min": int(total_outage_sec // 60),
            },
        }


    def calculate_and_save_daily_rollup(self, target_date: Optional[datetime.date] = None) -> None:
        """Розраховує добові підсумки (Rollup) для клімату, блекаутів та ребутів."""
        if target_date is None:
            target_date = datetime.date.today()
        date_str = target_date.strftime("%Y-%m-%d")

        start_dt = datetime.datetime.combine(target_date, datetime.time.min)
        end_dt = datetime.datetime.combine(target_date, datetime.time.max)
        start_ts = int(start_dt.timestamp())
        end_ts = int(end_dt.timestamp())

        try:
            conn = db_service._get_conn()
            # 1. Клімат за добу
            clim = conn.execute(
                "SELECT MIN(temp) as min_t, MAX(temp) as max_t, AVG(temp) as avg_t, "
                "AVG(hum) as avg_h, AVG(pres) as avg_p FROM climate_history "
                "WHERE ts >= ? AND ts <= ?",
                (start_ts, end_ts),
            ).fetchone()

            # 2. Відключення світла за добу
            outg = conn.execute(
                "SELECT COUNT(*) as cnt, COALESCE(SUM(duration_sec), 0) as sec "
                "FROM power_outages WHERE disconnected_at >= ? AND disconnected_at <= ?",
                (start_ts, end_ts),
            ).fetchone()

            # 3. Ребути плати за добу
            reboots = conn.execute(
                "SELECT COUNT(*) as cnt FROM event_log WHERE category = 'boot' AND ts >= ? AND ts <= ?",
                (float(start_ts), float(end_ts)),
            ).fetchone()

            rollup_data = {
                "temp_min": round(clim["min_t"], 1) if clim and clim["min_t"] is not None else None,
                "temp_max": round(clim["max_t"], 1) if clim and clim["max_t"] is not None else None,
                "temp_avg": round(clim["avg_t"], 1) if clim and clim["avg_t"] is not None else None,
                "hum_avg": round(clim["avg_h"], 0) if clim and clim["avg_h"] is not None else None,
                "pres_avg": round(clim["avg_p"], 1) if clim and clim["avg_p"] is not None else None,
                "outage_count": outg["cnt"] if outg else 0,
                "outage_sec": round(outg["sec"], 1) if outg else 0.0,
                "reboots_count": reboots["cnt"] if reboots else 0,
            }
            db_service.save_daily_rollup(date_str, rollup_data)
        except Exception as e:
            logger.warning("calculate_and_save_daily_rollup error: %s", e)


maintenance_service = MaintenanceService()
