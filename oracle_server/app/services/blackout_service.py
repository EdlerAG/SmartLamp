import asyncio
import time
from typing import Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.state_service import state_service


class BlackoutService:
    __slots__ = ("in_blackout", "blackout_started_at")

    def __init__(self) -> None:
        self.in_blackout: bool = False
        self.blackout_started_at: float = 0.0

    async def send_tg_alert(self, text: str, chat_id: str = "") -> None:
        target = chat_id or state_service.admin_id
        if not settings.TG_BOT_TOKEN or not target:
            return
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/sendMessage"
        try:
            await http_pool.get().post(url, json={"chat_id": target, "text": text}, timeout=6.0)
        except Exception:
            pass

    async def on_box_ping(self, prev_seen: float, ack: bool = False, ping_type: int = 0) -> None:
        now = time.time()

        if self.in_blackout and self.blackout_started_at > 0:
            duration = round(now - self.blackout_started_at, 1)
            state_service.outages.append({
                "disconnected_at": round(self.blackout_started_at, 1),
                "restored_at": round(now, 1),
                "duration_sec": duration,
            })
            self.in_blackout = False
            state_service.save_snapshot()
            mins = max(1, int(duration // 60))
            await self.send_tg_alert(
                f"⚡ Світло та домашній Wi-Fi увімкнули! Шкатулка онлайн.\n"
                f"⏱ Тривалість відключення: ~{mins} хв ({int(duration)} с)."
            )

        if ack:
            await self.send_tg_alert("✅ Настя торкнулася кришки шкатулки — твоє повідомлення прочитано! 🔥")

        if ping_type > 0:
            ping_names = {
                2: "🔥 Настя натиснула кнопку на шкатулці і кинула тобі ВОГНИК!",
                3: "⚡ Настя натиснула кнопку на шкатулці і бахнула БЛИСКАВКОЮ!",
                4: "🐱 Настя натиснула кнопку на шкатулці: МЕМНИЙ КУСЬ (=^._.^=)!",
                5: "👽 Настя натиснула кнопку на шкатулці: «Пороблено! UFO Beam»!",
                6: "👑 Настя передає тобі КОРОЛІВСЬКИЙ ПРИВІТ зі шкатулки!",
            }
            await self.send_tg_alert(ping_names.get(ping_type, "✨ Сигнал зі шкатулки!"))

    async def watchdog_loop(self) -> None:
        while True:
            await asyncio.sleep(5.0)
            if state_service.last_seen <= 0:
                continue
            age = time.time() - state_service.last_seen
            if age >= settings.BLACKOUT_TIMEOUT_SEC and not self.in_blackout:
                self.in_blackout = True
                self.blackout_started_at = state_service.last_seen
                state_service.save_snapshot()
                last_t = state_service.latest_telemetry.get("t", "--")
                last_h = state_service.latest_telemetry.get("h", "--")
                await self.send_tg_alert(
                    f"🔌 Зник зв'язок зі шкатулкою MagicBox ({int(age)} с).\n"
                    f"Ймовірно, вимкнули світло. Усі нові повідомлення та реакції будуть збережені в чергу!\n"
                    f"🌡 Останні показники в кімнаті: {last_t}°C, {last_h}%."
                )

    def get_status(self) -> Dict[str, Any]:
        age = state_service.seconds_since_seen()
        return {
            "in_blackout": self.in_blackout,
            "offline_seconds": age if self.in_blackout else 0.0,
            "recent_outages": list(state_service.outages),
        }


blackout_service = BlackoutService()