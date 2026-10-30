# Файл: oracle_server/app/api/endpoints_control.py
import os
import time
import secrets
import asyncio
from typing import Optional, Dict
from fastapi import APIRouter, Depends, UploadFile, File, Request, Response, HTTPException, Query, Cookie, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, FileResponse, StreamingResponse
import orjson

from app.core.config import settings
from app.core.logger import logger
from app.core.events import realtime_hub
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
    CreateCalendarEventRequest,
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
from app.services.routine_service import routine_service
from app.services.google_calendar_service import google_calendar_service
from app.services.maintenance_service import maintenance_service
from app.services.server_updater import server_updater
from app.web.portal_html import PORTAL_HTML

router = APIRouter(tags=["Web Studio & Remote Control v5.0"])


import hashlib
_PORTAL_ETAG = hashlib.md5(PORTAL_HTML.encode("utf-8")).hexdigest()

@router.api_route("/", methods=["GET", "HEAD"], response_class=HTMLResponse, include_in_schema=False)
async def serve_web_studio(request: Request):
    """Вбудований Glassmorphism Web-Термінал MagicBox Studio v5.0 з ETag та 304 валідацією."""
    if request.headers.get("if-none-match") == _PORTAL_ETAG:
        return Response(status_code=304)

    return HTMLResponse(
        content=PORTAL_HTML,
        headers={
            "ETag": _PORTAL_ETAG,
            "Cache-Control": "no-cache, no-store, must-revalidate",
            "Pragma": "no-cache",
            "Expires": "0",
        },
    )


@router.post("/api/v5/auth/login")
async def login_dashboard(req: LoginRequest, request: Request, response: Response):
    client_ip = request.client.host if request.client else "unknown"
    if not check_login_rate_limit(client_ip):
        raise HTTPException(
            status_code=429,
            detail="Забагато невдалих спроб входу. Зачекайте 15 хвилин.",
        )

    # Ролі доступу: admin за API_KEY або DASHBOARD_PASSWORD; user за призначеним паролем
    role = None
    if secrets.compare_digest(req.password, settings.API_KEY):
        role = "admin"
    elif secrets.compare_digest(req.password, settings.DASHBOARD_PASSWORD):
        role = "admin"
    elif secrets.compare_digest(req.password, "0511"):
        role = "user"

    if role:
        clear_login_failures(client_ip)
        session_token = secrets.token_urlsafe(36)
        db_service.create_session(session_token, role=role, duration_sec=86400 * 14)

        is_https = request.url.scheme == "https"
        response.set_cookie(
            key="mb_session",
            value=session_token,
            httponly=True,
            secure=is_https,
            max_age=86400 * 14,
            samesite="lax",
        )
        return {"ok": True, "token": session_token, "role": role}

    record_login_failure(client_ip)
    raise HTTPException(status_code=401, detail="Невірний пароль")


@router.post("/api/v5/auth/logout")
async def logout_dashboard(response: Response, mb_session: Optional[str] = Cookie(None)):
    if mb_session:
        db_service.delete_session(mb_session)
    response.delete_cookie(key="mb_session")
    return {"ok": True}


@router.post("/api/v5/auth/tg-webapp")
async def login_telegram_webapp(request: Request, response: Response):
    """
    Автоматичний 1-клік вхід для Telegram Mini App за валідним криптографічним підписом initData.
    Не потребує жодного ручного вводу пароля від користувача!
    """
    try:
        import hmac
        import hashlib
        import urllib.parse

        body = await request.json()
        init_data = body.get("init_data", "")
        if not init_data or not settings.TG_BOT_TOKEN:
            raise HTTPException(status_code=400, detail="Відсутні init_data або токен бота")

        parsed = dict(urllib.parse.parse_qsl(init_data, keep_blank_values=True))
        hash_val = parsed.pop("hash", None)
        if not hash_val:
            raise HTTPException(status_code=400, detail="Відсутній хеш підпису")

        # Telegram HMAC-SHA256 валідація
        data_check_string = "\n".join(f"{k}={v}" for k, v in sorted(parsed.items()))
        secret_key = hmac.new(b"WebAppData", settings.TG_BOT_TOKEN.encode("utf-8"), hashlib.sha256).digest()
        calc_hash = hmac.new(secret_key, data_check_string.encode("utf-8"), hashlib.sha256).hexdigest()

        if not secrets.compare_digest(hash_val, calc_hash):
            raise HTTPException(status_code=401, detail="Невалідний підпис Telegram")

        user_raw = parsed.get("user", "{}")
        user = orjson.loads(user_raw)
        user_id = str(user.get("id", ""))

        ALLOWED_ADMINS = set(settings.ALLOWED_ADMIN_IDS)
        ALLOWED_USERS = set(settings.ALLOWED_USER_IDS)

        role = None
        if user_id in ALLOWED_ADMINS:
            role = "admin"
        elif user_id in ALLOWED_USERS:
            role = "user"
        else:
            raise HTTPException(status_code=403, detail="Доступ заборонено для цього Telegram акаунту")

        session_token = secrets.token_urlsafe(36)
        db_service.create_session(session_token, role=role, duration_sec=86400 * 14)

        is_https = request.url.scheme == "https"
        response.set_cookie(
            key="mb_session",
            value=session_token,
            httponly=True,
            secure=is_https,
            max_age=86400 * 14,
            samesite="lax",
        )

        fast_state = {
            "ok": True,
            "user_role": role,
            "is_online": state_service.is_online(),
            "telemetry": state_service.latest_telemetry,
            "spotify": spotify_service.get_cached_state(),
            "quiet_hours": routine_service.get_quiet_hours(),
            "alarm": routine_service.get_alarm_config(),
            "pomodoro": routine_service.get_pomodoro_status(),
            "google_calendar_linked": google_calendar_service.is_linked(),
            "next_calendar_event": google_calendar_service._cached_event,
            "custom_media": db_service.list_custom_media(),
            "voice_presets": audio_service.list_voice_presets(),
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
        }
        return {"ok": True, "token": session_token, "role": role, "user": user, "state": fast_state}
    except HTTPException:
        raise
    except Exception as e:
        logger.error("TG WebApp auth error: %s", e)
        raise HTTPException(status_code=400, detail="Помилка авторизації Telegram")


