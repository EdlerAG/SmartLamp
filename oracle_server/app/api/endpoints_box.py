# Файл: oracle_server/app/api/endpoints_box.py
import os
import re
import time
from typing import Dict, Any, List
from fastapi import APIRouter, Depends, HTTPException, Request, WebSocket, WebSocketDisconnect, Query
from fastapi.responses import FileResponse, RedirectResponse
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
from app.services.spotify_service import spotify_service
from app.services.nastya_routine_service import nastya_routine_service
from app.services.google_calendar_service import google_calendar_service

router = APIRouter(tags=["ESP32-S3 Box Gateway v5.0"])


async def build_box_sync_response(payload: BoxTelemetryPayload) -> Dict[str, Any]:
    """Уніфікований обробник синхронізації та формування пакету для HTTP та WebSocket."""
    # 1. Обробка команд плеєра Spotify з кнопок лампи
    if payload.sp_act > 0:
        if payload.sp_act == 1:
            await spotify_service.player_action("next")
        elif payload.sp_act == 2:
            await spotify_service.player_action("previous")
        elif payload.sp_act == 3:
            cur_p = spotify_service.get_cached_state().get("playing", False)
            await spotify_service.player_action("pause" if cur_p else "play")

    # 2. Якщо лампа повідомила про скидання будильника фізичною кнопкою
    if payload.alarm_off:
        nastya_routine_service.dismiss_alarm()

    # 3. Двостороння синхронізація режиму Pomodoro
    has_pomo_cmd = any("pomo_action" in cmd for cmd in state_service.command_queue)
    if not has_pomo_cmd:
        cur_pomo = nastya_routine_service.get_pomodoro_status()
        if payload.pomo:
            nastya_routine_service.sync_pomodoro_from_box(
                active=True,
                remaining_sec=payload.pomo_rem,
                is_break=payload.pomo_brk,
                duration_min=payload.pomo_dur,
            )
        elif cur_pomo.get("active", False):
            nastya_routine_service.sync_pomodoro_from_box(
                active=False,
                remaining_sec=0,
                is_break=False,
                duration_min=payload.pomo_dur,
            )

    # 4. Demand-driven опитування Spotify
    sp_data = await spotify_service.get_sync_state(
        box_screen=payload.scr,
        box_unread=payload.unread,
        box_led_mode=payload.m,
    )

    resp = state_service.pop_next_sync_response()
    resp["sp_linked"] = spotify_service.is_linked()
    if sp_data and (payload.scr == 2 or payload.m == 11):
        resp["spotify"] = sp_data

    # 5. Перевірка світанкового будильника для Насті
    alarm_st = nastya_routine_service.check_alarm_state()
    if alarm_st.get("active"):
        resp["has_cmd"] = True
        resp["alarm_active"] = True
        if alarm_st.get("phase") == "sunrise":
            resp["set_mode"] = 2  # Бурштиновий схід
            ramp_pct = alarm_st.get("brightness_pct", 25)
            p_idx = "0" if ramp_pct <= 25 else ("1" if ramp_pct <= 50 else ("2" if ramp_pct <= 75 else "3"))
            resp["set_brt"] = p_idx
        elif alarm_st.get("phase") == "alarm_ring":
            resp["set_mode"] = 0  # Теплий затишок 100%
            resp["set_brt"] = "3"
            resp["msg"] = "ДОБРОГО РАНКУ, НАСТЮ!"
            resp["play_url"] = f"{settings.PUBLIC_SERVER_URL}/api/v1/media/sound/alarm.mp3"

    # 6. Перевірка тихих годин
    resp["quiet_hours"] = nastya_routine_service.is_quiet_hours_now()

    # 7. Перевірка найближчої події календаря Насті для Smart Hub
    cal_ev = await google_calendar_service.get_next_event()
    if cal_ev:
        resp["cal_event"] = cal_ev

    return resp


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

    # Якщо черга порожня, але лампа просить Long-Poll очікування — спимо з 0% CPU до моменту кліку користувача
    if not state_service.command_queue and wait > 0:
        await realtime_hub.wait_for_command(timeout=wait)

    return await build_box_sync_response(payload)


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

            resp = await build_box_sync_response(payload)
            await ws.send_text(orjson.dumps(resp).decode("utf-8"))
    except WebSocketDisconnect:
        pass
    except Exception:
        pass
    finally:
        realtime_hub.unregister_box_ws(ws)


