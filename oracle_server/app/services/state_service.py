import time
from collections import deque
from typing import Dict, Any, Optional, Deque

from app.core.config import settings
from app.core.events import realtime_hub
from app.core.logger import logger
from app.domain.schemas import BoxTelemetryPayload, CommandPayload
from app.services.database_service import db_service

MODES_UA = (
    "☀️ Теплий затишок",
    "🕯 Жива свічка",
    "🍯 Бурштинова свічка",
    "🔥 Камін з іскрами",
    "🌅 Захід сонця",
    "🌸 Сакура Неон",
    "🌌 Північне сяйво",
    "🌊 Океанський бриз",
    "⚡ Кібер-комета",
    "🌈 Веселка",
    "🌑 Вимкнено",
    "🎵 Spotify AmbiLight",
)


class StateService:
    __slots__ = (
        "last_seen",
        "latest_telemetry",
        "last_history_sample",
        "last_health_sample",
        "last_known_rst",
        "command_queue",
        "admin_id",
        "girl_id",
        "detected_base_url",
        "ai_persona_name",
        "ai_system_prompt",
        "ai_temperature",
    )

    DEFAULT_AI_PROMPT = (
        "Ти — теплий, дотепний і турботливий дух розумної шкатулки MagicBox. "
        "Пиши українською мовою, лаконічно (2-3 речення), живо і з гумором. "
        "Аналізуй погоду за вікном та мікроклімат у кімнаті, давай приємні поради."
    )

    def __init__(self) -> None:
        self.last_seen: float = 0.0
        self.latest_telemetry: Dict[str, Any] = {}
        self.last_history_sample: float = 0.0
        self.last_health_sample: float = 0.0
        self.last_known_rst: str = ""
        self.command_queue: Deque[Dict[str, Any]] = deque(maxlen=32)

        self.admin_id: str = db_service.get_setting("admin_id", settings.TG_ADMIN_ID) or settings.TG_ADMIN_ID
        self.girl_id: str = db_service.get_setting("girl_id", settings.TG_GIRL_ID) or settings.TG_GIRL_ID
        self.detected_base_url: str = db_service.get_setting("detected_base_url", "") or ""

        self.ai_persona_name: str = db_service.get_setting("ai_persona_name", "Дух MagicBox")
        self.ai_system_prompt: str = db_service.get_setting("ai_system_prompt", self.DEFAULT_AI_PROMPT)
        self.ai_temperature: float = float(db_service.get_setting("ai_temperature", 0.75))

    def save_auth_ids(self, admin_id: Optional[str] = None, girl_id: Optional[str] = None) -> None:
        if admin_id is not None:
            self.admin_id = admin_id
            db_service.set_setting("admin_id", admin_id)
        if girl_id is not None:
            self.girl_id = girl_id
            db_service.set_setting("girl_id", girl_id)

    def save_ai_persona(self, name: str, prompt: str, temp: float = 0.75) -> None:
        self.ai_persona_name = name.strip() or "Дух MagicBox"
        self.ai_system_prompt = prompt.strip() or self.DEFAULT_AI_PROMPT
        self.ai_temperature = round(float(temp), 2)
        db_service.set_setting("ai_persona_name", self.ai_persona_name)
        db_service.set_setting("ai_system_prompt", self.ai_system_prompt)
        db_service.set_setting("ai_temperature", self.ai_temperature)

    def remember_request_base_url(self, base_url: str) -> None:
        clean = base_url.rstrip("/")
        if clean and "127.0.0.1" not in clean and "localhost" not in clean and clean != self.detected_base_url:
            self.detected_base_url = clean
            db_service.set_setting("detected_base_url", clean)

    def resolve_public_url(self, preferred: str = "") -> str:
        cand = (preferred or settings.PUBLIC_SERVER_URL).rstrip("/")
        if cand and "127.0.0.1" not in cand and "localhost" not in cand and "YOUR_ORACLE_IP" not in cand:
            return cand
        return self.detected_base_url or cand

    def update_telemetry(self, payload: BoxTelemetryPayload) -> float:
        now = time.time()
        prev_seen = self.last_seen
        self.last_seen = now

        dump = payload.model_dump(exclude={"ack", "ping"})
        dump["m_name"] = MODES_UA[payload.m % len(MODES_UA)]
        self.latest_telemetry = dump

        # Записуємо зріз клімату в SQLite WAL кожні 5 хвилин
        if now - self.last_history_sample >= 300.0:
            self.last_history_sample = now
            db_service.add_climate_sample(payload.t, payload.h, payload.p, payload.l)

        # Фіксація апаратного здоров'я плати кожні 15 хвилин
        if now - self.last_health_sample >= 900.0:
            self.last_health_sample = now
            db_service.record_box_health(
                rssi=payload.rssi,
                heap=payload.heap,
                psram=payload.psram_free,
                uptime=0,
                reset_reason=0,
            )

        # Аудит перезавантажень плати (ESP32 Reset Reason)
        rst = (payload.rst or "").strip()
        if rst and rst != "UNKNOWN" and rst != self.last_known_rst:
            self.last_known_rst = rst
            db_service.log_event("esp32", "boot", "reset", {
                "reason": rst,
                "fw": payload.fw,
                "heap": payload.heap,
                "psram": payload.psram_free,
                "rssi": payload.rssi,
            })

        return prev_seen

    def enqueue_command(self, cmd: CommandPayload) -> int:
        data = cmd.model_dump(exclude_none=True)
        if data:
            self.enqueue_raw(data)
        return len(self.command_queue)

    def enqueue_raw(self, data: Dict[str, Any]) -> int:
        """
        Розумна черга з контролем TTL:
        - Зливає однотипні команди налаштувань світла/яскравості.
        - Автоматично призначає TTL (для звуків/повідомлень: 180с, для світла: 86400с).
        """
        now = time.time()
        if "created_at" not in data:
            data["created_at"] = now
        if "ttl_sec" not in data:
            # Для звуків, повідомлень і реакцій TTL = 180 секунд (захист від нічного раптового звуку)
            if any(k in data for k in ("play_url", "msg", "react")):
                data["ttl_sec"] = 180.0
            else:
                data["ttl_sec"] = 86400.0

        if self.command_queue:
            last = self.command_queue[-1]
            if not any(k in data for k in ("msg", "media_url", "play_url", "ota_url", "stop_media")):
                if not any(k in last for k in ("msg", "media_url", "play_url", "ota_url", "stop_media")):
                    last.update(data)
                    realtime_hub.notify_new_command()
                    return len(self.command_queue)

        self.command_queue.append(data)
        realtime_hub.notify_new_command()
        return len(self.command_queue)

    def pop_next_sync_response(self) -> Dict[str, Any]:
        """
        Витягує наступну активну команду.
        Автоматично відкидає прострочені команди (Anti-Zombie Guard).
        """
        bot_active = settings.SERVER_MANAGES_BOT and len(settings.TG_BOT_TOKEN) > 15
        now = time.time()

        while self.command_queue:
            cmd = dict(self.command_queue.popleft())
            created = cmd.get("created_at", now)
            ttl = cmd.get("ttl_sec", 86400.0)

            if (now - created) > ttl:
                logger.info(
                    "Anti-Zombie: пропущено застарілу команду (вік: %.1fс > TTL: %.1fс). Msg: '%s'",
                    now - created,
                    ttl,
                    cmd.get("msg") or cmd.get("play_url") or "settings",
                )
                continue

            # Очищуємо внутрішні службові поля перед відправкою на ESP32-S3
            cmd.pop("created_at", None)
            cmd.pop("ttl_sec", None)
            cmd["bot_managed"] = bot_active
            cmd["has_cmd"] = True
            cmd["has_more"] = len(self.command_queue) > 0
            return cmd

        return {"bot_managed": bot_active, "has_cmd": False, "has_more": False}

    def is_online(self) -> bool:
        return self.last_seen > 0 and (time.time() - self.last_seen) < settings.BLACKOUT_TIMEOUT_SEC

    def seconds_since_seen(self) -> Optional[float]:
        if self.last_seen <= 0:
            return None
        return round(time.time() - self.last_seen, 1)


state_service = StateService()
