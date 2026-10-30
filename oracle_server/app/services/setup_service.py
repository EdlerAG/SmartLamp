# Файл: oracle_server/app/services/setup_service.py
import time
from typing import Dict, Any, Optional

from app.core.logger import logger
from app.services.database_service import db_service
from app.services.spotify_service import spotify_service
from app.services.google_calendar_service import google_calendar_service


class SetupService:
    """
    Центральний сервіс першого налаштування (Onboarding Engine):
    - Розрахунок компактної бітової маски стану для ESP32 ('su').
    - Відстеження прогресу майстра налаштування в боті та веб-порталі.
    - Управління темами настрою (Mood Sync).
    - Тактовний графік нагадувань про опціональні кроки (Spotify / Календар).
    """

    __slots__ = ()

    # Бітові маски кроків налаштування (для передачі в ESP32)
    BIT_WIFI_OK = 1 << 0       # 1: Wi-Fi на зв'язку
    BIT_OWNER_PAIRED = 1 << 1  # 2: Власниця прив'язала бота (/start c_<secret>)
    BIT_MOOD_SET = 1 << 2      # 4: Обрано тему / настрій
    BIT_SPOTIFY_OK = 1 << 3    # 8: Spotify авторизовано
    BIT_CALENDAR_OK = 1 << 4   # 16: Google Calendar підключено
    BIT_QUIET_HOURS = 1 << 5   # 32: Нічний режим налаштовано
    BIT_PORTAL_TOUR = 1 << 6   # 64: Ознайомлення з веб-панеллю завершено

    def get_setup_bitmask(self) -> int:
        """
        Обчислює компактну бітову маску завершених кроків.
        Використовується в пакеті синхронізації з платою (поле 'su').
        """
        steps = db_service.get_setup_state()
        mask = self.BIT_WIFI_OK  # Сервер відповідає -> Wi-Fi та мережа активні

        if steps.get("pairing", {}).get("status") == "done" or db_service.is_device_claimed():
            mask |= self.BIT_OWNER_PAIRED

        if steps.get("mood", {}).get("status") == "done":
            mask |= self.BIT_MOOD_SET

        if steps.get("spotify", {}).get("status") == "done" or spotify_service.is_linked():
            mask |= self.BIT_SPOTIFY_OK

        if steps.get("calendar", {}).get("status") == "done" or google_calendar_service.is_linked():
            mask |= self.BIT_CALENDAR_OK

        if steps.get("quiet_hours", {}).get("status") == "done":
            mask |= self.BIT_QUIET_HOURS

        if steps.get("portal_tour", {}).get("status") == "done":
            mask |= self.BIT_PORTAL_TOUR

        return mask

    def get_full_status(self) -> Dict[str, Any]:
        """Повертає повний JSON-стан майстра налаштування для веб-панелі та бота."""
        steps = db_service.get_setup_state()
        sp_linked = spotify_service.is_linked()
        cal_linked = google_calendar_service.is_linked()

        # Автоматично синхронізуємо статус кроків зі справжніми сервісами
        if sp_linked and steps.get("spotify", {}).get("status") != "done":
            db_service.update_setup_step("spotify", "done")
            steps["spotify"] = {"status": "done", "completed_at": time.time(), "updated_at": time.time()}

        if cal_linked and steps.get("calendar", {}).get("status") != "done":
            db_service.update_setup_step("calendar", "done")
            steps["calendar"] = {"status": "done", "completed_at": time.time(), "updated_at": time.time()}

        return {
            "bitmask": self.get_setup_bitmask(),
            "claimed": db_service.is_device_claimed(),
            "owner_chat_id": db_service.get_owner_chat_id(),
            "theme_mood": db_service.get_theme_mood(),
            "steps": steps,
            "spotify_linked": sp_linked,
            "calendar_linked": cal_linked,
            "viewer_alerts_enabled": db_service.get_viewer_alerts_enabled(),
        }

    def record_claim(self, secret: str, chat_id: str) -> bool:
        """Реєстрація секрету прив'язки власниці."""
        return db_service.claim_device(secret, chat_id)

    def set_mood(self, mood: str) -> str:
        """Збереження вибраної теми настрою."""
        return db_service.set_theme_mood(mood)

    def advance_step(self, step_id: str, status: str) -> None:
        """Перехід етапу (done, skipped, dismissed, pending)."""
        valid_statuses = {"pending", "done", "skipped", "dismissed"}
        if status in valid_statuses:
            db_service.update_setup_step(step_id, status)

    def is_reminder_due(self, step_id: str) -> bool:
        """
        Перевіряє, чи настав час для делікатного нагадування про пропущений крок.
        - Лише якщо пристрій прив'язаний.
        - Крок у статусі 'pending' або 'skipped' (не 'done' чи 'dismissed').
        - Не більше 3 нагадувань на фічу.
        - Інтервал між нагадуваннями не менше 24 годин.
        """
        if not db_service.is_device_claimed():
            return False

        steps = db_service.get_setup_state()
        st = steps.get(step_id, {}).get("status", "pending")
        if st in ("done", "dismissed"):
            return False

        # Перевірка лічильника нагадувань
        count_key = f"reminder_count_{step_id}"
        ts_key = f"reminder_ts_{step_id}"

        count = int(db_service.get_setting(count_key, 0))
        if count >= 3:
            # Більше не турбуємо
            db_service.update_setup_step(step_id, "dismissed")
            return False

        last_ts = float(db_service.get_setting(ts_key, 0.0))
        now = time.time()
        # Мінімум 24 години (86400 сек) між спробами
        if now - last_ts < 86400.0:
            return False

        return True

    def mark_reminder_sent(self, step_id: str) -> None:
        """Фіксує відправку нагадування."""
        count_key = f"reminder_count_{step_id}"
        ts_key = f"reminder_ts_{step_id}"
        count = int(db_service.get_setting(count_key, 0)) + 1
        db_service.set_setting(count_key, count)
        db_service.set_setting(ts_key, time.time())


setup_service = SetupService()