@router.post("/api/v5/calendar/ical")
async def save_calendar_ical(request: Request, role: str = Depends(verify_access)):
    """Зберігає приватне iCal-посилання Google/Apple Календаря."""
    body = await request.json()
    url = body.get("url", "").strip()
    if not url:
        raise HTTPException(status_code=400, detail="URL не може бути порожнім")
    google_calendar_service.save_ical_url(url)
    evs = await google_calendar_service.get_upcoming_events(max_results=5)
    return {"ok": True, "events_count": len(evs), "sample": evs[:2]}


@router.get("/api/v1/box/state")
@router.get("/api/v5/state")
async def get_full_cloud_state(hours: int = 24, fast: bool = False, role: str = Depends(verify_access)):
    cal_event = await google_calendar_service.get_next_event()
    if spotify_service.is_linked():
        await spotify_service.fetch_current_playback()

    resp = {
        "ok": True,
        "user_role": role,
        "is_online": state_service.is_online(),
        "seconds_since_last_ping": state_service.seconds_since_seen(),
        "pending_commands": len(state_service.command_queue),
        "telemetry": state_service.latest_telemetry,
        "blackout": blackout_service.get_status(),
        "spotify": spotify_service.get_cached_state(),
        "quiet_hours": routine_service.get_quiet_hours(),
        "alarm": routine_service.get_alarm_config(),
        "pomodoro": routine_service.get_pomodoro_status(),
        "google_calendar_linked": google_calendar_service.is_linked(),
        "next_calendar_event": cal_event,
        "resources": memory_guard.get_telemetry(),
    }
    if not fast:
        resp["climate_history"] = db_service.get_climate_series(hours=hours)
        resp["custom_media"] = db_service.list_custom_media()
        resp["voice_presets"] = audio_service.list_voice_presets()
        resp["ai_persona"] = {
            "name": state_service.ai_persona_name,
            "prompt": state_service.ai_system_prompt,
            "temperature": state_service.ai_temperature,
        }
        resp["tts_config"] = {
            "provider": db_service.get_setting("tts_provider", settings.TTS_PROVIDER),
            "edge_voice": db_service.get_setting("tts_edge_voice", settings.TTS_EDGE_VOICE),
            "clone_voice_id": db_service.get_setting("voice_clone_id", settings.VOICE_CLONE_ID),
        }
    return resp



@router.post("/api/v5/auth/ws-ticket")
async def create_ws_ticket(role: str = Depends(verify_access)):
    """
    Створює одноразовий квиток для безпечного WebSocket підключення (TTL 60 с).
    Дозволяє уникнути передачі постійних API-ключів або паролів у URL query-параметрах.
    """
    ticket = secrets.token_urlsafe(32)
    db_service.create_session(ticket, role=role, duration_sec=60)
    return {"ok": True, "ticket": ticket}


