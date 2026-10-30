# Файл: oracle_server/tests/test_v56_core.py
import sys
import os
import unittest
import time

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from app.services.routine_service import routine_service
from app.services.database_service import db_service
from app.services.spotify_service import spotify_service
from app.core.events import realtime_hub


class TestV56Core(unittest.TestCase):
    def test_routine_service_pomodoro(self):
        st = routine_service.start_pomodoro(25)
        self.assertTrue(st["active"])
        self.assertEqual(st["mode"], "work")
        self.assertGreater(st["remaining_sec"], 0)

        st_stop = routine_service.stop_pomodoro()
        self.assertFalse(st_stop["active"])
        self.assertEqual(st_stop["mode"], "idle")

    def test_quiet_hours(self):
        cfg = routine_service.get_quiet_hours()
        self.assertIn("enabled", cfg)
        self.assertIn("start", cfg)
        self.assertIn("end", cfg)

    def test_session_validation(self):
        # Empty or invalid token must return None
        self.assertIsNone(db_service.validate_session(""))
        self.assertIsNone(db_service.validate_session("invalid-token-12345"))

        # Create session and validate
        test_tok = "test-token-v56-9999"
        created_tok = db_service.create_session(test_tok, role="admin", duration_sec=3600)
        self.assertEqual(created_tok, test_tok)
        role = db_service.validate_session(test_tok)
        self.assertEqual(role, "admin")
        self.assertIsInstance(role, str)

        # Cleanup
        db_service.delete_session(test_tok)
        self.assertIsNone(db_service.validate_session(test_tok))

    def test_spotify_cached_state_structure(self):
        st = spotify_service.get_cached_state()
        self.assertIn("playing", st)
        self.assertIn("title", st)
        self.assertIn("artist", st)
        self.assertIn("progress_ms", st)
        self.assertIn("duration_ms", st)
        self.assertIn("linked", st)

    def test_realtime_hub_initial_state(self):
        self.assertFalse(realtime_hub.is_box_ws_connected)
        self.assertEqual(len(realtime_hub._dashboard_clients), 0)

    def test_settings_v56_security_and_version(self):
        from app.core.config import settings
        self.assertEqual(settings.VERSION, "5.6.0-Hallmark")
        self.assertIn("1047202957", settings.ALLOWED_ADMIN_IDS)
        self.assertIn("1294097414", settings.ALLOWED_USER_IDS)

    def test_persistent_command_queue(self):
        db_service.clear_pending_commands()
        self.assertEqual(len(db_service.load_pending_commands()), 0)

        cmd1 = {"set_mode": 3, "set_brt": "2"}
        cmd2 = {"msg": "Тест персистентності"}
        db_service.save_pending_command(cmd1)
        db_service.save_pending_command(cmd2)

        loaded = db_service.load_pending_commands()
        self.assertEqual(len(loaded), 2)
        self.assertEqual(loaded[0].get("set_mode"), 3)
        self.assertEqual(loaded[1].get("msg"), "Тест персистентності")

        db_service.clear_pending_commands()
        self.assertEqual(len(db_service.load_pending_commands()), 0)

    def test_box_health_ws_rec_and_ota(self):
        db_service.record_box_health(
            rssi=-62,
            heap=240000,
            psram=4100000,
            uptime=7200,
            reset_reason=1,
            ws_rec=5,
            ota=1,
        )
        series = db_service.get_box_health_series(hours=1)
        self.assertGreaterEqual(len(series), 1)
        latest = series[-1]
        self.assertIn("ws_rec", latest)
        self.assertIn("ota", latest)
        self.assertEqual(latest["ws_rec"], 5)
        self.assertEqual(latest["ota"], 1)

    def test_metrics_and_ws_ticket(self):
        from fastapi.testclient import TestClient
        from main import app
        from app.core.config import settings

        client = TestClient(app)
        headers = {"X-Device-Key": settings.API_KEY}

        # 1. WS-Ticket
        t_resp = client.post("/api/v5/auth/ws-ticket", headers=headers)
        self.assertEqual(t_resp.status_code, 200)
        t_data = t_resp.json()
        self.assertTrue(t_data.get("ok"))
        ticket = t_data.get("ticket")
        self.assertTrue(bool(ticket))
        # Validate ticket session
        role = db_service.validate_session(ticket)
        self.assertEqual(role, "device")
        db_service.delete_session(ticket)

        # 2. Metrics endpoint
        m_resp = client.get("/api/v5/metrics", headers=headers)
        self.assertEqual(m_resp.status_code, 200)
        m_data = m_resp.json()
        self.assertTrue(m_data.get("ok"))
        self.assertIn("realtime", m_data)
        self.assertIn("box", m_data)
        self.assertIn("system", m_data)
        self.assertIn("ws_reconnects", m_data["box"])
        self.assertIn("ota_in_progress", m_data["box"])


if __name__ == "__main__":
    unittest.main()
