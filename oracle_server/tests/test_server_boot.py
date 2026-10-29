import unittest
import sys
import os

# Додаємо oracle_server до sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from main import app
from app.api.router import api_router
from app.api.endpoints_control import router as control_router
from app.api.endpoints_box import router as box_router
from app.services.nastya_routine_service import nastya_routine_service
from app.services.state_service import state_service
from app.domain.schemas import VolumeControlRequest, VoiceConfigRequest, BoxTelemetryPayload


class TestServerBootAndContracts(unittest.TestCase):
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

    def test_volume_and_voice_schemas(self):
        """Перевірка схеми налаштувань звуку та голосу."""
        vol_req = VolumeControlRequest(volume=15, muted=False, touch_beep=True)
        self.assertEqual(vol_req.volume, 15)

        voice_req = VoiceConfigRequest(provider="edge", voice_name="uk-UA-OstapNeural", voice_id=None)
        self.assertEqual(voice_req.voice_name, "uk-UA-OstapNeural")

    def test_alarm_dismiss(self):
        """Перевірка скидання будильника на день."""
        today = nastya_routine_service.get_kyiv_now().strftime("%Y-%m-%d")
        nastya_routine_service.dismiss_alarm()
        self.assertEqual(nastya_routine_service._alarm_dismissed_date, today)

        # check_alarm_state повинен повертати active: False
        st = nastya_routine_service.check_alarm_state()
        self.assertFalse(st.get("active", False))

    def test_pomodoro_work_duration_preservation(self):
        """Перевірка збереження тривалості роботи після перерви."""
        nastya_routine_service.start_pomodoro(work_min=45)
        self.assertEqual(nastya_routine_service._pomodoro_work_duration_min, 45)
        self.assertEqual(nastya_routine_service._pomodoro_duration_min, 45)

        # Імітація переходу на перерву
        nastya_routine_service._pomodoro_mode = "break"
        nastya_routine_service._pomodoro_duration_min = 5
        self.assertEqual(nastya_routine_service._pomodoro_work_duration_min, 45)

        # Зупинка
        nastya_routine_service.stop_pomodoro()
        self.assertEqual(nastya_routine_service._pomodoro_duration_min, 45)

    def test_box_telemetry_alarm_off(self):
        """Перевірка парсингу поля alarm_off у телеметрії."""
        payload = BoxTelemetryPayload(
            fw="v5.3",
            t=21.5,
            h=50.0,
            p=745.0,
            alarm_off=True,
        )
        self.assertTrue(payload.alarm_off)


if __name__ == "__main__":
    unittest.main()