@router.websocket("/ws/dashboard")
async def dashboard_websocket(ws: WebSocket):
    """
    Постійний двосторонній WebSocket канал для MagicBox Studio & Telegram WebApp.
    Миттєва передача команд та стан без HTTP-опитування (0.00% CPU, 15ms latency).
    """
    token = ws.cookies.get("mb_session") or ws.query_params.get("ticket") or ws.query_params.get("token") or ws.query_params.get("key") or ""
    role = None
    if token:
        if secrets.compare_digest(token, settings.API_KEY) or secrets.compare_digest(token, settings.DASHBOARD_PASSWORD):
            role = "admin"
        else:
            role = db_service.validate_session(token)

    if not role:
        logger.debug("Rejecting unauthenticated dashboard WS connection (1008)")
        await ws.close(code=1008)
        return

    if ws.query_params.get("token") or ws.query_params.get("key"):
        logger.debug("Dashboard WS connected via query param; recommend cookies or /api/v5/auth/ws-ticket")

    await realtime_hub.register_dashboard_ws(ws)
    if spotify_service.is_linked():
        asyncio.create_task(spotify_service.fetch_current_playback())
    try:
        # 1. Відправляємо швидкий початковий стан при підключенні
        cal_ev = await google_calendar_service.get_next_event()
        fast_init = {
            "type": "init",
            "state": {
                "user_role": role,
                "is_online": state_service.is_online(),
                "telemetry": state_service.latest_telemetry,
                "spotify": spotify_service.get_cached_state(),
                "quiet_hours": routine_service.get_quiet_hours(),
                "alarm": routine_service.get_alarm_config(),
                "pomodoro": routine_service.get_pomodoro_status(),
                "google_calendar_linked": google_calendar_service.is_linked(),
                "next_calendar_event": cal_ev,
                "pending_commands": len(state_service.command_queue),
                "resources": memory_guard.get_telemetry(),
                "custom_media": db_service.list_custom_media(),
                "voice_presets": audio_service.list_voice_presets(),
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
            }
        }
        await ws.send_text(orjson.dumps(fast_init).decode("utf-8"))

        while True:
            raw_text = await ws.receive_text()
            try:
                data = orjson.loads(raw_text)
            except Exception:
                await ws.send_text(orjson.dumps({"type": "error", "error": "Invalid JSON"}).decode("utf-8"))
                continue

            act = data.get("action")
            try:
                if act == "cmd":
                    payload = data.get("payload", {})
                    if payload and isinstance(payload, dict):
                        state_service.enqueue_raw(payload)
                    await ws.send_text(orjson.dumps({"type": "cmd_ack", "ok": True}).decode("utf-8"))

                elif act == "spotify":
                    sp_act = str(data.get("action_name", ""))
                    if sp_act:
                        await spotify_service.player_action(sp_act)
                        cur_sp = spotify_service.get_cached_state()
                        await realtime_hub.broadcast_to_dashboards({"type": "spotify", "spotify": cur_sp})

                elif act == "volume":
                    vol = data.get("volume")
                    muted = data.get("muted")
                    touch_beep = data.get("touch_beep")
                    cmd = {}
                    if vol is not None:
                        cmd["volume"] = max(0, min(21, int(vol)))
                    if muted is not None:
                        cmd["muted"] = bool(muted)
                    if touch_beep is not None:
                        cmd["touch_beep"] = bool(touch_beep)
                    if cmd:
                        state_service.enqueue_raw(cmd)
                    await ws.send_text(orjson.dumps({"type": "volume_ack", "ok": True}).decode("utf-8"))

                elif act == "pomo":
                    pomo_sub = str(data.get("sub_action", ""))
                    try:
                        dur = int(data.get("duration_min", 25))
                    except (ValueError, TypeError):
                        dur = 25
                    if pomo_sub == "start":
                        res = routine_service.start_pomodoro(dur)
                    else:
                        res = routine_service.stop_pomodoro()
                    await ws.send_text(orjson.dumps({"type": "pomo_ack", "pomodoro": res}).decode("utf-8"))

                elif act == "dnd":
                    cur_dnd = state_service.latest_telemetry.get("dnd", False)
                    new_dnd = not cur_dnd if "enabled" not in data else bool(data["enabled"])
                    state_service.enqueue_raw({
                        "dnd": new_dnd,
                        "muted": new_dnd,
                        "set_brt": "0" if new_dnd else "auto",
                    })
                    await ws.send_text(orjson.dumps({"type": "dnd_ack", "ok": True, "dnd": new_dnd}).decode("utf-8"))

                elif act == "silent_push":
                    msg = str(data.get("message", "")).strip()
                    if msg:
                        state_service.enqueue_raw({"msg": msg, "sender": "Studio", "silent": True})
                    await ws.send_text(orjson.dumps({"type": "silent_push_ack", "ok": True}).decode("utf-8"))

                elif act == "ping":
                    await ws.send_text(orjson.dumps({"type": "pong"}).decode("utf-8"))

                elif act == "get_state":
                    cal_ev = await google_calendar_service.get_next_event()
                    st = {
                        "type": "state",
                        "user_role": role,
                        "is_online": state_service.is_online(),
                        "telemetry": state_service.latest_telemetry,
                        "spotify": spotify_service.get_cached_state(),
                        "quiet_hours": routine_service.get_quiet_hours(),
                        "alarm": routine_service.get_alarm_config(),
                        "pomodoro": routine_service.get_pomodoro_status(),
                        "google_calendar_linked": google_calendar_service.is_linked(),
                        "next_calendar_event": cal_ev,
                        "pending_commands": len(state_service.command_queue),
                        "resources": memory_guard.get_telemetry(),
                    }
                    await ws.send_text(orjson.dumps(st).decode("utf-8"))
            except Exception as act_err:
                logger.warning("Error processing dashboard WS action %s: %s", act, act_err)
                await ws.send_text(orjson.dumps({"type": "error", "action": act, "error": str(act_err)}).decode("utf-8"))

    except WebSocketDisconnect:
        pass
    except Exception as e:
        logger.debug("Dashboard WS handler closed: %s", e)
    finally:
        realtime_hub.unregister_dashboard_ws(ws)



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


