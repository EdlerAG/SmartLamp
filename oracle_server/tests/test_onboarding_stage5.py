import unittest
from unittest.mock import AsyncMock, patch
import time

from app.core.config import settings
from app.services.database_service import db_service
from app.services.setup_service import setup_service
from app.services.state_service import state_service
from app.services.routine_service import routine_service
from app.services.reminder_service import reminder_service
from app.services.bot_service import bot_service
from app.services.spotify_service import spotify_service
from app.services.google_calendar_service import google_calendar_service


class TestOnboardingStage5(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        # Очищуємо та ініціалізуємо тестовий стан
        db_service.set_setting("owner_chat_id", "777000123")
        db_service.set_setting("theme_mood", "chornobyl")
        db_service.set_setting("reminder_last_check_day", "")
        db_service.set_setting("reminder_count_spotify", 0)
        db_service.set_setting("reminder_ts_spotify", 0.0)
        db_service.set_setting("reminder_count_calendar", 0)
        db_service.set_setting("reminder_ts_calendar", 0.0)

        # Очищуємо токени Spotify та Календаря
        spotify_service.clear_tokens()
        google_calendar_service.clear_tokens()
        setup_service.advance_step("spotify", "pending")
        setup_service.advance_step("calendar", "pending")

        # Встановлюємо лампу онлайн
        state_service.update_state_from_box({"m": 0, "fw": "v5.5"})

        # Мокаємо bot_service._api
        self.mock_api = AsyncMock(return_value={"ok": True, "result": {"message_id": 999}})
        bot_service._api = self.mock_api

    async def test_01_reminder_offline_guard(self):
        """Нагадування не надсилається, якщо лампа офлайн."""
        # Емулюємо офлайн (старий timestamp)
        state_service._last_seen_ms = time.time() * 1000 - 60000.0
        self.assertFalse(state_service.is_online())

        sent = await reminder_service.check_and_send_reminders(force_check=False)
        self.assertFalse(sent)
        self.mock_api.assert_not_called()

    async def test_02_reminder_quiet_hours_guard(self):
        """Нагадування не надсилається під час тихих годин."""
        # Оновлюємо активність лампи, щоб була онлайн
        state_service.update_state_from_box({"m": 0})
        self.assertTrue(state_service.is_online())

        with patch.object(routine_service, "is_quiet_hours_now", return_value=True):
            sent = await reminder_service.check_and_send_reminders(force_check=False)
            self.assertFalse(sent)
            self.mock_api.assert_not_called()

    async def test_03_send_spotify_reminder_and_record_ts(self):
        """Відправка нагадування про Spotify та фіксація timestamp і лічильника."""
        state_service.update_state_from_box({"m": 0})
        self.assertTrue(state_service.is_online())

        with patch.object(routine_service, "is_quiet_hours_now", return_value=False):
            sent = await reminder_service.check_and_send_reminders(force_check=True)
            self.assertTrue(sent)
            self.mock_api.assert_called()

            # Перевіряємо, що збережено лічильник і timestamp
            count = int(db_service.get_setting("reminder_count_spotify", 0))
            self.assertEqual(count, 1)

            # Друга спроба одразу має бути відхилена через інтервал 24h
            sent2 = await reminder_service.check_and_send_reminders(force_check=True)
            # Тепер черга календаря!
            self.assertTrue(sent2)
            cal_count = int(db_service.get_setting("reminder_count_calendar", 0))
            self.assertEqual(cal_count, 1)

            # Третя спроба — обидва вже мають timestamp менше 24h тому
            sent3 = await reminder_service.check_and_send_reminders(force_check=True)
            self.assertFalse(sent3)

    async def test_04_bot_reminder_callbacks(self):
        """Обробка кнопок [Пізніше] та [Більше не нагадувати]."""
        # 1. reminder_later
        cb_later = {
            "id": "cb1",
            "from": {"id": 777000123},
            "message": {"message_id": 100, "chat": {"id": 777000123}},
            "data": "reminder_later:spotify",
        }
        await bot_service.handle_update({"update_id": 1, "callback_query": cb_later})
        ts = float(db_service.get_setting("reminder_ts_spotify", 0.0))
        self.assertGreater(ts, 0.0)

        # 2. reminder_dismiss
        cb_dismiss = {
            "id": "cb2",
            "from": {"id": 777000123},
            "message": {"message_id": 101, "chat": {"id": 777000123}},
            "data": "reminder_dismiss:spotify",
        }
        await bot_service.handle_update({"update_id": 2, "callback_query": cb_dismiss})
        st = db_service.get_setup_state().get("spotify", {}).get("status")
        self.assertEqual(st, "dismissed")

    async def test_05_bot_setup_and_reset_commands(self):
        """Перевірка команд /setup та /reset."""
        # /setup
        upd_setup = {
            "update_id": 3,
            "message": {"message_id": 102, "chat": {"id": 777000123}, "text": "/setup"}
        }
        await bot_service.handle_update(upd_setup)
        self.mock_api.assert_called()

        # /reset
        upd_reset = {
            "update_id": 4,
            "message": {"message_id": 103, "chat": {"id": 777000123}, "text": "/reset"}
        }
        await bot_service.handle_update(upd_reset)

        # reset_act:accounts
        cb_reset_acc = {
            "id": "cb3",
            "from": {"id": 777000123},
            "message": {"message_id": 104, "chat": {"id": 777000123}},
            "data": "reset_act:accounts",
        }
        await bot_service.handle_update({"update_id": 5, "callback_query": cb_reset_acc})
        self.assertFalse(spotify_service.is_linked())
        self.assertFalse(google_calendar_service.is_linked())


if __name__ == "__main__":
    unittest.main()
