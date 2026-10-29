# Файл: oracle_server/app/services/nastya_routine_service.py
import time
import datetime
from typing import Dict, Any, List, Optional
from app.core.config import settings
from app.core.logger import logger
from app.services.database_service import db_service
from app.services.state_service import state_service


class NastyaRoutineService:
    """
    Сервіси комфорту, затишку та щоденної підтримки для Насті:
    - Налаштовувані Тихі Години (Quiet Hours 23:00 - 08:00)
    - Розумний будильник «Схід Сонця» (Sunrise Alarm)
    - Режим фокусу та навчання Pomodoro (25/5 хв)
    - Ранкова метео-турбота «Що одягнути / чи брати парасольку»
    """

    __slots__ = (
        "_pomodoro_active",
        "_pomodoro_mode",
        "_pomodoro_end_time",
        "_pomodoro_duration_min",
        "_pomodoro_work_duration_min",
        "_last_alarm_date",
        "_alarm_dismissed_date",
        "_last_morning_brief_date",
    )

    def __init__(self) -> None:
        self._pomodoro_active: bool = False
        self._pomodoro_mode: str = "idle" # "work", "break", "idle"
        self._pomodoro_end_time: float = 0.0
        self._pomodoro_duration_min: int = 25
        self._pomodoro_work_duration_min: int = 25
        self._last_alarm_date: str = ""
        self._alarm_dismissed_date: str = ""
        self._last_morning_brief_date: str = ""

    @staticmethod
    def get_kyiv_now() -> datetime.datetime:
        """Повертає точний локальний час за Києвом, незалежно від часового поясу сервера."""
        try:
            from zoneinfo import ZoneInfo
            return datetime.datetime.now(ZoneInfo("Europe/Kyiv"))
        except Exception:
            return datetime.datetime.now()

    # --- 1. ТИХІ ГОДИНИ (QUIET HOURS) ---
    def get_quiet_hours(self) -> Dict[str, Any]:
        return {
            "enabled": bool(db_service.get_setting("quiet_hours_enabled", True)),
            "start": str(db_service.get_setting("quiet_hours_start", "23:00")),
            "end": str(db_service.get_setting("quiet_hours_end", "08:00")),
        }

    def set_quiet_hours(self, enabled: bool, start: str = "23:00", end: str = "08:00") -> None:
        db_service.set_setting("quiet_hours_enabled", bool(enabled))
        db_service.set_setting("quiet_hours_start", str(start))
        db_service.set_setting("quiet_hours_end", str(end))
        logger.info("Quiet hours updated: enabled=%s, %s - %s", enabled, start, end)

    def is_quiet_hours_now(self) -> bool:
        cfg = self.get_quiet_hours()
        if not cfg["enabled"]:
            return False

        try:
            now = self.get_kyiv_now()
            cur_min = now.hour * 60 + now.minute

            s_h, s_m = map(int, cfg["start"].split(":"))
            e_h, e_m = map(int, cfg["end"].split(":"))
            start_min = s_h * 60 + s_m
            end_min = e_h * 60 + e_m

            if start_min < end_min:
                return start_min <= cur_min < end_min
            else:
                # Перехід через північ (наприклад 23:00 -> 08:00)
                return cur_min >= start_min or cur_min < end_min
        except Exception as e:
            logger.warning("Error evaluating quiet hours: %s", e)
            return False

    # --- 2. РОЗУМНИЙ БУДИЛЬНИК (SUNRISE ALARM) ---
    def get_alarm_config(self) -> Dict[str, Any]:
        return {
            "enabled": bool(db_service.get_setting("alarm_enabled", False)),
            "time": str(db_service.get_setting("alarm_time", "07:30")),
            "days": db_service.get_setting("alarm_days", [0, 1, 2, 3, 4]), # Пн-Пт
            "snooze_min": int(db_service.get_setting("alarm_snooze_min", 9)),
        }

    def set_alarm_config(self, enabled: bool, time_str: str, days: List[int], snooze_min: int = 9) -> None:
        db_service.set_setting("alarm_enabled", bool(enabled))
        db_service.set_setting("alarm_time", str(time_str))
        db_service.set_setting("alarm_days", list(days))
        db_service.set_setting("alarm_snooze_min", int(snooze_min))
        logger.info("Alarm config updated: enabled=%s at %s, days=%s", enabled, time_str, days)

    def check_alarm_state(self) -> Dict[str, Any]:
        """
        Перевіряє чи потрібно запустити плавний схід сонця (за 20 хв до пробудження)
        або відтворити ранковий сигнал.
        """
        cfg = self.get_alarm_config()
        if not cfg["enabled"]:
            return {"active": False}

        now = self.get_kyiv_now()
        weekday = now.weekday()
        if weekday not in cfg["days"]:
            return {"active": False}

        today_str = now.strftime("%Y-%m-%d")
        if self._alarm_dismissed_date == today_str:
            return {"active": False}

        try:
            a_h, a_m = map(int, cfg["time"].split(":"))
            alarm_total = a_h * 60 + a_m
            cur_total = now.hour * 60 + now.minute

            diff = alarm_total - cur_total

            # Схід сонця: 20 хвилин до встановленого часу
            if 0 < diff <= 20:
                # Прогресія від 5% до 100%
                ramp_pct = int(((20 - diff) / 20.0) * 100)
                return {"active": True, "phase": "sunrise", "brightness_pct": max(5, ramp_pct)}

            # Момент пробудження (хвилина в хвилину)
            if diff == 0 and self._last_alarm_date != today_str:
                self._last_alarm_date = today_str
                return {"active": True, "phase": "alarm_ring"}

        except Exception as e:
            logger.warning("Error evaluating alarm state: %s", e)

        return {"active": False}

    def dismiss_alarm(self) -> None:
        """Скидає будильник/схід сонця на сьогодні за дотиком до шкатулки або через інтерфейс."""
        today_str = self.get_kyiv_now().strftime("%Y-%m-%d")
        self._alarm_dismissed_date = today_str
        logger.info("Alarm dismissed for today: %s", today_str)

    # --- 3. РЕЖИМ ФОКУСУ ТА НАВЧАННЯ (POMODORO) ---
    def start_pomodoro(self, work_min: int = 25, break_min: int = 5) -> Dict[str, Any]:
        now = time.time()
        self._pomodoro_active = True
        self._pomodoro_mode = "work"
        self._pomodoro_duration_min = work_min
        self._pomodoro_work_duration_min = work_min
        self._pomodoro_end_time = now + (work_min * 60)

        # Ставимо тепле затишне світло для концентрації (бурштин або камін)
        state_service.enqueue_raw({
            "pomo_action": "start",
            "pomo_min": str(work_min),
            "set_mode": 2, # Бурштинова свічка
            "set_brt": "2", # 65%
            "media_timer": work_min,
        })
        logger.info("Pomodoro started: %d min work", work_min)
        return self.get_pomodoro_status()

    def stop_pomodoro(self) -> Dict[str, Any]:
        self._pomodoro_active = False
        self._pomodoro_mode = "idle"
        self._pomodoro_duration_min = self._pomodoro_work_duration_min
        self._pomodoro_end_time = 0.0
        state_service.enqueue_raw({
            "pomo_action": "stop",
            "stop_media": True,
        })
        logger.info("Pomodoro stopped")
        return self.get_pomodoro_status()

    def get_pomodoro_status(self) -> Dict[str, Any]:
        if not self._pomodoro_active:
            return {"active": False, "mode": "idle", "remaining_sec": 0, "remaining_min": 0, "pct": 0}

        now = time.time()
        rem = max(0, int(self._pomodoro_end_time - now))
        total_sec = self._pomodoro_duration_min * 60
        pct = int(((total_sec - rem) / max(1, total_sec)) * 100)

        if rem == 0:
            if self._pomodoro_mode == "work":
                # Перехід на перерву 5 хвилин
                self._pomodoro_mode = "break"
                self._pomodoro_duration_min = 5
                self._pomodoro_end_time = now + 300
                rem = 300
                pct = 0
                state_service.enqueue_raw({
                    "pomo_action": "start",
                    "pomo_break": True,
                    "pomo_min": "5",
                    "set_mode": 7, # Океанський спокійний бриз
                    "media_timer": 5,
                    "play_url": f"{settings.PUBLIC_SERVER_URL}/api/v1/media/sound/notification.mp3",
                })
            else:
                self.stop_pomodoro()
                return {"active": False, "mode": "idle", "remaining_sec": 0, "remaining_min": 0, "pct": 100}

        return {
            "active": self._pomodoro_active,
            "mode": self._pomodoro_mode,
            "remaining_sec": rem,
            "remaining_min": int(rem // 60),
            "pct": min(100, pct),
        }

    def sync_pomodoro_from_box(self, active: bool, remaining_sec: int, is_break: bool, duration_min: int = 25) -> None:
        """Синхронізація стану Pomodoro при зміні/запуску з фізичних кнопок шкатулки."""
        self._pomodoro_active = active
        if active:
            self._pomodoro_mode = "break" if is_break else "work"
            if not is_break:
                self._pomodoro_work_duration_min = max(5, duration_min)
            self._pomodoro_duration_min = 5 if is_break else max(5, duration_min)
            self._pomodoro_end_time = time.time() + max(1, remaining_sec)
        else:
            self._pomodoro_mode = "idle"
            self._pomodoro_duration_min = self._pomodoro_work_duration_min
            self._pomodoro_end_time = 0.0

    # --- 4. РАНКОВИЙ МЕТЕО-ГІД «ЩО ОДЯГНУТИ» ДЛЯ НАСТІ ---
    async def generate_morning_brief_for_nastya(self) -> str:
        from app.services.weather_ai_service import weather_ai_service
        try:
            weather = await weather_ai_service.get_combined_weather()
            temp = float(weather.get("outdoor_temp", 15.0))
            desc = weather.get("weather_desc", "ясно")
            rain_prob = int(weather.get("rain_prob_6h", 0))
            is_rain = rain_prob >= 45 or weather.get("weather_code", 0) >= 51
            wind = float(weather.get("wind_kmh", 5.0))

            # Формуємо практичну пораду щодо одягу
            clothes = ""
            if temp < 5:
                clothes = "Одягай найтепліше пальто або пуховик, шапку і шарф ❄️"
            elif temp < 12:
                clothes = "Сьогодні прохолодно — чудово підійде тепле пальто або куртка з шарфом 🧥"
            elif temp < 18:
                clothes = "Погода комфортна: куртка, светр або тренч будуть якраз доречними ✨"
            else:
                clothes = "На вулиці тепло! Можна сміливо вдягати легкий одяг ☀️"

            umbrella = " Обов'язково візьми парасольку — висока ймовірність дощу ☂️" if is_rain else " Дощу не очікується, парасолька не знадобиться."
            wind_tip = f" Вітряно ({wind:.0f} км/год), застібайся щільніше 💨" if wind > 20 else ""

            text = (
                f"☀️ <b>Доброго ранку, Настю!</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"За вікном зараз: <b>{temp:+.1f}°C</b>, {desc}.\n"
                f"{clothes}{umbrella}{wind_tip}\n\n"
                f"Продуктивного та гарного тобі дня!"
            )
            return text
        except Exception as e:
            logger.warning("Morning brief generation error: %s", e)
            return "☀️ Доброго ранку, Настю! Продуктивного та гарного тобі дня!"


nastya_routine_service = NastyaRoutineService()