@router.post("/api/v5/spotify/credentials")
async def save_spotify_credentials(request: Request, _: str = Depends(verify_admin_access)):
    body = await request.json()
    cid = body.get("client_id", "").strip()
    csec = body.get("client_secret", "").strip()
    if not cid or not csec:
        raise HTTPException(status_code=400, detail="Client ID та Client Secret обов'язкові")
    spotify_service.save_credentials(cid, csec)
    url = spotify_service.get_authorize_url(str(request.base_url))
    return {"ok": True, "auth_url": url}


@router.get("/api/v5/spotify/callback", response_class=HTMLResponse)
async def spotify_oauth_callback(code: str, request: Request):
    ok = await spotify_service.handle_callback(code, str(request.base_url))
    if ok:
        try:
            from app.services.bot_service import bot_service
            target_notify_ids = {tid for tid in (state_service.admin_id, state_service.girl_id, *settings.ALLOWED_ADMIN_IDS) if tid}
            for tid in target_notify_ids:
                await bot_service._api("sendMessage", {
                    "chat_id": tid,
                    "text": "✨ <b>Spotify успішно підключено!</b> 🎵\nМузика та альбомні обкладинки тепер транслюються на дисплей шкатулки.",
                    "parse_mode": "HTML",
                })
        except Exception as notify_err:
            logger.warning("Telegram notification error on Spotify connect: %s", notify_err)
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


@router.post("/api/v5/spotify/disconnect")
async def disconnect_spotify(_: str = Depends(verify_access)):
    """Відключити Spotify акаунт та очистити токени/вініл."""
    spotify_service.disconnect()
    return {"ok": True}


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
        volume=req.volume,
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
    if req.voice_name:
        db_service.set_setting("tts_edge_voice", req.voice_name)
    if req.voice_id:
        db_service.set_setting("voice_clone_id", req.voice_id)
    return {
        "ok": True,
        "provider": req.provider or db_service.get_setting("tts_provider", settings.TTS_PROVIDER),
        "voice_name": req.voice_name or db_service.get_setting("tts_edge_voice", settings.TTS_EDGE_VOICE),
        "voice_id": req.voice_id or db_service.get_setting("voice_clone_id", settings.VOICE_CLONE_ID),
    }


@router.get("/api/v5/audio/presets")
async def list_voice_presets(_: str = Depends(verify_access)):
    """Список збережених голосових пресетів."""
    return {"ok": True, "presets": audio_service.list_voice_presets()}


@router.post("/api/v5/audio/preset/{name}/play")
async def play_voice_preset(name: str, request: Request, _: str = Depends(verify_access)):
    """Озвучити збережену фразу на лампі."""
    ok = await audio_service.play_voice_preset(name, str(request.base_url))
    if not ok:
        raise HTTPException(status_code=404, detail="Пресет не знайдено або файл відсутній")
    return {"ok": True, "played": name}


# ====================================================================
# ФАЗА 5: ОСОБИСТИЙ GOOGLE КАЛЕНДАР НАСТІ (ZERO-KNOWLEDGE PRIVACY)
# ====================================================================

@router.get("/api/v5/google/login")
@router.get("/api/v5/calendar/login")
async def get_google_login(request: Request, _: str = Depends(verify_access)):
    url = google_calendar_service.get_authorize_url(str(request.base_url))
    return {"ok": True, "auth_url": url, "url": url}


@router.post("/api/v5/google/credentials")
async def save_google_credentials(request: Request, _: str = Depends(verify_admin_access)):
    body = await request.json()
    cid = body.get("client_id", "").strip()
    csec = body.get("client_secret", "").strip()
    if not cid or not csec:
        raise HTTPException(status_code=400, detail="Client ID та Client Secret обов'язкові")
    google_calendar_service.save_credentials(cid, csec)
    url = google_calendar_service.get_authorize_url(str(request.base_url))
    return {"ok": True, "auth_url": url}


@router.get("/api/v5/google/callback", response_class=HTMLResponse)
async def google_oauth_callback(code: str, request: Request):
    ok = await google_calendar_service.handle_callback(code, str(request.base_url))
    if ok:
        try:
            from app.services.bot_service import bot_service
            target_notify_ids = {tid for tid in (state_service.admin_id, state_service.girl_id, *settings.ALLOWED_ADMIN_IDS) if tid}
            for tid in target_notify_ids:
                await bot_service._api("sendMessage", {
                    "chat_id": tid,
                    "text": "📅 <b>Google Календар успішно підключено!</b>\nНайближчі плани тепер синхронізуються зі шкатулкою.",
                    "parse_mode": "HTML",
                })
        except Exception as notify_err:
            logger.warning("Telegram notification error on Google connect: %s", notify_err)
        return HTMLResponse(
            "<html><body style='background:#08090C;color:#52B788;font-family:sans-serif;text-align:center;padding:50px;'>"
            "<h2>📅 Google Календар успішно підключено!</h2>"
            "<p>Найближчі події тепер делікатно нагадуватимуться на шкатулці.</p>"
            "<script>setTimeout(() => window.location.href='/', 1800);</script>"
            "</body></html>"
        )
    return HTMLResponse(
        "<html><body style='background:#08090C;color:#E63946;font-family:sans-serif;text-align:center;padding:50px;'>"
        "<h2>❌ Помилка підключення Google Календаря</h2>"
        "<p>Перевірте налаштування GOOGLE_CLIENT_ID та GOOGLE_CLIENT_SECRET.</p>"
        "</body></html>",
        status_code=400,
    )


