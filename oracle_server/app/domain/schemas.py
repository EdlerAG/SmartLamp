import time
from pydantic import BaseModel, Field
from typing import Optional, List, Dict, Any


class BoxTelemetryPayload(BaseModel):
    """Вхідний пакет телеметрії від ESP32-S3 (v4.5 / v5.0)."""
    fw: str = "v4.5"
    t: float
    h: float
    p: float
    p_diff: float = 0.0
    l: int = 0
    m: int = 0
    m_name: str = ""
    b_pct: int = 0
    b_lvl: int = 0
    vol: int = 12
    muted: bool = False
    t_beep: bool = True
    dnd: bool = False
    pomo: bool = False
    pomo_rem: int = 0
    pomo_brk: bool = False
    pomo_dur: int = 25
    sleep_m: int = 0
    media_m: int = 0
    scr: int = 0
    rssi: int = -60
    time: str = "--:--"
    unread: bool = False
    heap: int = 0
    psram_free: int = 0
    ack: bool = False
    ping: int = 0  # 2..6 = натиснута фізична кнопка реакції на лампі
    sp_act: int = 0 # 1: next, 2: prev, 3: play_pause від кнопок лампи
    rst: Optional[str] = "UNKNOWN" # Причина останнього перезапуску ESP32
    alarm_off: bool = False # Будильник вимкнено фізичною кнопкою на шкатулці


class CommandPayload(BaseModel):
    """Команда для постановки в чергу шкатулки з підтримкою TTL."""
    created_at: float = Field(default_factory=time.time)
    ttl_sec: float = Field(default=86400.0, description="Час життя команди в секундах")
    set_mode: Optional[int] = Field(None, ge=0, le=11)
    set_brt: Optional[str] = Field(None, description="'auto' або '0'..'3'")
    sleep: Optional[int] = Field(None, ge=0, le=180)
    volume: Optional[int] = Field(None, ge=0, le=21)
    muted: Optional[bool] = None
    touch_beep: Optional[bool] = None
    stop_media: Optional[bool] = None
    media_timer: Optional[int] = None
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
    birthday_celebrate: Optional[bool] = None
    pomo_action: Optional[str] = None
    pomo_min: Optional[int] = None
    pomo_break: Optional[bool] = None
    silent: Optional[bool] = None
    dnd: Optional[bool] = None


class CreateCalendarEventRequest(BaseModel):
    summary: str = Field(..., min_length=1, max_length=120)
    time_str: str = Field(..., description="Час початку '14:30' або ISO рядок")
    date_str: Optional[str] = Field(None, description="Дата 'YYYY-MM-DD' (якщо не вказано — сьогодні)")
    duration_min: Optional[int] = Field(60, ge=5, le=1440)
    description: Optional[str] = Field("", max_length=500)
    source: Optional[str] = Field("manual", description="'manual', 'bot', 'external'")


class SpotifyActionRequest(BaseModel):
    """Запит керування відтворенням Spotify."""
    action: str = Field(..., description="'next', 'previous', 'play', 'pause'")


class VolumeControlRequest(BaseModel):
    """Керування гучністю та звуковими режимами."""
    volume: Optional[int] = Field(None, ge=0, le=21)
    muted: Optional[bool] = None
    touch_beep: Optional[bool] = None


class VoiceConfigRequest(BaseModel):
    """Налаштування синтезатора мовлення та власного голосу."""
    provider: Optional[str] = Field("edge", description="'edge', 'elevenlabs', 'fish_audio'")
    voice_name: Optional[str] = Field(None, description="'uk-UA-OstapNeural', 'uk-UA-PolinaNeural' або назва клону")
    voice_id: Optional[str] = None
    api_key: Optional[str] = None


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
