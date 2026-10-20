# Файл: oracle_server/app/api/endpoints_box.py
import os
import re
from fastapi import APIRouter, Depends, HTTPException, Request, WebSocket, WebSocketDisconnect, Query
from fastapi.responses import FileResponse
import orjson

from app.core.security import verify_access
from app.core.config import settings
from app.core.events import realtime_hub
from app.domain.schemas import BoxTelemetryPayload
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.media_service import media_service
from app.services.ota_service import ota_service

router = APIRouter(tags=["ESP32-S3 Box Gateway v5.0"])


@router.post("/api/v5/box/sync")
@router.post("/api/v1/box/sync")
async def sync_box(
    request: Request,
    payload: BoxTelemetryPayload,
    wait: float = Query(0.0, ge=0.0, le=25.0, description="Час Long-Poll очікування миттєвої команди (сек)"),
    _: str = Depends(verify_access),
):
    state_service.remember_request_base_url(str(request.base_url))
    prev_seen = state_service.update_telemetry(payload)
    await blackout_service.on_box_ping(prev_seen, ack=bool(payload.ack), ping_type=payload.ping or 0)

    # Якщо черга порожня, але лампа просить Long-Poll очікування — спимо з 0% CPU до моменту кліку користувача!
    if not state_service.command_queue and wait > 0:
        await realtime_hub.wait_for_command(timeout=wait)

    return state_service.pop_next_sync_response()


@router.websocket("/ws/box")
async def box_websocket_endpoint(ws: WebSocket):
    """Постійний двосторонній WebSocket-канал для ESP32-S3 (латентність 15 мс)."""
    key = ws.headers.get("x-device-key") or ws.query_params.get("key") or ""
    if key != settings.API_KEY:
        await ws.close(code=1008)
        return

    await realtime_hub.register_box_ws(ws)
    try:
        while True:
            raw_msg = await ws.receive_text()
            data = orjson.loads(raw_msg)
            payload = BoxTelemetryPayload(**data)
            prev_seen = state_service.update_telemetry(payload)
            await blackout_service.on_box_ping(prev_seen, ack=bool(payload.ack), ping_type=payload.ping or 0)

            if state_service.command_queue:
                await ws.send_text(orjson.dumps(state_service.pop_next_sync_response()).decode("utf-8"))
    except WebSocketDisconnect:
        pass
    except Exception:
        pass
    finally:
        realtime_hub.unregister_box_ws(ws)


@router.get("/api/v5/media/live.mbx")
async def download_live_mbx():
    """Віддає активну мультикадрову анімацію .mbx із RAM-диска (/dev/shm)."""
    if not os.path.exists(media_service.RAM_ANIM_PATH):
        raise HTTPException(status_code=404, detail="Active animation not found")
    return FileResponse(media_service.RAM_ANIM_PATH, media_type="application/octet-stream", filename="live.mbx")


@router.get("/api/v5/media/{name}.mbx")
async def download_named_mbx(name: str):
    """Віддає збережену кастомну гіфку/наліпку з каталогу."""
    safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:28].lower()
    info = db_service.get_media_by_name(safe_name)
    if info and os.path.exists(info["file_path"]):
        return FileResponse(info["file_path"], media_type="application/octet-stream", filename=f"{safe_name}.mbx")
    if os.path.exists(media_service.RAM_ANIM_PATH):
        return FileResponse(media_service.RAM_ANIM_PATH, media_type="application/octet-stream", filename="live.mbx")
    raise HTTPException(status_code=404, detail="MBX media not found")


@router.get("/api/v1/media/frame.rgb565")
async def download_rgb565_frame():
    """Віддає 115200 байт кадру RGB565 із RAM-диска (/dev/shm)."""
    if not os.path.exists(media_service.RAM_FRAME_PATH):
        raise HTTPException(status_code=404, detail="Frame not found")
    return FileResponse(media_service.RAM_FRAME_PATH, media_type="application/octet-stream")


@router.get("/api/v5/ram/{filename}")
async def download_ram_audio(filename: str):
    """Миттєва роздача TTS та голосових повідомлень із RAM-диска (/dev/shm)."""
    safe_fname = re.sub(r"[^a-zA-Z0-9_.-]", "", filename)[:32]
    path = os.path.join(settings.RAM_MEDIA_DIR, safe_fname)
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="RAM audio stream not found")
    m_type = "audio/ogg" if safe_fname.endswith(".ogg") else "audio/mpeg"
    return FileResponse(path, media_type=m_type, filename=safe_fname)


@router.get("/api/v1/media/sound/{name}.mp3")
async def download_custom_sound(name: str):
    safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24].lower()
    path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Sound not found")
    return FileResponse(path, media_type="audio/mpeg", filename=f"{safe_name}.mp3")


@router.get("/api/v1/ota/latest")
async def get_latest_ota_metadata(request: Request):
    return ota_service.get_metadata(str(request.base_url))


@router.get("/api/v1/ota/firmware.bin")
async def download_firmware():
    if not os.path.exists(settings.FIRMWARE_PATH):
        raise HTTPException(status_code=404, detail="Firmware binary not found")
    return FileResponse(settings.FIRMWARE_PATH, media_type="application/octet-stream", filename="firmware.bin")