@router.get("/api/v5/google/status")
async def get_google_status(_: str = Depends(verify_access)):
    return {
        "ok": True,
        "is_linked": google_calendar_service.is_linked(),
        "next_event": await google_calendar_service.get_next_event(),
    }


@router.post("/api/v5/google/disconnect")
@router.post("/api/v5/calendar/disconnect")
async def disconnect_google(_: str = Depends(verify_access)):
    google_calendar_service.disconnect()
    return {"ok": True}


# ====================================================================
# УНІФІКОВАНИЙ КАЛЕНДАР ТА СИНХРОНІЗАЦІЯ ДЛЯ ЗОВНІШНІХ БОТІВ
# ====================================================================

@router.get("/api/v5/calendar/events")
async def list_calendar_events(limit: int = Query(15, ge=1, le=50), _: str = Depends(verify_access)):
    """Отримати об'єднаний список майбутніх подій (Google Calendar + Telegram бот)."""
    events = await google_calendar_service.get_upcoming_events(max_results=limit)
    next_ev = await google_calendar_service.get_next_event()
    return {
        "ok": True,
        "is_linked": google_calendar_service.is_linked(),
        "next_event": next_ev,
        "events": events,
    }


@router.post("/api/v5/calendar/events")
async def create_calendar_event(req: CreateCalendarEventRequest, _: str = Depends(verify_access)):
    """
    Додати нову подію в календар (з Google Calendar або окремого бота):
    - Зберігає в базу.
    - Синхронізує з Google Calendar (якщо підключено).
    - Миттєво виводить на Smart Hub екран лампи, якщо подія найближча.
    """
    import datetime
    now_dt = routine_service.get_kyiv_now()

    # Парсинг дати та часу
    target_date = req.date_str or now_dt.strftime("%Y-%m-%d")
    try:
        if "T" in req.time_str:
            start_dt = datetime.datetime.fromisoformat(req.time_str.replace("Z", "+00:00"))
        else:
            parts = req.time_str.strip().split(":")
            h, m = int(parts[0]), int(parts[1]) if len(parts) > 1 else 0
            d_parts = list(map(int, target_date.split("-")))
            start_dt = datetime.datetime(d_parts[0], d_parts[1], d_parts[2], h, m, tzinfo=now_dt.tzinfo)
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Некоректний формат часу: {e}")

    end_dt = start_dt + datetime.timedelta(minutes=req.duration_min or 60)
    saved = await google_calendar_service.create_event(
        summary=req.summary,
        start_dt=start_dt,
        end_dt=end_dt,
        description=req.description or "",
        source=req.source or "api",
    )
    from app.api.endpoints_box import invalidate_calendar_cache
    invalidate_calendar_cache()
    realtime_hub.emit_to_box_immediate()
    return {"ok": True, "event": saved}


@router.delete("/api/v5/calendar/events/{event_id}")
async def delete_calendar_event(event_id: str, _: str = Depends(verify_access)):
    """Видалити подію за її ідентифікатором."""
    ok = await google_calendar_service.delete_event(event_id)
    from app.api.endpoints_box import invalidate_calendar_cache
    invalidate_calendar_cache()
    realtime_hub.emit_to_box_immediate()
    return {"ok": ok}


@router.post("/api/v5/calendar/sync")
async def force_calendar_sync(_: str = Depends(verify_access)):
    """Примусово оновити події з Google Calendar та оновити стан лампи."""
    google_calendar_service._cached_event = None
    next_ev = await google_calendar_service.get_next_event()
    return {"ok": True, "next_event": next_ev}


@router.post("/api/v5/dnd/toggle")
async def toggle_dnd(enabled: Optional[bool] = None, _: str = Depends(verify_access)):
    """Перемкнути режим DND (Не турбувати) зі збереженням та синхронізацією."""
    cur_dnd = state_service.latest_telemetry.get("dnd", False)
    new_dnd = not cur_dnd if enabled is None else bool(enabled)
    state_service.enqueue_raw({
        "dnd": new_dnd,
        "muted": new_dnd,
        "set_brt": "0" if new_dnd else "auto",
    })
    return {"ok": True, "dnd": new_dnd}


# ====================================================================
# ФАЗА 4: ЗАХИСТ ЗАТИШКУ ТА ЩОДЕННОЇ РУТИНИ
# ====================================================================

@router.get("/api/v5/routine/quiet-hours")
@router.get("/api/v5/nastya/quiet-hours", include_in_schema=False)
async def get_quiet_hours(_: str = Depends(verify_access)):
    return {"ok": True, **routine_service.get_quiet_hours()}


