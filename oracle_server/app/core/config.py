import os
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


@dataclass(frozen=True, slots=True)
class Settings:
    PROJECT_NAME: str = "MagicBox Oracle Cloud Hub"
    VERSION: str = "4.5.1-Production"

    API_KEY: str = os.getenv("MAGICBOX_KEY", "smartlamp-secret-key")
    PUBLIC_SERVER_URL: str = os.getenv("PUBLIC_SERVER_URL", "http://127.0.0.1:8000").rstrip("/")

    TG_BOT_TOKEN: str = os.getenv("TG_BOT_TOKEN", "")
    TG_ADMIN_ID: str = os.getenv("TG_ADMIN_ID", "")
    TG_GIRL_ID: str = os.getenv("TG_GIRL_ID", "")

    SERVER_MANAGES_BOT: bool = os.getenv("SERVER_MANAGES_BOT", "true").lower() == "true"
    BLACKOUT_TIMEOUT_SEC: float = float(os.getenv("BLACKOUT_TIMEOUT_SEC", "25.0"))

    WEATHER_LAT: float = float(os.getenv("WEATHER_LAT", "49.68"))
    WEATHER_LON: float = float(os.getenv("WEATHER_LON", "32.33"))
    GEMINI_API_KEY: str = os.getenv("GEMINI_API_KEY", "")

    STORAGE_DIR: str = os.getenv("STORAGE_DIR", "storage")
    STATE_FILE: str = os.path.join(STORAGE_DIR, "hub_state.json")
    FIRMWARE_PATH: str = os.path.join(STORAGE_DIR, "ota", "latest_firmware.bin")
    MEDIA_DIR: str = os.path.join(STORAGE_DIR, "media")
    SOUNDS_DIR: str = os.path.join(STORAGE_DIR, "sounds")
    STICKERS_DIR: str = os.path.join(STORAGE_DIR, "stickers")

    MAX_FW_SIZE: int = 4 * 1024 * 1024
    MAX_STORAGE_BYTES: int = int(os.getenv("MAX_STORAGE_MB", "150")) * 1024 * 1024


settings = Settings()
for folder in (
    os.path.dirname(settings.FIRMWARE_PATH),
    settings.MEDIA_DIR,
    settings.SOUNDS_DIR,
    settings.STICKERS_DIR,
):
    os.makedirs(folder, exist_ok=True)