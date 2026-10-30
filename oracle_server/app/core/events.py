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

    __slots__ = ("_box_ws", "_dashboard_clients", "_cmd_wakeup_event", "_box_lock", "_spotify_poll_task")

    def __init__(self) -> None:
        self._box_ws: Optional[WebSocket] = None
        self._dashboard_clients: Set[WebSocket] = set()
        self._cmd_wakeup_event: asyncio.Event = asyncio.Event()
        self._box_lock: asyncio.Lock = asyncio.Lock()
        self._spotify_poll_task: Optional[asyncio.Task] = None

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
        async with self._box_lock:
            if self._box_ws is None:
                return False
            try:
                await self._box_ws.send_text(orjson.dumps(payload).decode("utf-8"))
                return True
            except Exception:
                self._box_ws = None
                return False

    async def send_bytes_to_box(self, data: bytes) -> bool:
        if self._box_ws is None or not data:
            return False
        async with self._box_lock:
            if self._box_ws is None:
                return False
            try:
                await self._box_ws.send_bytes(data)
                return True
            except Exception:
                self._box_ws = None
                return False

    async def register_dashboard_ws(self, ws: WebSocket) -> None:
        await ws.accept()
        self._dashboard_clients.add(ws)
        if self._spotify_poll_task is None or self._spotify_poll_task.done():
            self._spotify_poll_task = asyncio.create_task(self._demand_spotify_worker())

    def unregister_dashboard_ws(self, ws: WebSocket) -> None:
        self._dashboard_clients.discard(ws)

    async def _demand_spotify_worker(self) -> None:
        """
        Demand-driven воркер: активний ВИКЛЮЧНО коли є відкриті клієнти Студії / Telegram.
        Опитує Spotify та транслює свіжий трек у відкриті вкладки при зміні або кожні 15с.
        Коли клієнтів 0 — завершується (0.00% CPU).
        """
        import time
        from app.services.spotify_service import spotify_service

        last_track_id = ""
        last_playing = None
        last_lyric = ""
        last_push_time = 0.0

        try:
            while self._dashboard_clients:
                if spotify_service.is_linked():
                    try:
                        cur_st = await spotify_service.fetch_current_playback()
                        now = time.time()
                        cur_track = cur_st.get("track_id", "")
                        cur_playing = cur_st.get("playing", False)
                        cur_lyric = cur_st.get("lyric", "")

                        needs_push = (
                            cur_track != last_track_id
                            or cur_playing != last_playing
                            or cur_lyric != last_lyric
                            or (now - last_push_time > 15.0)
                        )

                        if needs_push:
                            last_track_id = cur_track
                            last_playing = cur_playing
                            last_lyric = cur_lyric
                            last_push_time = now
                            await self.broadcast_to_dashboards({"type": "spotify", "spotify": cur_st})
                    except Exception:
                        pass
                await asyncio.sleep(2.5)
        except asyncio.CancelledError:
            pass
        finally:
            self._spotify_poll_task = None



    def emit_to_box_immediate(self) -> None:
        """Миттєвий пуш команд у WebSocket лампи при будь-якій дії (з Telegram-бота, розкладу чи API)."""
        if self._box_ws is None:
            return
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self._async_push_pending_to_box())
        except RuntimeError:
            pass

    async def _async_push_pending_to_box(self) -> None:
        if self._box_ws is None:
            return
        from app.services.state_service import state_service
        next_cmd = state_service.pop_next_sync_response()
        if next_cmd.get("has_cmd") or next_cmd.get("spotify") or next_cmd.get("cal_event"):
            await self.push_to_box(next_cmd)

    def emit_spotify_to_box(self, spotify_state: Dict[str, Any]) -> None:
        if self._box_ws is None:
            return
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self.push_to_box({
                "bot_managed": True,
                "has_cmd": False,
                "has_more": False,
                "sp_linked": True,
                "spotify": spotify_state,
            }))
        except RuntimeError:
            pass

    def emit_calendar_to_box(self, cal_event: Optional[Dict[str, Any]]) -> None:
        if self._box_ws is None:
            return
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self.push_to_box({
                "bot_managed": True,
                "has_cmd": False,
                "has_more": False,
                "cal_event": cal_event,
            }))
        except RuntimeError:
            pass

    def emit_vinyl_to_box(self, vinyl_data: bytes) -> None:
        if self._box_ws is None or not vinyl_data:
            return
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self.send_bytes_to_box(vinyl_data))
        except Exception:
            pass

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




realtime_hub = RealtimeHub()