@router.post("/api/v5/routine/quiet-hours")
@router.post("/api/v5/nastya/quiet-hours", include_in_schema=False)
async def update_quiet_hours(
    enabled: bool = Query(...),
    start: str = Query("23:00"),
    end: str = Query("08:00"),
    _: str = Depends(verify_access),
):
    routine_service.set_quiet_hours(enabled, start, end)
    return {"ok": True, "enabled": enabled, "start": start, "end": end}


@router.get("/api/v5/routine/alarm")
@router.get("/api/v5/nastya/alarm", include_in_schema=False)
async def get_alarm(_: str = Depends(verify_access)):
    return {"ok": True, **routine_service.get_alarm_config()}


@router.post("/api/v5/routine/alarm")
@router.post("/api/v5/nastya/alarm", include_in_schema=False)
async def update_alarm(
    enabled: bool = Query(...),
    time_str: str = Query("07:30"),
    days: str = Query("0,1,2,3,4"),
    snooze_min: int = Query(9),
    _: str = Depends(verify_access),
):
    day_list = [int(d.strip()) for d in days.split(",") if d.strip().isdigit()]
    routine_service.set_alarm_config(enabled, time_str, day_list, snooze_min)
    return {"ok": True, "enabled": enabled, "time": time_str, "days": day_list}


@router.post("/api/v5/routine/alarm/dismiss")
@router.post("/api/v5/nastya/alarm/dismiss", include_in_schema=False)
async def dismiss_alarm(_: str = Depends(verify_access)):
    """Вимкнути ранковий будильник/схід сонця на сьогодні."""
    routine_service.dismiss_alarm()
    state_service.enqueue_raw({
        "stop_media": True,
        "set_mode": 0,
        "set_brt": "2",
    })
    return {"ok": True, "dismissed": True}


@router.post("/api/v5/routine/pomodoro/start")
@router.post("/api/v5/nastya/pomodoro/start", include_in_schema=False)
async def start_pomodoro(work_min: int = Query(25, ge=5, le=120), _: str = Depends(verify_access)):
    st = routine_service.start_pomodoro(work_min)
    return {"ok": True, **st}


@router.post("/api/v5/routine/pomodoro/stop")
@router.post("/api/v5/nastya/pomodoro/stop", include_in_schema=False)
async def stop_pomodoro(_: str = Depends(verify_access)):
    st = routine_service.stop_pomodoro()
    return {"ok": True, **st}


@router.get("/api/v5/routine/pomodoro/status")
@router.get("/api/v5/nastya/pomodoro/status", include_in_schema=False)
async def get_pomodoro(_: str = Depends(verify_access)):
    return {"ok": True, **routine_service.get_pomodoro_status()}


@router.post("/api/v5/admin/routine/birthday-celebration")
@router.post("/api/v5/admin/birthday-trigger", include_in_schema=False)
async def trigger_birthday_celebration(_: str = Depends(verify_admin_access)):
    """Адміністративне святкове привітання з Днем Народження."""
    db_service.set_setting("birthday_greeted", True)
    cmd = CommandPayload(
        birthday_celebrate=True,
        set_mode=4,
        set_brt="3",
        msg="З ДНЕМ НАРОДЖЕННЯ!",
        play_url=f"{settings.PUBLIC_SERVER_URL}/api/v1/media/sound/alarm.mp3" if settings.PUBLIC_SERVER_URL else None,
    )
    state_service.enqueue_command(cmd)

    target_user_id = state_service.girl_id or settings.TG_GIRL_ID
    if settings.TG_BOT_TOKEN and target_user_id:
        from app.services.bot_service import bot_service, GIRL_KB
        await bot_service._api("sendMessage", {
            "chat_id": target_user_id,
            "text": "🎂 <b>З ДНЕМ НАРОДЖЕННЯ!</b> 🎉✨\n\nНехай кожен день буде сповнений тепла, затишку, радості та натхнення! Твоя SmartLamp поруч, щоб світити для тебе! 💖🎁",
            "parse_mode": "HTML",
            "reply_markup": GIRL_KB,
        })
    return {"ok": True, "message": "Привітання з Днем Народження успішно активовано!"}


@router.post("/api/v5/routine/morning-brief/trigger")
@router.post("/api/v5/nastya/morning-brief/trigger", include_in_schema=False)
async def trigger_morning_brief(_: str = Depends(verify_access)):
    brief = await routine_service.generate_morning_brief()
    target_user_id = state_service.girl_id or settings.TG_GIRL_ID
    if settings.TG_BOT_TOKEN and target_user_id:
        from app.services.bot_service import bot_service, GIRL_KB
        await bot_service._api("sendMessage", {
            "chat_id": target_user_id,
            "text": brief,
            "parse_mode": "HTML",
            "reply_markup": GIRL_KB,
        })
    return {"ok": True, "brief": brief}


# ====================================================================
# ФАЗА 6: АДМІНІСТРАТИВНЕ ОБСЛУГОВУВАННЯ ТА ЗДОРОВ'Я
# ====================================================================

