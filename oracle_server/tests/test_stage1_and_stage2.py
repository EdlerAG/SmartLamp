import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from fastapi.testclient import TestClient

from app.core.config import settings
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.spotify_service import spotify_service
from app.services.google_calendar_service import google_calendar_service
from app.services.maintenance_service import maintenance_service
from main import app


class TestStage1AndStage2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.client = TestClient(app)
        cls.headers = {"X-Device-Key": settings.API_KEY}
        cls.admin_headers = {"Authorization": f"Bearer {settings.API_KEY}"}

    def test_01_full_state_and_voice_presets(self):
        """Перевірка виправлення бага db_service.list_voice_presets() -> audio_service.list_voice_presets()."""
        resp = self.client.get("/api/v5/state", headers=self.headers)
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertTrue(data.get("ok"))
        self.assertIn("voice_presets", data)
        self.assertIsInstance(data["voice_presets"], list)

        # Перевірка окремого ендпоінта списку пресетів
        p_resp = self.client.get("/api/v5/audio/presets", headers=self.headers)
        self.assertEqual(p_resp.status_code, 200)
        self.assertTrue(p_resp.json().get("ok"))

    def test_02_play_voice_preset_handling(self):
        """Перевірка обробки результату play_voice_preset без падіння на AttributeError."""
        resp = self.client.post("/api/v5/audio/preset/unknown_test_voice_123/play", headers=self.headers)
        self.assertEqual(resp.status_code, 404)

    def test_03_infographic_png_generation(self):
        """Перевірка виправлення генерації інфографіки GET /api/v5/infographic/png."""
        resp = self.client.get("/api/v5/infographic/png", headers=self.headers)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.headers.get("content-type"), "image/png")
        self.assertTrue(resp.content.startswith(b"\x89PNG\r\n\x1a\n"))

    def test_04_spotify_and_google_disconnect(self):
        """Перевірка Unlink для Spotify та Google Календаря з очищенням подій."""
        # 1. Створюємо тестову подію з source=google
        db_service.save_calendar_event(
            event_id="test_g_1",
            summary="Тест зустріч",
            start_ts=2000000000,
            source="google",
        )
        evs = db_service.list_upcoming_calendar_events()
        self.assertTrue(any(e["id"] == "test_g_1" for e in evs))

        # 2. Відключаємо Google Календар
        g_resp = self.client.post("/api/v5/google/disconnect", headers=self.headers)
        self.assertEqual(g_resp.status_code, 200)
        evs_after = db_service.list_upcoming_calendar_events()
        self.assertFalse(any(e["id"] == "test_g_1" for e in evs_after))

        # 3. Відключаємо Spotify
        sp_resp = self.client.post("/api/v5/spotify/disconnect", headers=self.headers)
        self.assertEqual(sp_resp.status_code, 200)
        self.assertFalse(spotify_service.is_linked())

    def test_05_sandbox_reset_for_nastya(self):
        """Перевірка передподарункового скидання та прив'язки Насті (1294097414)."""
        # Додаємо тестовий семпл клімату та блекаут
        db_service.add_climate_sample(23.5, 45.0, 755.0, 60)
        db_service.record_outage(1000.0, 1100.0, 100.0)

        resp = self.client.post("/api/v5/admin/sandbox-reset", headers=self.admin_headers)
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertTrue(data.get("ok"))
        self.assertEqual(state_service.girl_id, "1294097414")

    def test_06_event_audit_log_and_health(self):
        """Перевірка журналу подій event_log та телеметрії здоров'я плати."""
        db_service.log_event("admin", "test", "verification_action", {"test_key": 42})
        events = db_service.get_recent_events(limit=10)
        self.assertTrue(any(e["action"] == "verification_action" for e in events))

        e_resp = self.client.get("/api/v5/admin/events", headers=self.admin_headers)
        self.assertEqual(e_resp.status_code, 200)
        self.assertTrue(e_resp.json().get("ok"))

        # Здоров'я плати
        db_service.record_box_health(rssi=-55, heap=220000, psram=4000000, uptime=3600, reset_reason=1)
        h_resp = self.client.get("/api/v5/admin/box-health", headers=self.admin_headers)
        self.assertEqual(h_resp.status_code, 200)
        self.assertTrue(h_resp.json().get("ok"))

    def test_07_daily_rollups(self):
        """Перевірка добової агрегації та ролапів."""
        maintenance_service.calculate_and_save_daily_rollup()
        rollups = db_service.get_daily_rollups(days=7)
        self.assertIsInstance(rollups, list)
        self.assertGreaterEqual(len(rollups), 1)

        r_resp = self.client.get("/api/v5/admin/rollups", headers=self.admin_headers)
        self.assertEqual(r_resp.status_code, 200)
        self.assertTrue(r_resp.json().get("ok"))

    def test_08_websocket_dashboard(self):
        """Перевірка WebSocket каналу /ws/dashboard (Етап 3, v5.6 Realtime)."""
        token = "test_ws_token_stage_123"
        db_service.create_session(token, role="admin", duration_sec=3600)
        with self.client.websocket_connect(f"/ws/dashboard?token={token}") as ws:
            data = ws.receive_json()
            self.assertEqual(data.get("type"), "init")
            self.assertIn("state", data)
            self.assertIn("telemetry", data["state"])

    def test_09_server_updater_status(self):
        """Перевірка статусу самооновлення сервера /api/v5/admin/server-update/status (Етап 4)."""
        resp = self.client.get("/api/v5/admin/server-update/status", headers=self.admin_headers)
        self.assertEqual(resp.status_code, 200)
        data = resp.json()
        self.assertTrue(data.get("ok"))
        self.assertIn("current_commit", data)
        self.assertIn("update_available", data)


if __name__ == "__main__":
    unittest.main()
