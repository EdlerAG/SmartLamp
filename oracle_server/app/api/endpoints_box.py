import os
import re
from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import FileResponse
from app.core.security import verify_device_key
from app.core.config import settings
from app.models.schemas import BoxStatusPayload
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.media_service import media_service
from app.services.audio_service import audio_service

router = APIRouter(tags=["ESP32 Box Gateway"])


@router.post("/api/v1/box/sync")
@router.post("/api/telemetry")
async def sync_box(payload: BoxStatusPayload, _: str = Depends(verify_device_key)):
    prev_seen = state_service.update_telemetry(payload)
    await blackout_service.on_box_ping(prev_seen, ack=bool(payload.ack), ping_type=payload.ping or 0)
    return state_service.pop_next_sync_response()


@router.get("/api/v1/ota/firmware.bin")
@router.get("/api/ota/latest/binary")
async def download_firmware():
    if not os.path.exists(settings.FIRMWARE_PATH):
        raise HTTPException(status_code=404, detail="Firmware binary not found")
    return FileResponse(settings.FIRMWARE_PATH, media_type="application/octet-stream", filename="firmware.bin")


@router.get("/api/v1/media/frame.rgb565")
async def download_rgb565_frame():
    if not os.path.exists(media_service.FRAME_PATH):
        raise HTTPException(status_code=404, detail="Frame not found")
    return FileResponse(media_service.FRAME_PATH, media_type="application/octet-stream")


@router.get("/api/v1/media/tts.mp3")
async def download_tts_mp3():
    if not os.path.exists(audio_service.TTS_FILE):
        raise HTTPException(status_code=404, detail="TTS audio not found")
    return FileResponse(audio_service.TTS_FILE, media_type="audio/mpeg", filename="tts.mp3")


@router.get("/api/v1/media/sound/{name}.mp3")
async def download_custom_sound(name: str):
    safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24]
    path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Sound not found")
    return FileResponse(path, media_type="audio/mpeg", filename=f"{safe_name}.mp3")