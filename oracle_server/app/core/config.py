import os
import tempfile
from dataclasses import dataclass


def _load_dotenv_if_exists(env_path: str = ".env") -> None:
    if not os.path.exists(env_path):
        return
    try:
        with open(env_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or "=" not in line:
                    continue
                k, v = line.split("=", 1)
                k = k.strip()
                v = v.strip().strip("'").strip('"')
                if k and k not in os.environ:
                    os.environ[k] = v
    except Exception:
        pass


_load_dotenv_if_exists()


def _resolve_ram_cache_dir(fallback_storage: str) -> str:
    """Використовує /dev/shm (RAM-диск Linux) для миттєвої роздачі кадрів та TTS з 0% I/O диска."""
    if os.path.isdir("/dev/shm") and os.access("/dev/shm", os.W_OK):
        return "/dev/shm/magicbox_ram"
    return os.path.join(tempfile.gettempdir(), "magicbox_ram")


@dataclass(frozen=True, slots=True)
class Settings:
    PROJECT_NAME: str = "MagicBox Sovereign Cloud v5.0"
    VERSION: str = "5.0.0-HyperLight"

    # Безпека та мережа
    API_KEY: str = os.getenv("MAGICBOX_KEY", "smartlamp-secret-key")
    DASHBOARD_PASSWORD: str = os.getenv("DASHBOARD_PASSWORD", os.getenv("MAGICBOX_KEY", "0511"))
    PUBLIC_SERVER_URL: str = os.getenv("PUBLIC_SERVER_URL", "http://127.0.0.1:8000").rstrip("/")

    # Telegram
    TG_BOT_TOKEN: str = os.getenv("TG_BOT_TOKEN", "")
    TG_ADMIN_ID: str = os.getenv("TG_ADMIN_ID", "")
    TG_GIRL_ID: str = os.getenv("TG_GIRL_ID", "")
    SERVER_MANAGES_BOT: bool = os.getenv("SERVER_MANAGES_BOT", "true").lower() == "true"

    # Таймінги блекауту та синхронізації
    BLACKOUT_TIMEOUT_SEC: float = float(os.getenv("BLACKOUT_TIMEOUT_SEC", "25.0"))

    # Погода та Кастомний ШІ (Опціональний, неблокуючий)
    WEATHER_LAT: float = float(os.getenv("WEATHER_LAT", "49.68"))
    WEATHER_LON: float = float(os.getenv("WEATHER_LON", "32.33"))
    GEMINI_API_KEY: str = os.getenv("GEMINI_API_KEY", "")
    GEMINI_MODEL: str = os.getenv("GEMINI_MODEL", "gemini-2.5-flash")

    # Опціональний міст із QuantCore (читання статусу портфеля з сусідньої папки без витрат RAM)
    QUANTCORE_STATE_PATH: str = os.getenv(
        "QUANTCORE_STATE_PATH",
        os.path.abspath(os.path.join(os.getcwd(), "..", "trading_agent", "QuantCore-GrandMaster-Server", "storage", "engine_state.json")),
    )

    # Постійне дискове сховище (SQLite WAL, бібліотека кастомних гіфок та звуків)
    STORAGE_DIR: str = os.getenv("STORAGE_DIR", "storage")
    DB_PATH: str = os.path.join(STORAGE_DIR, "magicbox_v5.db")
    FIRMWARE_PATH: str = os.path.join(STORAGE_DIR, "ota", "latest_firmware.bin")
    STICKERS_DIR: str = os.path.join(STORAGE_DIR, "stickers")   # Кастомні наліпки та .mbx гіфки
    SOUNDS_DIR: str = os.path.join(STORAGE_DIR, "sounds")       # Кастомні MP3 звуки реакцій

    # Надшвидкий буфер у RAM (/dev/shm) для активних стрімів
    RAM_MEDIA_DIR: str = _resolve_ram_cache_dir(STORAGE_DIR)
    MEDIA_DIR: str = _resolve_ram_cache_dir(STORAGE_DIR)

    # Ліміти для 1GB RAM VPS
    MAX_FW_SIZE: int = 4 * 1024 * 1024
    MAX_STORAGE_BYTES: int = int(os.getenv("MAX_STORAGE_MB", "150")) * 1024 * 1024
    GIF_MAX_FRAMES: int = int(os.getenv("GIF_MAX_FRAMES", "36"))  # До 36 кадрів 240x240 у PSRAM лампи
    GIF_DEFAULT_FPS: int = int(os.getenv("GIF_DEFAULT_FPS", "15"))


settings = Settings()

for _folder in (
    settings.STORAGE_DIR,
    os.path.dirname(settings.FIRMWARE_PATH),
    settings.STICKERS_DIR,
    settings.SOUNDS_DIR,
    settings.RAM_MEDIA_DIR,
):
    os.makedirs(_folder, exist_ok=True)