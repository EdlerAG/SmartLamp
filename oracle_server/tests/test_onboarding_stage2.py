# Файл: oracle_server/tests/test_onboarding_stage2.py
import unittest
import asyncio
import os
import sys

# Додаємо кореневий каталог oracle_server до sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from app.services.database_service import db_service
from app.services.setup_service import setup_service
from app.services.state_service import state_service
from app.services.bot_service import bot_service
from app.services.routine_service import routine_service
from app.services.blackout_service import blackout_service
from app.core.config import settings


class TestOnboardingStage2(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        # Очищаємо тестовий стан
        db_service.purge_sandbox_test_data()
        db_service.set_viewer_alerts_enabled(True)
        bot_service.admin_preview_girl = False
        self.sent_messages = []

        # Підміняємо _api бота для запису викликів на рівні класу TelegramBotService
        from app.services.bot_service import TelegramBotService
        async def mock_api(self_instance, method: str, payload: dict):
            self.sent_messages.append({"method": method, "payload": payload})
            if method == "sendMessage":
                return {"ok": True, "result": {"message_id": len(self.sent_messages)}}
            return {"ok": True}

        self.orig_api = TelegramBotService._api
        TelegramBotService._api = mock_api

    async def asyncTearDown(self):
        from app.services.bot_service import TelegramBotService
        TelegramBotService._api = self.orig_api

    async def test_01_bot_start_claim_deeplink(self):
        """Тест автоматичної прив'язки через /start c_<secret> та виведення Кроку 1 (Настрій)."""
        secret = "BOTCLAIM777"
        owner_chat = "99887711"
        db_service.register_device_claim(secret)

        msg = {
            "chat": {"id": int(owner_chat)},
            "from": {"first_name": "Owner"},
            "text": f"/start c_{secret}",
        }
        await bot_service._handle_message(msg)

        # Перевіряємо прив'язку
        self.assertEqual(db_service.get_owner_chat_id(), owner_chat)
        self.assertTrue(db_service.is_device_claimed())

        # Перевіряємо відправку привітання та клавіатури вибору атмосфери
        self.assertTrue(len(self.sent_messages) > 0)
        sent = self.sent_messages[-1]
        self.assertEqual(sent["method"], "sendMessage")
        self.assertIn("Крок 1 із 4", sent["payload"]["text"])
        kb = sent["payload"]["reply_markup"]["inline_keyboard"]
        callback_data_list = [btn["callback_data"] for row in kb for btn in row]
        self.assertIn("setup_mood:cozy", callback_data_list)
        self.assertIn("setup_mood:sakura", callback_data_list)

    async def test_02_bot_onboarding_wizard_callbacks(self):
        """Тест послідовного проходження кроків онбордингу через CallbackQueries."""
        chat_id = "99887711"
        msg_id = 42

        # 1. Вибір настрою 'sakura'
        cb_mood = {
            "id": "cb1",
            "data": "setup_mood:sakura",
            "message": {"chat": {"id": int(chat_id)}, "message_id": msg_id},
        }
        await bot_service._handle_callback(cb_mood)
        self.assertEqual(db_service.get_theme_mood(), "sakura")
        steps = db_service.get_setup_state()
        self.assertEqual(steps["mood"]["status"], "done")

        # Перевіряємо, що наступний крок це Spotify
        last_edit = [m for m in self.sent_messages if m["method"] == "editMessageText"][-1]
        self.assertIn("Крок 2 із 4", last_edit["payload"]["text"])

        # 2. Пропуск Spotify
        cb_sp = {
            "id": "cb2",
            "data": "setup_skip:spotify",
            "message": {"chat": {"id": int(chat_id)}, "message_id": msg_id},
        }
        await bot_service._handle_callback(cb_sp)
        steps = db_service.get_setup_state()
        self.assertEqual(steps["spotify"]["status"], "skipped")

        # Перевіряємо, що наступний крок це Календар
        last_edit = [m for m in self.sent_messages if m["method"] == "editMessageText"][-1]
        self.assertIn("Крок 3 із 4", last_edit["payload"]["text"])

        # 3. Пропуск Календаря
        cb_cal = {
            "id": "cb3",
            "data": "setup_skip:calendar",
            "message": {"chat": {"id": int(chat_id)}, "message_id": msg_id},
        }
        await bot_service._handle_callback(cb_cal)
        steps = db_service.get_setup_state()
        self.assertEqual(steps["calendar"]["status"], "skipped")

        # Перевіряємо, що наступний крок це Тихі години
        last_edit = [m for m in self.sent_messages if m["method"] == "editMessageText"][-1]
        self.assertIn("Крок 4 із 4", last_edit["payload"]["text"])

        # 4. Вибір тихих годин 22:00 - 08:00
        cb_qh = {
            "id": "cb4",
            "data": "setup_qh:22_08",
            "message": {"chat": {"id": int(chat_id)}, "message_id": msg_id},
        }
        await bot_service._handle_callback(cb_qh)
        steps = db_service.get_setup_state()
        self.assertEqual(steps["quiet_hours"]["status"], "done")
        self.assertEqual(steps["portal_tour"]["status"], "done")

        # Перевіряємо фінальне повідомлення
        last_edit = [m for m in self.sent_messages if m["method"] == "editMessageText"][-1]
        self.assertIn("повністю готова до роботи", last_edit["payload"]["text"])

    async def test_03_viewer_calendar_read_only(self):
        """Тест режиму перегляду календаря для глядача: події видно, редагування заблоковано."""
        # 1. Перегляд календаря для глядача
        text, kb = await bot_service._calendar_view("", is_viewer=True)
        self.assertIn("КАЛЕНДАР ТА РОЗКЛАД", text)
        self.assertIn("Режим перегляду", text)

        # Перевіряємо, що кнопки видалення подій відсутні
        cb_data = [btn.get("callback_data", "") for row in kb.get("inline_keyboard", []) for btn in row]
        for cb in cb_data:
            self.assertFalse(cb.startswith("cal_act:del_"))
            self.assertNotEqual(cb, "cal_act:disconnect")

        # 2. Спроба глядача створити подію з тексту блокується
        admin_chat = "1047202957"
        state_service.save_auth_ids(admin_id=admin_chat)
        msg = {
            "chat": {"id": int(admin_chat)},
            "from": {"first_name": "Viewer"},
            "text": "16:00 Тестова зустріч",
        }
        await bot_service._handle_message(msg)
        sent = self.sent_messages[-1]
        self.assertIn("Режим перегляду", sent["payload"]["text"])

    async def test_04_viewer_alerts_toggle_and_blackout(self):
        """Тест перемикання сповіщень глядача та придушення блекаут-тривог при вимкненні."""
        admin_chat = "1047202957"
        state_service.save_auth_ids(admin_id=admin_chat)

        # Перевіряємо перемикання через callback
        cb_alert = {
            "id": "cba",
            "data": "admin:toggle_alerts",
            "message": {"chat": {"id": int(admin_chat)}, "message_id": 10},
        }
        # За замовчуванням True -> після кліку стане False
        await bot_service._handle_callback(cb_alert)
        self.assertFalse(db_service.get_viewer_alerts_enabled())

        # Перевірка blackout_service: якщо alerts вимкнені, send_tg_alert нічого не надсилає
        # Підміняємо http_pool для перевірки
        mock_http_calls = []
        class MockHttp:
            async def post(self, url, **kwargs):
                mock_http_calls.append({"url": url, "kwargs": kwargs})

        from unittest.mock import patch
        with patch("app.services.blackout_service.http_pool.get", return_value=MockHttp()):
            await blackout_service.send_tg_alert("Блекаут!", chat_id=admin_chat)
            self.assertEqual(len(mock_http_calls), 0)

            # Вмикаємо назад
            db_service.set_viewer_alerts_enabled(True)
            await blackout_service.send_tg_alert("Блекаут!", chat_id=admin_chat)
            self.assertEqual(len(mock_http_calls), 1)


if __name__ == "__main__":
    unittest.main()
