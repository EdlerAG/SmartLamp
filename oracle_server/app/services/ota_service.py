import hashlib
import os
import time
from typing import Dict, Any
from fastapi import HTTPException, UploadFile
from app.core.config import settings
from app.services.state_service import state_service


class OtaService:
    __slots__ = ("last_uploaded_at", "last_sha256", "last_size")

    def __init__(self) -> None:
        self.last_uploaded_at: float = 0.0
        self.last_sha256: str = ""
        self.last_size: int = 0

    def save_binary_bytes(self, content: bytes, base_url: str) -> Dict[str, Any]:
        if len(content) > settings.MAX_FW_SIZE:
            raise HTTPException(status_code=413, detail="Файл перевищує ліміт 4 МБ")
        if len(content) < 150000 or content[0] != 0xE9:
            raise HTTPException(status_code=400, detail="Невірний бінарний файл ESP32 (немає 0xE9)")

        with open(settings.FIRMWARE_PATH, "wb") as f:
            f.write(content)

        self.last_size = len(content)
        self.last_sha256 = hashlib.sha256(content).hexdigest()
        self.last_uploaded_at = time.time()

        ota_url = f"{base_url.rstrip('/')}/api/v1/ota/firmware.bin"
        state_service.enqueue_raw({"ota_url": ota_url})

        return {
            "ok": True,
            "size": self.last_size,
            "sha256": self.last_sha256,
            "ota_url": ota_url,
            "status": "Queued for MagicBox",
        }

    async def save_and_queue_firmware(self, file: UploadFile, base_url: str) -> Dict[str, Any]:
        content = await file.read(settings.MAX_FW_SIZE + 1)
        return self.save_binary_bytes(content, base_url)

    def get_metadata(self, base_url: str) -> Dict[str, Any]:
        exists = os.path.exists(settings.FIRMWARE_PATH)
        return {
            "available": exists,
            "size": os.path.getsize(settings.FIRMWARE_PATH) if exists else 0,
            "sha256": self.last_sha256,
            "uploaded_at": self.last_uploaded_at,
            "download_url": f"{base_url.rstrip('/')}/api/v1/ota/firmware.bin" if exists else None,
        }


ota_service = OtaService()