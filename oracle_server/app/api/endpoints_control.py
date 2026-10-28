# Файл: oracle_server/app/api/endpoints_control.py
import os
import secrets
from fastapi import APIRouter, Depends, UploadFile, File, Request, Response, HTTPException, Query
from fastapi.responses import HTMLResponse, FileResponse

from app.core.config import settings
from app.core.security import (
    verify_access,
    verify_admin_access,
    check_login_rate_limit,
    record_login_failure,
    clear_login_failures,
)
from app.core.memory_guard import memory_guard
from app.domain.schemas import (
    CommandPayload,
    AiPersonaConfigRequest,
    AiAskRequest,
    LoginRequest,
    SpotifyActionRequest,
    VolumeControlRequest,
    VoiceConfigRequest,
)
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.ota_service import ota_service
from app.services.media_service import media_service
from app.services.weather_ai_service import weather_ai_service
from app.services.audio_service import audio_service
from app.services.spotify_service import spotify_service
from app.services.infographic_service import infographic_service
from app.web.portal_html import PORTAL_HTML

router = APIRouter(tags=["Web Studio & Remote Control v5.0"])


@router.get("/", response_class=HTMLResponse, include_in_schema=False)
async def serve_web_studio():
    """Вбудований Glassmorphism Web-Термінал MagicBox Studio v5.0 (0 МБ додаткового RAM)."""
    return HTMLResponse(content=PORTAL_HTML)


@router.post("/api/v5/auth/login")
async def login_dashboard(req: LoginRequest, request: Request, response: Response):
    client_ip = request.client.host if request.client else "unknown"
    if not check_login_rate_limit(client_ip):
        raise HTTPException(
            status_code=429,
            detail="Забагато невдалих спроб входу. Зачекайте 15 хвилин.",
        )

    is_admin = secrets.compare_digest(req.password, settings.DASHBOARD_PASSWORD) or secrets.compare_digest(req.password, settings.API_KEY)
    if is_admin:
        clear_login_failures(client_ip)
        session_token = secrets.token_urlsafe(36)
        db_service.create_session(session_token, role="admin", duration_sec=86400 * 14)

        is_https = request.url.scheme == "https"
        response.set_cookie(
            key="mb_session",
            value=session_token,
            httponly=True,
            secure=is_https,
            max_age=86400 * 14,
            samesite="lax",
        )
        return {"ok": True, "token": session_token, "role": "admin"}

    record_login_failure(client_ip)
    raise HTTPException(status_code=401, detail="Невірний пароль")


@router.post("/api/v5/auth/logout")
async def logout_dashboard(response: Response, mb_session: Optional[str] = Cookie(None)):
    if mb_session:
        db_service.delete_session(mb_session)
    response.delete_cookie(key="mb_session")
    return {"ok": True}


@router.get("/api/v1/box/state")
@router.get("/api/v5/state")
async def get_full_cloud_state(hours: int = 24, _: str = Depends(verify_access)):
    return {
        "ok": True,
        "is_online": state_service.is_online(),
        "seconds_since_last_ping": state_service.seconds_since_seen(),
        "pending_commands": len(state_service.command_queue),
        "telemetry": state_service.latest_telemetry,
        "climate_history": db_service.get_climate_series(hours=hours),
        "blackout": blackout_service.get_status(),
        "custom_media": db_service.list_custom_media(),
        "spotify": spotify_service.get_cached_state(),
        "ai_persona": {
            "name": state_service.ai_persona_name,
            "prompt": state_service.ai_system_prompt,
            "temperature": state_service.ai_temperature,
        },
        "tts_config": {
            "provider": db_service.get_setting("tts_provider", settings.TTS_PROVIDER),
            "edge_voice": db_service.get_setting("tts_edge_voice", settings.TTS_EDGE_VOICE),
            "clone_voice_id": db_service.get_setting("voice_clone_id", settings.VOICE_CLONE_ID),
        },
        "voice_presets": db_service.list_voice_presets(),
        "resources": memory_guard.get_telemetry(),
    }


@router.post("/api/v1/box/command")
@router.post("/api/v5/command")
async def enqueue_command(cmd: CommandPayload, _: str = Depends(verify_access)):
    queued = state_service.enqueue_command(cmd)
    return {"ok": True, "queued": queued}


