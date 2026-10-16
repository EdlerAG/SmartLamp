import os
import time
from collections import deque
from typing import Dict, Any, Optional, Deque, List
import orjson
from app.models.schemas import BoxStatusPayload, QueueCommandRequest
from app.core.config import settings

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
)


class StateService:
    __slots__ = (
        "last_seen",
        "latest_telemetry",
        "climate_history",
        "last_history_sample",
        "command_queue",
        "admin_id",
        "girl_id",
        "outages",
    )

    def __init__(self) -> None:
        self.last_seen: float = 0.0
        self.latest_telemetry: Dict[str, Any] = {}
        self.climate_history: Deque[Dict[str, Any]] = deque(maxlen=288)
        self.last_history_sample: float = 0.0
        self.command_queue: Deque[Dict[str, Any]] = deque(maxlen=32)
        self.admin_id: str = settings.TG_ADMIN_ID
        self.girl_id: str = settings.TG_GIRL_ID
        self.outages: Deque[Dict[str, Any]] = deque(maxlen=50)
        self.load_snapshot()

    def load_snapshot(self) -> None:
        if not os.path.exists(settings.STATE_FILE):
            return
        try:
            with open(settings.STATE_FILE, "rb") as f:
                data = orjson.loads(f.read())
            self.latest_telemetry = data.get("latest_telemetry", {})
            self.admin_id = data.get("admin_id") or settings.TG_ADMIN_ID
            self.girl_id = data.get("girl_id") or settings.TG_GIRL_ID
            for item in data.get("climate_history", []):
                self.climate_history.append(item)
            for item in data.get("outages", []):
                self.outages.append(item)
            for cmd in data.get("command_queue", []):
                self.command_queue.append(cmd)
        except Exception:
            pass

    def save_snapshot(self) -> None:
        try:
            payload = {
                "admin_id": self.admin_id,
                "girl_id": self.girl_id,
                "latest_telemetry": self.latest_telemetry,
                "climate_history": list(self.climate_history),
                "outages": list(self.outages),
                "command_queue": list(self.command_queue),
            }
            tmp_path = settings.STATE_FILE + ".tmp"
            with open(tmp_path, "wb") as f:
                f.write(orjson.dumps(payload))
            os.replace(tmp_path, settings.STATE_FILE)
        except Exception:
            pass

    def update_telemetry(self, payload: BoxStatusPayload) -> float:
        now = time.time()
        prev_seen = self.last_seen
        self.last_seen = now
        self.latest_telemetry = payload.model_dump(exclude={"ack", "ping"})

        if now - self.last_history_sample >= 300.0:
            self.last_history_sample = now
            self.climate_history.append({
                "ts": int(now),
                "t": round(payload.t, 1),
                "h": round(payload.h, 0),
                "p": round(payload.p, 0),
                "l": payload.l,
            })
            self.save_snapshot()
        return prev_seen

    def enqueue_command(self, cmd: QueueCommandRequest) -> int:
        data = cmd.model_dump(exclude_none=True)
        if data:
            self.enqueue_raw(data)
        return len(self.command_queue)

    def enqueue_raw(self, data: Dict[str, Any]) -> int:
        if "set_mode" in data and len(data) == 1:
            for item in self.command_queue:
                if "set_mode" in item and len(item) == 1:
                    item["set_mode"] = data["set_mode"]
                    return len(self.command_queue)
        self.command_queue.append(data)
        return len(self.command_queue)

    def pop_next_sync_response(self) -> Dict[str, Any]:
        bot_active = settings.SERVER_MANAGES_BOT and len(settings.TG_BOT_TOKEN) > 15
        if not self.command_queue:
            return {"bot_managed": bot_active, "has_cmd": False}

        cmd = dict(self.command_queue.popleft())
        cmd["bot_managed"] = bot_active
        cmd["has_cmd"] = True
        return cmd

    def is_online(self) -> bool:
        return self.last_seen > 0 and (time.time() - self.last_seen) < settings.BLACKOUT_TIMEOUT_SEC

    def seconds_since_seen(self) -> Optional[float]:
        if self.last_seen <= 0:
            return None
        return round(time.time() - self.last_seen, 1)


state_service = StateService()