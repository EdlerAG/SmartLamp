import asyncio
import gc
import io
import os
import re
import time
from typing import Dict, Any
from fastapi import UploadFile, HTTPException
from app.core.config import settings
from app.services.state_service import state_service
from app.services.storage_service import storage_manager


class MediaService:
    """Конвертує фото, прозорі наліпки (.webp) та GIF-анімації у бінарний формат RGB565 для GC9A01."""

    __slots__ = ()
    FRAME_PATH = os.path.join(settings.MEDIA_DIR, "latest_frame.rgb565")
    BG_COLOR = (6, 9, 16)

    @classmethod
    def _image_to_rgb565_bytes(cls, img) -> bytearray:
        from PIL import Image, ImageOps

        if img.mode in ("RGBA", "LA") or (img.mode == "P" and "transparency" in img.info):
            rgba = img.convert("RGBA")
            bg = Image.new("RGBA", rgba.size, (*cls.BG_COLOR, 255))
            composited = Image.alpha_composite(bg, rgba).convert("RGB")
        else:
            composited = img.convert("RGB")

        fitted = ImageOps.fit(composited, (240, 240), method=Image.Resampling.BILINEAR)
        raw_rgb = memoryview(fitted.tobytes())
        buf = bytearray(240 * 240 * 2)

        out_idx = 0
        for i in range(0, len(raw_rgb), 3):
            r = raw_rgb[i]
            g = raw_rgb[i + 1]
            b = raw_rgb[i + 2]
            buf[out_idx] = (r & 0xF8) | (g >> 5)
            buf[out_idx + 1] = ((g & 0x1C) << 3) | (b >> 3)
            out_idx += 2
        return buf

    @classmethod
    def _convert_sync(cls, raw_bytes: bytes, out_path: str) -> int:
        from PIL import Image

        with Image.open(io.BytesIO(raw_bytes)) as img:
            if getattr(img, "is_animated", False) and getattr(img, "n_frames", 1) > 1:
                img.seek(img.n_frames // 2)
            buf = cls._image_to_rgb565_bytes(img)

        with open(out_path, "wb") as f:
            f.write(buf)

        del buf
        gc.collect()
        storage_manager.enforce_quota()
        return 115200

    async def convert_bytes_and_queue(self, raw_bytes: bytes, base_url: str = "", save_name: str = "") -> str:
        target_path = self.FRAME_PATH
        if save_name:
            safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", save_name)[:24] or "sticker"
            target_path = os.path.join(settings.STICKERS_DIR, f"{safe_name}.rgb565")

        await asyncio.to_thread(self._convert_sync, raw_bytes, target_path)
        if target_path != self.FRAME_PATH:
            with open(target_path, "rb") as src, open(self.FRAME_PATH, "wb") as dst:
                dst.write(src.read())

        host = state_service.resolve_public_url(base_url)
        img_url = f"{host}/api/v1/media/frame.rgb565?v={int(time.time())}"
        state_service.enqueue_raw({"img_url": img_url})
        return img_url

    def trigger_saved_sticker(self, name: str, base_url: str = "") -> Dict[str, Any]:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24]
        if not safe_name:
            raise HTTPException(status_code=400, detail="Невірна назва наліпки")
        src_path = os.path.join(settings.STICKERS_DIR, f"{safe_name}.rgb565")
        if not os.path.exists(src_path):
            raise HTTPException(status_code=404, detail=f"Наліпку '{safe_name}' не знайдено")

        with open(src_path, "rb") as src, open(self.FRAME_PATH, "wb") as dst:
            dst.write(src.read())

        host = state_service.resolve_public_url(base_url)
        img_url = f"{host}/api/v1/media/frame.rgb565?v={int(time.time())}"
        state_service.enqueue_raw({"img_url": img_url})
        return {"ok": True, "sticker": safe_name, "img_url": img_url}

    async def convert_image_to_rgb565(self, file: UploadFile, base_url: str = "", save_name: str = "") -> int:
        raw = await file.read(8 * 1024 * 1024)
        try:
            await self.convert_bytes_and_queue(raw, base_url, save_name)
        except Exception as e:
            raise HTTPException(status_code=400, detail=f"Помилка обробки зображення: {e}")
        return 115200


media_service = MediaService()