@router.get("/api/v1/weather")
async def get_weather_and_barometer(_: str = Depends(verify_access)):
    return await weather_ai_service.get_combined_weather()


@router.post("/api/v1/ai/briefing")
async def trigger_ai_briefing(req: AiAskRequest, _: str = Depends(verify_access)):
    return await weather_ai_service.generate_smart_briefing(req.custom_note or "", speak=req.speak)


@router.post("/api/v5/ai/persona")
async def update_ai_persona(req: AiPersonaConfigRequest, _: str = Depends(verify_access)):
    state_service.save_ai_persona(req.persona_name, req.system_prompt, req.temperature)
    return {"ok": True, "persona_name": state_service.ai_persona_name}


@router.post("/api/v1/media/upload-image")
@router.post("/api/v5/media/upload")
async def upload_custom_media(
    request: Request,
    save_name: str = "",
    reaction_slot: int = 0,
    set_as_saver: bool = False,
    timer_min: int = Query(0, description="Таймер показу в хвилинах (0 = безкінечно)"),
    file: UploadFile = File(...),
    _: str = Depends(verify_access),
):
    meta = await media_service.handle_upload(
        file,
        str(request.base_url),
        save_name=save_name,
        reaction_slot=reaction_slot,
        set_as_saver=set_as_saver,
        media_timer_min=timer_min if timer_min > 0 else None,
    )
    return {"ok": True, **meta}


@router.post("/api/v1/media/sticker/{name}")
@router.post("/api/v5/media/trigger/{name}")
async def trigger_saved_media(
    name: str,
    request: Request,
    saver: bool = Query(False, description="Встановити як постійну заставку (Вікно 2)"),
    timer_min: int = Query(0, description="Таймер показу в хвилинах (0 = безкінечно)"),
    _: str = Depends(verify_access),
):
    return media_service.trigger_saved_media(
        name,
        str(request.base_url),
        set_as_saver=saver,
        media_timer_min=timer_min if timer_min > 0 else None,
    )


@router.post("/api/v5/media/bind")
async def bind_media_to_slot(name: str, slot: int, _: str = Depends(verify_access)):
    ok = db_service.bind_media_to_reaction_slot(name, slot)
    return {"ok": ok, "name": name, "slot": slot}


@router.delete("/api/v5/media/{name}")
async def delete_custom_media(name: str, _: str = Depends(verify_access)):
    fpath = db_service.delete_custom_media(name)
    if fpath and os.path.exists(fpath):
        try:
            os.remove(fpath)
        except OSError:
            pass
    memory_guard.recalculate_disk_usage()
    return {"ok": True, "deleted": name}


@router.get("/api/v5/spotify/login")
async def get_spotify_login(request: Request, _: str = Depends(verify_access)):
    url = spotify_service.get_authorize_url(str(request.base_url))
    return {"ok": True, "auth_url": url}


@router.get("/api/v5/spotify/callback", response_class=HTMLResponse)
async def spotify_oauth_callback(code: str, request: Request):
    ok = await spotify_service.handle_callback(code, str(request.base_url))
    if ok:
        return HTMLResponse(
            "<html><body style='background:#08090C;color:#52B788;font-family:sans-serif;text-align:center;padding:50px;'>"
            "<h2>✨ Spotify успішно підключено!</h2>"
            "<p>Тепер альбомні обкладинки транслюються на вінілову заставку шкатулки.</p>"
            "<script>setTimeout(() => window.location.href='/', 1500);</script>"
            "</body></html>"
        )
    return HTMLResponse(
        "<html><body style='background:#08090C;color:#E63946;font-family:sans-serif;text-align:center;padding:50px;'>"
        "<h2>❌ Помилка авторизації Spotify</h2>"
        "<p>Перевірте Client ID та Client Secret у налаштуваннях.</p>"
        "</body></html>",
        status_code=400,
    )


@router.post("/api/v5/spotify/action")
async def spotify_action(req: SpotifyActionRequest, _: str = Depends(verify_access)):
    ok = await spotify_service.player_action(req.action)
    return {"ok": ok, "state": spotify_service.get_cached_state()}


