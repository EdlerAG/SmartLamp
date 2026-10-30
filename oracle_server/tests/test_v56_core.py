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
        tok = db_service.create_session("admin", 3600)
        self.assertTrue(bool(tok))
        role = db_service.validate_session(tok)
        self.assertEqual(role, "admin")
        self.assertIsInstance(role, str)

        # Cleanup
        db_service.delete_session(tok)
        self.assertIsNone(db_service.validate_session(tok))

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


if __name__ == "__main__":
    unittest.main()
