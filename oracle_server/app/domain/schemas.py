from pydantic import BaseModel, Field
from typing import Optional, List, Dict, Any


class BoxTelemetryPayload(BaseModel):
    """Вхідний пакет телеметрії від ESP32-S3 (v4.0 / v5.0)."""
    fw: str = "v4.0"
    t: float
    h: float
    p: float
    p_diff: float = 0.0
    l: int = 50
    m: int = 0
    auto_b: bool = True
    b_idx: int = 2
    b_pct: int = 65
    sleep_m: int = 0
    scr: int = 0
    rssi: int = -60
    time: str = "--:--"
    unread: bool = False
    heap: int = 0
    psram_free: int = 0
    ack: bool = False
    ping: int = 0  # 2..6 = натиснута фізична кнопка реакції на лампі
    sp_act: int = 0 # 1: next, 2: prev, 3: play_pause від кнопок лампи


class CommandPayload(BaseModel):
    """Команда для постановки в чергу шкатулки."""
    set_mode: Optional[int] = Field(None, ge=0, le=11)
    set_brt: Optional[str] = Field(None, description="'auto' або '0'..'3'")
    sleep: Optional[int] = Field(None, ge=0, le=180)
    volume: Optional[int] = Field(None, ge=0, le=21)
    react: Optional[int] = Field(None, ge=2, le=6)
    sender: Optional[str] = Field(None, max_length=32)
    msg: Optional[str] = Field(None, max_length=120)
    ota_url: Optional[str] = None
    play_url: Optional[str] = None
    media_name: Optional[str] = None
    media_url: Optional[str] = None
    media_crc: Optional[str] = None
    media_frames: Optional[int] = None
    media_fps: Optional[int] = None
    ambi_rgb: Optional[List[int]] = None
    set_saver: Optional[bool] = None
    sp_action: Optional[str] = None
    spotify: Optional[Dict[str, Any]] = None
    sys_cmd: Optional[str] = Field(None, description="'reboot', 'clear_cache', 'start_ap', 'eco_sleep'")


class SpotifyActionRequest(BaseModel):
    """Запит керування відтворенням Spotify."""
    action: str = Field(..., description="'next', 'previous', 'play', 'pause'")


class AiPersonaConfigRequest(BaseModel):
    """Налаштування кастомного ШІ-духа шкатулки."""
    persona_name: str = Field("Затишний Дух MagicBox", max_length=40)
    system_prompt: str = Field(..., max_length=1200)
    temperature: float = Field(0.7, ge=0.0, le=1.5)


class AiAskRequest(BaseModel):
    custom_note: Optional[str] = ""
    speak: bool = False


class LoginRequest(BaseModel):
    password: str