@router.get("/api/v5/spotify/state")
async def get_spotify_state(_: str = Depends(verify_access)):
    return {"ok": True, "spotify": spotify_service.get_cached_state()}


@router.post("/api/v1/audio/upload-sound")
async def upload_custom_sound(
    request: Request,
    name: str,
    file: UploadFile = File(...),
    _: str = Depends(verify_access),
):
    return await audio_service.save_custom_sound_upload(name, file, str(request.base_url))


@router.post("/api/v1/audio/tts")
async def trigger_tts_speech(text: str, request: Request, _: str = Depends(verify_access)):
    return await audio_service.speak_text(text, str(request.base_url))


@router.post("/api/v1/ota/upload")
async def upload_ota_binary(request: Request, file: UploadFile = File(...), _: str = Depends(verify_admin_access)):
    return await ota_service.save_and_queue_firmware(file, str(request.base_url))


@router.post("/api/v5/media/stop")
async def stop_active_media(_: str = Depends(verify_access)):
    """Зупинити активний показ медіа на лампі та повернутися на попередній екран."""
    media_service.stop_media()
    return {"ok": True, "message": "Зупинка медіа відправлена"}


@router.post("/api/v5/media/timer")
async def set_media_timer(
    minutes: int = Query(..., ge=0, le=1440, description="Хвилини (0 = безкінечно)"),
    _: str = Depends(verify_access),
):
    """Встановити таймер для поточного показу медіа."""
    media_service.set_timer(minutes)
    return {"ok": True, "timer_min": minutes}


@router.get("/api/v5/infographic/png")
async def get_infographic_png(_: str = Depends(verify_access)):
    """Повертає згенеровану 800x500 інфографіку у стилі Hallmark Obsidian."""
    png_path = await infographic_service.generate_infographic_png()
    if not os.path.exists(png_path):
        raise HTTPException(status_code=500, detail="Infographic generation failed")
    return FileResponse(png_path, media_type="image/png", filename="hallmark_infographic.png")


@router.post("/api/v5/audio/volume")
async def set_audio_volume(req: VolumeControlRequest, _: str = Depends(verify_access)):
    """Керування гучністю (0..21), Mute та звуком сенсорних кнопок."""
    cmd = CommandPayload(
        vol=req.volume,
        muted=req.muted,
        touch_beep=req.touch_beep,
    )
    queued = state_service.enqueue_command(cmd)
    return {"ok": True, "queued": queued, "volume": req.volume, "muted": req.muted, "touch_beep": req.touch_beep}


@router.post("/api/v5/audio/voice-config")
async def configure_voice(req: VoiceConfigRequest, _: str = Depends(verify_access)):
    """Налаштування провайдера TTS (Edge-TTS, ElevenLabs, Fish Audio) та голосу."""
    if req.provider:
        db_service.set_setting("tts_provider", req.provider)
    if req.edge_voice:
        db_service.set_setting("tts_edge_voice", req.edge_voice)
    if req.clone_voice_id:
        db_service.set_setting("voice_clone_id", req.clone_voice_id)
    return {
        "ok": True,
        "provider": req.provider or db_service.get_setting("tts_provider", settings.TTS_PROVIDER),
        "edge_voice": req.edge_voice or db_service.get_setting("tts_edge_voice", settings.TTS_EDGE_VOICE),
        "clone_voice_id": req.clone_voice_id or db_service.get_setting("voice_clone_id", settings.VOICE_CLONE_ID),
    }


@router.get("/api/v5/audio/presets")
async def list_voice_presets(_: str = Depends(verify_access)):
    """Список збережених голосових пресетів."""
    return {"ok": True, "presets": db_service.list_voice_presets()}


@router.post("/api/v5/audio/preset/{name}/play")
async def play_voice_preset(name: str, request: Request, _: str = Depends(verify_access)):
    """Озвучити збережену фразу на лампі."""
    res = await audio_service.play_voice_preset(name, str(request.base_url))
    if not res.get("ok"):
        raise HTTPException(status_code=404, detail=res.get("error", "Preset not found"))
    return res
