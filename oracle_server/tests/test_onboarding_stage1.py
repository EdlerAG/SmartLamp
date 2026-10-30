# Файл: oracle_server/tests/test_onboarding_stage1.py
import unittest
import os
import sys

# Додаємо кореневий каталог oracle_server до sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from fastapi.testclient import TestClient
from main import app
from app.services.database_service import db_service
from app.services.setup_service import setup_service
from app.core.config import settings


class TestOnboardingStage1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.client = TestClient(app)
        cls.admin_headers = {"Authorization": f"Bearer {settings.API_KEY}"}

    def test_01_privacy_and_terms_public(self):
        """Перевірка публічних HTML сторінок для Google Cloud OAuth Branding (без авторизації)."""
        res_priv = self.client.get("/privacy")
        self.assertEqual(res_priv.status_code, 200)
        self.assertIn("text/html", res_priv.headers.get("content-type", ""))
        self.assertIn("Політика конфіденційності", res_priv.text)
        self.assertIn("MagicBox", res_priv.text)

        res_terms = self.client.get("/terms")
        self.assertEqual(res_terms.status_code, 200)
        self.assertIn("text/html", res_terms.headers.get("content-type", ""))
        self.assertIn("Умови використання", res_terms.text)
        self.assertIn("MagicBox", res_terms.text)

    def test_02_device_claim_flow(self):
        """Перевірка криптографічної реєстрації та прив'язки секрету ESP32."""
        secret = "TESTBOXCLAIM1"
        chat_id_owner = "777000123"

        # Реєструємо секрет
        db_service.register_device_claim(secret)

        # Прив'язуємо пристрій
        ok = db_service.claim_device(secret, chat_id_owner)
        self.assertTrue(ok)
        self.assertEqual(db_service.get_owner_chat_id(), chat_id_owner)
        self.assertTrue(db_service.is_device_claimed())

        # Повторна прив'язка тим самим користувачем (ідемпотентність)
        ok_again = db_service.claim_device(secret, chat_id_owner)
        self.assertTrue(ok_again)

        # Спроба перехоплення іншим користувачем
        ok_hijack = db_service.claim_device(secret, "999888111")
        self.assertFalse(ok_hijack)
        self.assertEqual(db_service.get_owner_chat_id(), chat_id_owner)

    def test_03_setup_state_and_bitmask(self):
        """Перевірка розрахунку бітової маски 'su' та зміни тем настрою."""
        status = setup_service.get_full_status()
        self.assertIn("bitmask", status)
        self.assertIn("steps", status)
        self.assertIn("theme_mood", status)

        # Встановлюємо настрій 'sakura'
        mood = setup_service.set_mood("sakura")
        self.assertEqual(mood, "sakura")
        self.assertEqual(db_service.get_theme_mood(), "sakura")

        # Перевіряємо бітову маску
        bitmask = setup_service.get_setup_bitmask()
        self.assertTrue(bitmask & setup_service.BIT_WIFI_OK)
        self.assertTrue(bitmask & setup_service.BIT_MOOD_SET)

        # Завершуємо крок тихих годин
        setup_service.advance_step("quiet_hours", "done")
        bitmask_after = setup_service.get_setup_bitmask()
        self.assertTrue(bitmask_after & setup_service.BIT_QUIET_HOURS)

    def test_04_viewer_alerts_toggle(self):
        """Перевірка вмикання/вимикання сповіщень глядачу."""
        db_service.set_viewer_alerts_enabled(False)
        self.assertFalse(db_service.get_viewer_alerts_enabled())

        db_service.set_viewer_alerts_enabled(True)
        self.assertTrue(db_service.get_viewer_alerts_enabled())

    def test_05_setup_api_endpoints(self):
        """Тестування REST ендпоінтів /api/v5/setup/*."""
        # Отримання статусу
        res = self.client.get("/api/v5/setup/state", headers=self.admin_headers)
        self.assertEqual(res.status_code, 200)
        data = res.json()
        self.assertIn("bitmask", data)
        self.assertIn("theme_mood", data)

        # Встановлення настрою через API
        res_mood = self.client.post("/api/v5/setup/mood", json={"mood": "ocean"}, headers=self.admin_headers)
        self.assertEqual(res_mood.status_code, 200)
        self.assertEqual(res_mood.json().get("mood"), "ocean")

        # Оновлення статусу кроку
        res_step = self.client.post("/api/v5/setup/step/portal_tour", json={"status": "done"}, headers=self.admin_headers)
        self.assertEqual(res_step.status_code, 200)
        self.assertEqual(res_step.json().get("status"), "done")

        # Перевірка налаштування сповіщень глядачу через API
        res_alerts = self.client.get("/api/v5/admin/viewer-alerts", headers=self.admin_headers)
        self.assertEqual(res_alerts.status_code, 200)

        res_alerts_set = self.client.post("/api/v5/admin/viewer-alerts", json={"enabled": False}, headers=self.admin_headers)
        self.assertEqual(res_alerts_set.status_code, 200)
        self.assertFalse(res_alerts_set.json().get("enabled"))

    def test_06_box_sync_includes_su_and_mood(self):
        """Перевірка наявності 'su' та 'mood' у відповіді /api/v5/box/sync та автореєстрації claim."""
        claim_secret = "SYNCSECRET99"
        payload = {
            "fw": "v5.0",
            "t": 22.4,
            "h": 45.0,
            "p": 1012.3,
            "l": 150,
            "m": 1,
            "b_pct": 50,
            "b_lvl": 1,
            "vol": 10,
            "claim": claim_secret,
        }
        res = self.client.post(
            "/api/v5/box/sync",
            json=payload,
            headers={"X-Device-Key": settings.API_KEY},
        )
        self.assertEqual(res.status_code, 200)
        data = res.json()
        self.assertIn("su", data)
        self.assertIsInstance(data["su"], int)
        self.assertIn("mood", data)
        self.assertEqual(data["mood"], "ocean")

        # Перевіряємо, що секрет плати зареєстровано в БД
        claim_hash = import_hashlib = __import__("hashlib").sha256(claim_secret.encode("utf-8")).hexdigest()
        row = db_service._get_conn().execute(
            "SELECT claim_hash FROM device_claim WHERE claim_hash = ?", (claim_hash,)
        ).fetchone()
        self.assertIsNotNone(row)

    def test_07_chernobyl_mood_theme(self):
        """Перевірка вибору атмосфери 'chernobyl' та оновлення bitmask."""
        res = self.client.post("/api/v5/setup/mood", json={"mood": "chernobyl"}, headers=self.admin_headers)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(res.json().get("mood"), "chernobyl")

        state_res = self.client.get("/api/v5/setup/state", headers=self.admin_headers)
        self.assertEqual(state_res.status_code, 200)
        data = state_res.json()
        self.assertEqual(data["steps"]["mood"]["mood"], "chernobyl")


if __name__ == "__main__":
    unittest.main()