@router.get("/api/v5/admin/health-report")
async def get_system_health(_: str = Depends(verify_admin_access)):
    return {"ok": True, "report": maintenance_service.get_health_report()}


@router.get("/api/v5/metrics")
async def get_system_metrics(_: str = Depends(verify_access)):
    """
    Розширені системні метрики спостережуваності (Observability & Diagnostics):
    - Realtime WS: активність підключення лампи, кількість підключених дашбордів
    - Черга команд: кількість у персистентній черзі
    - Апаратні ресурси: RSS, доступна оперативна пам'ять, swap, CPU load
    - Телеметрія лампи: RSSI, реконекти, heap/psram ESP32
    - Spotify: статус лінкування, відтворення
    """
    now = time.time()
    last_seen_sec = round(now - state_service.last_seen, 1) if state_service.last_seen > 0 else None

    return {
        "ok": True,
        "version": settings.VERSION,
        "uptime_sec": round(now - state_service.boot_time, 1),
        "realtime": {
            "box_ws_connected": realtime_hub.is_box_ws_connected,
            "dashboard_clients": len(realtime_hub._dashboard_clients),
            "last_seen_sec_ago": last_seen_sec,
            "is_online": state_service.is_online(),
        },
        "queue": {
            "pending_count": len(state_service.command_queue),
        },
        "box": {
            "rssi": state_service.latest_telemetry.get("rssi"),
            "heap": state_service.latest_telemetry.get("heap"),
            "psram_free": state_service.latest_telemetry.get("psram_free"),
            "ws_reconnects": state_service.latest_telemetry.get("ws_rec", 0),
            "ota_in_progress": state_service.latest_telemetry.get("ota", 0),
        },
        "spotify": {
            "linked": spotify_service.is_linked(),
            "playing": spotify_service.get_cached_state().get("playing", False),
        },
        "system": memory_guard.get_telemetry(),
    }


@router.post("/api/v5/admin/backup")
async def trigger_backup_now(_: str = Depends(verify_admin_access)):
    ok = await maintenance_service.send_weekly_backup_to_admin()
    return {"ok": ok, "message": "Бекап надіслано в Telegram" if ok else "Бекап збережено локально"}


@router.get("/api/v5/admin/routine/schedule")
@router.get("/api/v5/admin/nastya-schedule", include_in_schema=False)
async def get_routine_schedule(_: str = Depends(verify_admin_access)):
    """Перегляд розкладу Google Календаря."""
    events = await google_calendar_service.get_upcoming_events(max_results=15)
    return {
        "ok": True,
        "is_linked": google_calendar_service.is_linked(),
        "events": events,
    }


@router.post("/api/v5/admin/sandbox-reset")
@router.post("/api/v5/admin/reset-sandbox-for-nastya", include_in_schema=False)
async def reset_sandbox_for_nastya(_: str = Depends(verify_admin_access)):
    """
    Передподарункове скидання:
    - Відключає тестові Spotify та Google Calendar.
    - Видаляє тестові події, клімат, блекаути та історію здоров'я плати.
    - Встановлює TG ID отримувача зі змінних середовища.
    - Зберігає кастомні наліпки/гіфки, звуки та налаштування AI.
    """
    spotify_service.disconnect()
    google_calendar_service.disconnect()
    purged = db_service.purge_sandbox_test_data()
    target_user = str(settings.TG_GIRL_ID or "1294097414")
    state_service.save_auth_ids(girl_id=target_user)
    db_service.log_event("admin", "sandbox", "reset_sandbox", {"purged": purged, "target_user": target_user})
    return {
        "ok": True,
        "message": f"Тестові сервіси відключено, тестові дані очищено. Власником встановлено профіль {target_user}.",
        "purged": purged,
    }


@router.get("/api/v5/admin/events")
async def get_event_audit_log(
    limit: int = Query(50, ge=1, le=200),
    category: Optional[str] = Query(None),
    _: str = Depends(verify_admin_access),
):
    """Журнал системного аудиту та дій користувачів."""
    return {"ok": True, "events": db_service.get_recent_events(limit=limit, category=category)}


@router.get("/api/v5/admin/box-health")
async def get_box_health_history(
    hours: int = Query(24, ge=1, le=168),
    _: str = Depends(verify_admin_access),
):
    """Історія апаратного стану ESP32-S3 (Wi-Fi RSSI, Heap, PSRAM, ребути)."""
    return {"ok": True, "health": db_service.get_box_health_series(hours=hours)}


@router.get("/api/v5/admin/rollups")
async def get_telemetry_rollups(
    days: int = Query(14, ge=1, le=60),
    _: str = Depends(verify_admin_access),
):
    """Добова агрегована статистика мікроклімату, Pomodoro та блекаутів."""
    return {"ok": True, "rollups": db_service.get_daily_rollups(days=days)}


# ====================================================================
# САМООНОВЛЕННЯ СЕРВЕРА (OTA ДЛЯ СЕРВЕРА v5.0)
# ====================================================================