@router.get("/api/v5/spotify/login_redirect")
async def spotify_login_redirect(request: Request):
    """Компактний 307 Redirect для простого та легкого QR-коду на екрані плати."""
    auth_url = spotify_service.get_authorize_url(str(request.base_url))
    if not auth_url:
        raise HTTPException(status_code=400, detail="Spotify client credentials not configured")
    return RedirectResponse(auth_url, status_code=307)


@router.get("/api/v5/spotify/vinyl.rgb565")
async def download_spotify_vinyl():
    """Віддає 36992 байти обкладинки вінілу 136x136 RGB565 з RAM-диска."""
    if not os.path.exists(settings.SPOTIFY_VINYL_PATH):
        raise HTTPException(status_code=404, detail="Spotify vinyl art not ready")
    return FileResponse(settings.SPOTIFY_VINYL_PATH, media_type="application/octet-stream", filename="vinyl.rgb565")


@router.get("/api/v5/media/live.mbx")
async def download_live_mbx():
    """Віддає активну мультикадрову анімацію .mbx із RAM-диска (/dev/shm)."""
    if not os.path.exists(media_service.RAM_ANIM_PATH):
        raise HTTPException(status_code=404, detail="Active animation not found")
    return FileResponse(media_service.RAM_ANIM_PATH, media_type="application/octet-stream", filename="live.mbx")


@router.post("/api/v5/box/batch-telemetry")
async def receive_batch_telemetry(payload: Dict[str, Any], _=Depends(verify_access)):
    """Приймає пакет офлайн-телеметрії («чорна скринька» шкатулки під час блекауту/відсутності Wi-Fi)."""
    points = payload.get("points", [])
    if not points:
        return {"ok": True, "count": 0}
    now_ts = int(time.time())
    samples = []
    for p in points:
        age_sec = max(0, int(p.get("age_sec", 0)))
        ts = now_ts - age_sec
        samples.append({
            "ts": ts,
            "temp": p.get("t", 22.0),
            "hum": p.get("h", 50.0),
            "pres": p.get("p", 750.0),
            "light": p.get("l", 50),
        })
    count = db_service.add_climate_batch(samples)
    realtime_hub.broadcast("telemetry", {"type": "batch", "count": count})
    return {"ok": True, "count": count}


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


@router.get("/api/v1/media/sound/{name}")
async def download_custom_sound(name: str):
    """Роздача MP3, OGG та WAV звуків реакцій та збережених голосових пресетів."""
    safe_name = re.sub(r"[^a-zA-Z0-9_.-]", "", name)[:36].lower()
    path = os.path.join(settings.SOUNDS_DIR, safe_name)
    if not os.path.exists(path):
        for ext in (".mp3", ".ogg", ".wav"):
            cand = os.path.join(settings.SOUNDS_DIR, f"{safe_name}{ext}")
            if os.path.exists(cand):
                path = cand
                safe_name = f"{safe_name}{ext}"
                break
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Sound not found")
    m_type = "audio/ogg" if safe_name.endswith(".ogg") else ("audio/wav" if safe_name.endswith(".wav") else "audio/mpeg")
    return FileResponse(path, media_type=m_type, filename=safe_name)


@router.get("/api/v1/ota/latest")
async def get_latest_ota_metadata(request: Request):
    return ota_service.get_metadata(str(request.base_url))


@router.get("/api/v1/ota/firmware.bin")
async def download_firmware():
    if not os.path.exists(settings.FIRMWARE_PATH):
        raise HTTPException(status_code=404, detail="Firmware binary not found")
    return FileResponse(settings.FIRMWARE_PATH, media_type="application/octet-stream", filename="firmware.bin")
