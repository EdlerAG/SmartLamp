# Файл: oracle_server/app/services/ota_service.py
import hashlib
import os
import time
from typing import Dict, Any
from fastapi import HTTPException, UploadFile
from app.core.config import settings
from app.core.memory_guard import memory_guard
from app.services.state_service import state_service


class OtaService:
    __slots__ = ("last_uploaded_at", "last_sha256", "last_size")

    def __init__(self) -> None:
        self.last_uploaded_at: float = 0.0
        self.last_sha256: str = ""
        self.last_size: int = 0
        self._restore_existing_metadata()

    def _restore_existing_metadata(self) -> None:
        if os.path.exists(settings.FIRMWARE_PATH):
            try:
                st = os.stat(settings.FIRMWARE_PATH)
                self.last_size = st.st_size
                self.last_uploaded_at = st.st_mtime
                with open(settings.FIRMWARE_PATH, "rb") as f:
                    self.last_sha256 = hashlib.sha256(f.read()).hexdigest()
            except Exception:
                pass

    def save_binary_bytes(self, content: bytes, base_url: str) -> Dict[str, Any]:
        if len(content) > settings.MAX_FW_SIZE:
            raise HTTPException(status_code=413, detail="Файл перевищує ліміт 4 МБ")
        if len(content) < 150000 or content[0] != 0xE9:
            raise HTTPException(status_code=400, detail="Невірний бінарний файл ESP32 (немає заголовка 0xE9)")

        tmp_path = settings.FIRMWARE_PATH + ".tmp"
        with open(tmp_path, "wb") as f:
            f.write(content)
        os.replace(tmp_path, settings.FIRMWARE_PATH)

        self.last_size = len(content)
        self.last_sha256 = hashlib.sha256(content).hexdigest()
        self.last_uploaded_at = time.time()
        memory_guard.trim_ram()

        host = state_service.resolve_public_url(base_url)
        ota_url = f"{host}/api/v1/ota/firmware.bin?sha={self.last_sha256[:10]}"
        state_service.enqueue_raw({"ota_url": ota_url, "ota_sha256": self.last_sha256})

        return {
            "ok": True,
            "size": self.last_size,
            "sha256": self.last_sha256,
            "ota_url": ota_url,
            "status": "Queued for MagicBox v3.8",
        }

    async def save_and_queue_firmware(self, file: UploadFile, base_url: str) -> Dict[str, Any]:
        content = await file.read(settings.MAX_FW_SIZE + 1)
        return self.save_binary_bytes(content, base_url)

    def get_metadata(self, base_url: str) -> Dict[str, Any]:
        exists = os.path.exists(settings.FIRMWARE_PATH)
        host = state_service.resolve_public_url(base_url)
        return {
            "available": exists,
            "size": self.last_size if exists else 0,
            "sha256": self.last_sha256,
            "uploaded_at": self.last_uploaded_at,
            "download_url": f"{host}/api/v1/ota/firmware.bin" if exists else None,
        }


ota_service = OtaService()