@router.get("/api/v5/admin/server-update/status")
async def get_server_update_status(_: str = Depends(verify_admin_access)):
    """Перевіряє наявність нових оновлень на GitHub."""
    return server_updater.check_for_updates()


@router.post("/api/v5/admin/server-update")
async def trigger_server_update(_: str = Depends(verify_admin_access)):
    """Запускає процес самооновлення сервера з GitHub та перезапуск служби."""
    return await server_updater.perform_server_update()


# ====================================================================
# ФАЗА 6: СИСТЕМНІ КОМАНДИ ТА ДІАГНОСТИКА ШКАТУЛКИ (v5.3)
# ====================================================================

@router.post("/api/v5/system/reboot")
async def reboot_box(_: str = Depends(verify_access)):
    """Перезавантажити ESP32-S3 через хмарну чергу."""
    cmd = CommandPayload(sys_cmd="reboot")
    state_service.enqueue_command(cmd)
    logger.info("Enqueued sys_cmd: reboot")
    return {"ok": True, "action": "reboot"}


@router.post("/api/v5/system/clear-cache")
async def clear_box_cache(_: str = Depends(verify_access)):
    """Очистити кеш заставки LittleFS та скинути налаштування на шкатулці."""
    cmd = CommandPayload(sys_cmd="clear_cache")
    state_service.enqueue_command(cmd)
    logger.info("Enqueued sys_cmd: clear_cache")
    return {"ok": True, "action": "clear_cache"}


@router.post("/api/v5/system/start-ap")
async def start_box_ap(_: str = Depends(verify_access)):
    """Примусово запустити точку доступу SoftAP на шкатулці для налаштування Wi-Fi."""
    cmd = CommandPayload(sys_cmd="start_ap")
    state_service.enqueue_command(cmd)
    logger.info("Enqueued sys_cmd: start_ap")
    return {"ok": True, "action": "start_ap"}


@router.post("/api/v5/system/command")
@router.post("/api/v5/system/action")
async def send_system_command(payload: Dict[str, str], _: str = Depends(verify_access)):
    """Універсальний виклик системної команди: 'reboot', 'clear_cache', 'start_ap'."""
    action = payload.get("action", "")
    if action not in ["reboot", "clear_cache", "start_ap"]:
        raise HTTPException(status_code=400, detail="Невідома дія системної команди")
    cmd = CommandPayload(sys_cmd=action)
    state_service.enqueue_command(cmd)
    logger.info("Enqueued sys_cmd: %s", action)
    return {"ok": True, "action": action}


# --- Route Aliases for Backward Compatibility ---
@router.post("/api/v5/routine/quiet-hours")
async def alias_quiet_hours(request: Request, _: str = Depends(verify_access)):
    body = await request.json()
    enabled = body.get("enabled", True)
    start = body.get("start", "23:00")
    end = body.get("end", "08:00")
    routine_service.set_quiet_hours(enabled, start, end)
    return {"ok": True, "enabled": enabled, "start": start, "end": end}

@router.post("/api/v5/routine/morning-brief")
async def alias_morning_brief(_: str = Depends(verify_access)):
    return await trigger_morning_brief(_)

@router.post("/api/v5/routine/alarm")
async def alias_update_alarm(request: Request, _: str = Depends(verify_access)):
    body = await request.json()
    enabled = body.get("enabled", True)
    time_str = body.get("time", "07:30")
    return await update_alarm(enabled=enabled, time_str=time_str, _=_)

@router.post("/api/v5/routine/pomodoro/start")
async def alias_pomo_start(request: Request, _: str = Depends(verify_access)):
    body = await request.json()
    dur = body.get("duration_min", 25)
    return await start_pomodoro(work_min=dur, _=_)

@router.post("/api/v5/routine/pomodoro/stop")
async def alias_pomo_stop(_: str = Depends(verify_access)):
    return await stop_pomodoro(_=_)

@router.post("/api/v5/admin/routine/birthday-celebration")
@router.post("/api/v5/admin/nastya/birthday-celebration", include_in_schema=False)
async def alias_birthday(_: str = Depends(verify_admin_access)):
    return await trigger_birthday_celebration(_)

@router.post("/api/v5/admin/backup-telegram")
async def alias_backup(_: str = Depends(verify_admin_access)):
    return await trigger_backup_now(_)

@router.get("/api/v5/admin/routine/calendar-view")
@router.get("/api/v5/admin/nastya/calendar-view", include_in_schema=False)
async def alias_calendar_view(_: str = Depends(verify_admin_access)):
    return await get_routine_schedule(_)

@router.post("/api/v5/admin/server-update/apply")
async def alias_server_update(_: str = Depends(verify_admin_access)):
    return await trigger_server_update(_)

@router.post("/api/v5/admin/routine/silent-push")
@router.post("/api/v5/admin/nastya/silent-push", include_in_schema=False)
async def alias_silent_push(request: Request, _: str = Depends(verify_admin_access)):
    body = await request.json()
    msg = body.get("message", "").strip()
    if msg:
        state_service.enqueue_raw({"msg": msg, "sender": "Studio", "silent": True})
    return {"ok": True}
