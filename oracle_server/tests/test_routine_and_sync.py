import os
import sys
import unittest
import time

# Ensure oracle_server directory is in Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from app.domain.schemas import CommandPayload, BoxTelemetryPayload
from app.services.state_service import state_service
from app.services.routine_service import routine_service


class TestRoutineAndSync(unittest.TestCase):
    def setUp(self):
        state_service.command_queue.clear()
        routine_service.stop_pomodoro()
        state_service.command_queue.clear()

    def test_pomodoro_start_lifecycle(self):
        """Перевірка старту Pomodoro та відсутності сміття в черзі."""
        status = routine_service.start_pomodoro(work_min=25)
        self.assertTrue(status["active"])
        self.assertEqual(status["mode"], "work")
        self.assertGreater(status["remaining_sec"], 1400)

        # Перевірка поставленої команди
        self.assertGreater(len(state_service.command_queue), 0)
        cmd = state_service.command_queue[-1]
        self.assertEqual(cmd.get("pomo_action"), "start")
        self.assertEqual(cmd.get("pomo_min"), "25")
        self.assertEqual(cmd.get("set_mode"), 2)
        # Гарантуємо відсутність полів msg та sender, що викликали хибні сповіщення
        self.assertNotIn("msg", cmd)
        self.assertNotIn("sender", cmd)

    def test_pomodoro_transition_to_break(self):
        """Перевірка автоматичного переходу фокусу на 5-хвилинну перерву зі статусом pomo_break."""
        routine_service.start_pomodoro(work_min=25)
        state_service.command_queue.clear()

        # Симулюємо завершення робочого інтервалу фокусу
        routine_service._pomodoro_end_time = time.time() - 1
        status = routine_service.get_pomodoro_status()

        self.assertTrue(status["active"])
        self.assertEqual(status["mode"], "break")
        self.assertEqual(status["remaining_min"], 5)

        # Перевіряємо, що сервер поставив команду перерви з прапорцем pomo_break
        self.assertGreater(len(state_service.command_queue), 0)
        cmd = state_service.command_queue[-1]
        self.assertEqual(cmd.get("pomo_action"), "start")
        self.assertTrue(cmd.get("pomo_break"))
        self.assertEqual(cmd.get("pomo_min"), "5")
        self.assertEqual(cmd.get("set_mode"), 7)  # Океанський спокійний бриз

    def test_pomodoro_stop_clean(self):
        """Перевірка зупинки Pomodoro без примусового скидання режиму світла на 0."""
        routine_service.start_pomodoro(work_min=25)
        state_service.command_queue.clear()

        status = routine_service.stop_pomodoro()
        self.assertFalse(status["active"])
        self.assertEqual(status["mode"], "idle")

        self.assertGreater(len(state_service.command_queue), 0)
        cmd = state_service.command_queue[-1]
        self.assertEqual(cmd.get("pomo_action"), "stop")
        self.assertTrue(cmd.get("stop_media"))
        # Режим світла не повинен примусово перетиратися
        self.assertNotIn("set_mode", cmd)

    def test_pomodoro_sync_from_box(self):
        """Перевірка синхронізації стану Pomodoro, запущеного кнопками на шкатулці."""
        routine_service.sync_pomodoro_from_box(
            active=True,
            remaining_sec=1200,
            is_break=False,
            duration_min=25
        )
        status = routine_service.get_pomodoro_status()
        self.assertTrue(status["active"])
        self.assertEqual(status["mode"], "work")
        self.assertGreaterEqual(status["remaining_sec"], 1195)

        # Синхронізація вимкнення з лампи
        routine_service.sync_pomodoro_from_box(
            active=False,
            remaining_sec=0,
            is_break=False
        )
        status = routine_service.get_pomodoro_status()
        self.assertFalse(status["active"])

    def test_sunrise_alarm_brightness_preset_mapping(self):
        """Перевірка мапінгу відсотків світанку на дискретні індекси яскравості лампи (0..3)."""
        test_cases = [
            (5, "0"),   # 15%
            (25, "0"),  # 15%
            (26, "1"),  # 35%
            (50, "1"),  # 35%
            (51, "2"),  # 65%
            (75, "2"),  # 65%
            (76, "3"),  # 100%
            (95, "3"),  # 100%
        ]
        for ramp_pct, expected_preset in test_cases:
            p_idx = "0" if ramp_pct <= 25 else ("1" if ramp_pct <= 50 else ("2" if ramp_pct <= 75 else "3"))
            self.assertEqual(p_idx, expected_preset, f"Failed for ramp_pct {ramp_pct}")

    def test_system_command_queue(self):
        """Перевірка постановки та вивантаження системних команд обслуговування (sys_cmd)."""
        for act in ["reboot", "clear_cache", "start_ap"]:
            state_service.command_queue.clear()
            cmd = CommandPayload(sys_cmd=act)
            state_service.enqueue_command(cmd)

            resp = state_service.pop_next_sync_response()
            self.assertTrue(resp["has_cmd"])
            self.assertEqual(resp.get("sys_cmd"), act)

    def test_box_telemetry_payload_validation(self):
        """Перевірка парсингу повної телеметрії з новими полями rst, pomo та sp_act."""
        raw_telemetry = {
            "fw": "v5.3-Hallmark",
            "t": 22.4,
            "h": 48.0,
            "p": 745.0,
            "m": 2,
            "vol": 14,
            "dnd": False,
            "pomo": True,
            "pomo_rem": 1200,
            "pomo_brk": False,
            "pomo_dur": 25,
            "sp_act": 1,
            "rst": "POWERON",
            "heap": 180000,
            "psram_free": 4500000,
        }
        parsed = BoxTelemetryPayload(**raw_telemetry)
        self.assertEqual(parsed.fw, "v5.3-Hallmark")
        self.assertEqual(parsed.rst, "POWERON")
        self.assertEqual(parsed.pomo_dur, 25)
        self.assertEqual(parsed.sp_act, 1)


if __name__ == "__main__":
    unittest.main()
