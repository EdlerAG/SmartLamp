# Файл: oracle_server/app/core/events.py
import asyncio
from typing import Dict, Any, Set, Optional
from fastapi import WebSocket
import orjson


class RealtimeHub:
    """
    Подієвий хаб реального часу з 0.00% споживанням CPU у простої:
    - WebSocket-канал для ESP32-S3 (/ws/box) та Long-Poll очікування через asyncio.Event.
    - Двостороння WebSocket-трансляція для відкритих вкладок Web-Студії (/ws/dashboard).
    """

    __slots__ = ("_box_ws", "_dashboard_clients", "_cmd_wakeup_event")

    def __init__(self) -> None:
        self._box_ws: Optional[WebSocket] = None
        self._dashboard_clients: Set[WebSocket] = set()
        self._cmd_wakeup_event: asyncio.Event = asyncio.Event()

    @property
    def is_box_ws_connected(self) -> bool:
        return self._box_ws is not None

    def notify_new_command(self) -> None:
        """Миттєво пробуджує очікуючий Long-Poll запит лампи (латентність < 5 мс)."""
        self._cmd_wakeup_event.set()

    async def wait_for_command(self, timeout: float = 0.0) -> bool:
        if timeout <= 0:
            return False
        self._cmd_wakeup_event.clear()
        try:
            await asyncio.wait_for(self._cmd_wakeup_event.wait(), timeout=timeout)
            self._cmd_wakeup_event.clear()
            return True
        except asyncio.TimeoutError:
            return False

    async def register_box_ws(self, ws: WebSocket) -> None:
        await ws.accept()
        self._box_ws = ws

    def unregister_box_ws(self, ws: WebSocket) -> None:
        if self._box_ws is ws:
            self._box_ws = None

    async def push_to_box(self, payload: Dict[str, Any]) -> bool:
        self.notify_new_command()
        if self._box_ws is None:
            return False
        try:
            await self._box_ws.send_text(orjson.dumps(payload).decode("utf-8"))
            return True
        except Exception:
            self._box_ws = None
            return False

    async def register_dashboard_ws(self, ws: WebSocket) -> None:
        await ws.accept()
        self._dashboard_clients.add(ws)

    def unregister_dashboard_ws(self, ws: WebSocket) -> None:
        self._dashboard_clients.discard(ws)



    def emit_to_dashboards(self, data: Dict[str, Any]) -> None:
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self.broadcast_to_dashboards(data))
        except RuntimeError:
            pass

    async def broadcast_to_dashboards(self, data: Dict[str, Any]) -> None:
        # 1. Трансляція для WebSocket
        if self._dashboard_clients:
            msg = orjson.dumps(data).decode("utf-8")
            dead = []
            for ws in self._dashboard_clients:
                try:
                    await ws.send_text(msg)
                except Exception:
                    dead.append(ws)
            for ws in dead:
                self._dashboard_clients.discard(ws)

        # 2. Трансляція для SSE підписників
        self.broadcast_sse("state", data)


realtime_hub = RealtimeHub()
