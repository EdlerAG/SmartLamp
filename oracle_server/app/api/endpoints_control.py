from fastapi import APIRouter, Depends, UploadFile, File, Request
from app.core.security import verify_device_key
from app.models.schemas import QueueCommandRequest, AiBriefingRequest
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.ota_service import ota_service
from app.services.media_service import media_service
from app.services.ai_service import ai_service
from app.services.audio_service import audio_service
from app.services.weather_service import weather_service
from app.services.storage_service import storage_manager

router = APIRouter(tags=["Remote Control & Cloud Services"])


@router.get("/api/v1/box/state")
@router.get("/api/telemetry")
async def get_cloud_state(_: str = Depends(verify_device_key)):
    return {
        "is_online": state_service.is_online(),
        "seconds_since_last_ping": state_service.seconds_since_seen(),
        "pending_commands": len(state_service.command_queue),
        "telemetry": state_service.latest_telemetry,
        "storage": storage_manager.get_storage_stats(),
    }


@router.get("/api/v1/weather")
async def get_weather_and_barometer(_: str = Depends(verify_device_key)):
    return await weather_service.get_combined_weather()


@router.post("/api/v1/ai/briefing")
async def trigger_ai_briefing(req: AiBriefingRequest, speak: bool = False, _: str = Depends(verify_device_key)):
    return await ai_service.generate_smart_briefing(req.custom_note or "", speak=speak)


@router.post("/api/v1/box/command")
@router.post("/api/commands")
async def enqueue_command(cmd: QueueCommandRequest, _: str = Depends(verify_device_key)):
    queued = state_service.enqueue_command(cmd)
    return {"ok": True, "queued": queued}


@router.get("/api/blackout")
@router.get("/api/v1/box/blackout")
async def get_blackout_info(_: str = Depends(verify_device_key)):
    return blackout_service.get_status()


@router.post("/api/v1/ota/upload")
async def upload_ota_binary(
    request: Request,
    file: UploadFile = File(...),
    _: str = Depends(verify_device_key),
):
    return await ota_service.save_and_queue_firmware(file, str(request.base_url))


@router.post("/api/v1/media/upload-image")
async def upload_display_image(
    request: Request,
    save_name: str = "",
    file: UploadFile = File(...),
    _: str = Depends(verify_device_key),
):
    size = await media_service.convert_image_to_rgb565(file, str(request.base_url), save_name=save_name)
    return {"ok": True, "rgb565_bytes": size, "saved_as": save_name or "latest_frame"}


@router.post("/api/v1/media/sticker/{name}")
async def show_saved_sticker(name: str, request: Request, _: str = Depends(verify_device_key)):
    return media_service.trigger_saved_sticker(name, str(request.base_url))


@router.post("/api/v1/audio/upload-sound")
async def upload_custom_sound(
    name: str,
    file: UploadFile = File(...),
    _: str = Depends(verify_device_key),
):
    return await audio_service.save_custom_sound(name, file)


@router.post("/api/v1/audio/tts")
async def trigger_tts_speech(
    text: str,
    request: Request,
    _: str = Depends(verify_device_key),
):
    return await audio_service.speak_text(text, str(request.base_url))