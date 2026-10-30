import unittest
import sys
import os
from fastapi.testclient import TestClient

# Додаємо oracle_server до sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from main import app
from app.core.config import settings
from app.api.endpoints_control import router as control_router
from app.api.endpoints_box import router as box_router
from app.services.routine_service import routine_service
from app.services.state_service import state_service
from app.services.infographic_service import infographic_service
from app.domain.schemas import VolumeControlRequest, VoiceConfigRequest, BoxTelemetryPayload


class TestServerBootAndContracts(unittest.TestCase):
    def setUp(self):
        self.client = TestClient(app)

    def test_app_and_routers_loaded(self):
        """Перевірка успішного імпорту всіх модулів без NameError / AttributeError."""
        self.assertIsNotNone(app)
        self.assertIsNotNone(control_router)
        self.assertIsNotNone(box_router)

        # Перевірка наявності ключових роутів
        routes = [r.path for r in box_router.routes + control_router.routes]
        self.assertIn("/api/v5/box/sync", routes)
        self.assertIn("/api/v5/state", routes)
        self.assertIn("/api/v5/audio/volume", routes)
        self.assertIn("/api/v5/audio/voice-config", routes)
        self.assertIn("/api/v5/system/reboot", routes)
        self.assertIn("/api/v5/system/clear-cache", routes)
        self.assertIn("/api/v5/system/start-ap", routes)
        self.assertIn("/api/v5/nastya/alarm/dismiss", routes)

    def test_health_and_root_endpoints(self):
        """Перевірка /health та кореневого веб-терміналу."""
        res = self.client.get("/health")
        self.assertEqual(res.status_code, 200)
        self.assertEqual(res.json().get("status"), "ok")

        res_root = self.client.get("/")
        self.assertEqual(res_root.status_code, 200)
        self.assertIn("text/html", res_root.headers.get("content-type", ""))

    def test_infographic_generation_no_name_error(self):
        """Перевірка генерації інфографіки (валідний PNG-заголовок, відсутність NameError io)."""
        png_data = infographic_service.generate_status_infographic()
        self.assertIsNotNone(png_data)
        self.assertTrue(png_data.startswith(b"\x89PNG\r\n\x1a\n"))

    def test_box_sync_endpoint_auth_and_response(self):
        """Перевірка автентифікації та успішного синку шкатулки через TestClient."""
        payload = {
            "fw": "v5.4",
            "t": 22.4,
            "h": 55.0,
            "p": 748.2,
            "p_diff": 0.1,
            "l": 40,
            "m": 0,
            "m_name": "Теплий затишок",
            "b_pct": 65,
            "b_lvl": 2,
            "vol": 12,
            "muted": False,
            "t_beep": True,
            "dnd": False,
            "pomo": False,
            "pomo_rem": 0,
            "pomo_brk": False,
            "pomo_dur": 25,
            "sleep_m": 0,
            "media_m": 0,
            "scr": 0,
            "rssi": -55,
            "time": "14:35",
            "unread": False,
            "heap": 120000,
            "psram_free": 4194304,
            "ack": False,
            "ping": 0,
            "sp_act": 0,
            "rst": "POWERON",
            "alarm_off": False
        }
        # Без ключа доступу -> 401 Unauthorized
        res_unauth = self.client.post("/api/v5/box/sync", json=payload)
        self.assertEqual(res_unauth.status_code, 401)

        # З валідним ключем X-Device-Key -> 200 OK
        headers = {"X-Device-Key": settings.API_KEY}
        res = self.client.post("/api/v5/box/sync", json=payload, headers=headers)
        self.assertEqual(res.status_code, 200)
        data = res.json()
        self.assertIn("has_cmd", data)
        self.assertIn("quiet_hours", data)
        self.assertIn("bot_managed", data)

    def test_volume_and_voice_schemas(self):
        """Перевірка схеми налаштувань звуку та голосу."""
        vol_req = VolumeControlRequest(volume=15, muted=False, touch_beep=True)
        self.assertEqual(vol_req.volume, 15)

        voice_req = VoiceConfigRequest(provider="edge", voice_name="uk-UA-OstapNeural", voice_id=None)
        self.assertEqual(voice_req.voice_name, "uk-UA-OstapNeural")

    def test_alarm_dismiss(self):
        """Перевірка скидання будильника на день."""
        today = routine_service.get_kyiv_now().strftime("%Y-%m-%d")
        routine_service.dismiss_alarm()
        self.assertEqual(routine_service._alarm_dismissed_date, today)

        # check_alarm_state повинен повертати active: False
        st = routine_service.check_alarm_state()
        self.assertFalse(st.get("active", False))

    def test_pomodoro_work_duration_preservation(self):
        """Перевірка збереження тривалості роботи після перерви."""
        routine_service.start_pomodoro(work_min=45)
        self.assertEqual(routine_service._pomodoro_work_duration_min, 45)
        self.assertEqual(routine_service._pomodoro_duration_min, 45)

        # Імітація переходу на перерву
        routine_service._pomodoro_mode = "break"
        routine_service._pomodoro_duration_min = 5
        self.assertEqual(routine_service._pomodoro_work_duration_min, 45)

        # Зупинка
        routine_service.stop_pomodoro()
        self.assertEqual(routine_service._pomodoro_duration_min, 45)

    def test_box_telemetry_alarm_off(self):
        """Перевірка парсингу поля alarm_off у телеметрії."""
        payload = BoxTelemetryPayload(
            fw="v5.4",
            t=21.5,
            h=50.0,
            p=745.0,
            alarm_off=True,
        )
        self.assertTrue(payload.alarm_off)


if __name__ == "__main__":
    unittest.main()
