"""
Файл: oracle_server/app/services/reminder_service.py
Сервіс делікатних фонових нагадувань про незавершені опціональні можливості (Spotify, Google Calendar).
Виконується згідно з узгодженим планом:
- Щоденна перевірка о 11:00 за київським часом.
- Тільки якщо лампа онлайн і не діють тихі години.
- Лише для власниці (owner).
- Не більше 4 нагадувань на фічу з інтервалом не менше 24 годин.
- Кнопки: [Підключити], [Пізніше], [Не нагадувати].
"""

import asyncio
from datetime import datetime
import zoneinfo
from typing import Optional

from app.core.logger import logger
from app.core.config import settings
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.routine_service import routine_service
from app.services.setup_service import setup_service
from app.services.spotify_service import spotify_service
from app.services.google_calendar_service import google_calendar_service


class ReminderService:
    __slots__ = ("_task", "_running")

    def __init__(self) -> None:
        self._task: Optional[asyncio.Task] = None
        self._running: bool = False

    def start(self) -> None:
        if self._running:
            return
        self._running = True
        self._task = asyncio.create_task(self._worker_loop(), name="reminder_service_loop")
        logger.info("⏰ ReminderService background worker started")

    def stop(self) -> None:
        self._running = False
        if self._task and not self._task.done():
            self._task.cancel()
        logger.info("⏰ ReminderService stopped")

    async def _worker_loop(self) -> None:
        # Затримка при старті сервера перед першою перевіркою
        await asyncio.sleep(30.0)
        while self._running:
            try:
                await self.check_and_send_reminders()
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error("Error in ReminderService loop: %s", e)

            # Перевірка кожні 15 хвилин
            await asyncio.sleep(900.0)

    async def check_and_send_reminders(self, force_check: bool = False) -> bool:
        """
        Перевіряє передумови та надсилає одне делікатне нагадування власниці, якщо настав час.
        """
        owner_id = db_service.get_owner_chat_id()
        if not owner_id:
            return False

        # Перевірка онлайн-статусу лампи
        if not state_service.is_online() and not force_check:
            return False

        # Перевірка тихих годин
        if routine_service.is_quiet_hours_now() and not force_check:
            return False

        # Перевірка часу: 11:00-12:00 за локальним часом
        today_str = ""
        if not force_check:
            try:
                tz = zoneinfo.ZoneInfo(db_service.get_setting("timezone", "Europe/Kyiv"))
            except Exception:
                tz = zoneinfo.ZoneInfo("Europe/Kyiv")

            now_local = datetime.now(tz)
            if now_local.hour != 11:
                return False

            today_str = now_local.strftime("%Y-%m-%d")
            last_run_day = str(db_service.get_setting("reminder_last_check_day", ""))
            if last_run_day == today_str:
                return False

        # Перевіряємо незавершені кроки (не більше одного нагадування на день)
        # 1. Spotify
        if not spotify_service.is_linked() and setup_service.is_reminder_due("spotify"):
            sent = await self._send_spotify_reminder(owner_id)
            if sent:
                setup_service.mark_reminder_sent("spotify")
                if today_str:
                    db_service.set_setting("reminder_last_check_day", today_str)
                return True

        # 2. Google Calendar
        if not google_calendar_service.is_linked() and setup_service.is_reminder_due("calendar"):
            sent = await self._send_calendar_reminder(owner_id)
            if sent:
                setup_service.mark_reminder_sent("calendar")
                if today_str:
                    db_service.set_setting("reminder_last_check_day", today_str)
                return True

        return False

    async def _send_spotify_reminder(self, chat_id: str) -> bool:
        from app.services.bot_service import bot_service
        text = (
            "🎵 <b>Музика для твоєї шкатулки</b>\n\n"
            "Привіт! Ти ще не підключила <b>Spotify</b> до своєї MagicBox.\n\n"
            "Зі Spotify лампа показує вінілову обкладинку на екрані та підлаштовує "
            "фонове Ambi-освітлення під колір пісні, що грає.\n\n"
            "Хочеш підключити зараз?"
        )
        auth_url = spotify_service.get_authorize_url(settings.PUBLIC_SERVER_URL)
        buttons = []
        if auth_url:
            buttons.append([{"text": "🎵 Підключити Spotify", "url": auth_url}])
        else:
            buttons.append([{"text": "🎵 Підключити Spotify", "callback_data": "setup_next:spotify"}])

        buttons.append([
            {"text": "⏳ Пізніше", "callback_data": "reminder_later:spotify"},
            {"text": "❌ Більше не нагадувати", "callback_data": "reminder_dismiss:spotify"}
        ])

        res = await bot_service._api("sendMessage", {
            "chat_id": chat_id,
            "text": text,
            "parse_mode": "HTML",
            "reply_markup": {"inline_keyboard": buttons}
        })
        return bool(res and res.get("ok"))

    async def _send_calendar_reminder(self, chat_id: str) -> bool:
        from app.services.bot_service import bot_service
        text = (
            "📅 <b>Твій розклад та важливі події</b>\n\n"
            "Привіт! Нагадую, що ти можеш підключити свій <b>Google Календар</b>.\n\n"
            "Шкатулка показуватиме найближчу пару чи зустріч на екрані Smart Hub "
            "і м'яко нагадуватиме за 15 хвилин до початку.\n\n"
            "<i>(Твої події є суто приватними і доступні лише тобі)</i>"
        )
        auth_url = google_calendar_service.get_authorize_url(settings.PUBLIC_SERVER_URL)
        buttons = []
        if auth_url:
            buttons.append([{"text": "📅 Підключити Календар", "url": auth_url}])
        else:
            buttons.append([{"text": "📅 Підключити Календар", "callback_data": "setup_next:calendar"}])

        buttons.append([
            {"text": "⏳ Пізніше", "callback_data": "reminder_later:calendar"},
            {"text": "❌ Більше не нагадувати", "callback_data": "reminder_dismiss:calendar"}
        ])

        res = await bot_service._api("sendMessage", {
            "chat_id": chat_id,
            "text": text,
            "parse_mode": "HTML",
            "reply_markup": {"inline_keyboard": buttons}
        })
        return bool(res and res.get("ok"))


reminder_service = ReminderService()
