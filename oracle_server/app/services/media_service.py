import io
import os
import re
import time
import zlib
import struct
import asyncio
from typing import Dict, Any, Tuple, List
import numpy as np
from fastapi import UploadFile, HTTPException

from app.core.config import settings
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service


def _build_circular_mask(size: int = 240) -> np.ndarray:
    """Створює булеву маску круглого дисплея GC9A01 (240x240) один раз при запуску."""
    y, x = np.ogrid[:size, :size]
    center = (size - 1) / 2.0
    dist_sq = (x - center) ** 2 + (y - center) ** 2
    return dist_sq <= (center ** 2)


_CIRCLE_MASK_240 = _build_circular_mask(240)

BAYER_4X4 = np.array([
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5]
], dtype=np.float32)


class MediaService:
    """
    Векторизований NumPy медіа-рушій v4.0 / v5.0:
    - Конвертує статичні фото/стікери та анімації у RGB565 з Bayer 4x4 Ordered Dithering.
    - Автоматично вираховує домінуючий колір AmbiLight для стрічки SK6812.
    - Підтримує збереження постійної заставки для Вікна 2 (set_saver: true).
    """

    __slots__ = ()

    RAM_FRAME_PATH = os.path.join(settings.RAM_MEDIA_DIR, "active_frame.rgb565")
    RAM_ANIM_PATH = os.path.join(settings.RAM_MEDIA_DIR, "active_anim.mbx")
    BG_COLOR = (6, 9, 16)

    @classmethod
    def _fit_and_composite_frame(cls, frame_img) -> np.ndarray:
        """Масштабує кадр до 240x240 зі збереженням пропорцій та накладає на темний фон."""
        from PIL import Image, ImageOps

        if frame_img.mode in ("RGBA", "LA") or (frame_img.mode == "P" and "transparency" in frame_img.info):
            rgba = frame_img.convert("RGBA")
            rgba.thumbnail((224, 224), Image.Resampling.BILINEAR)
            bg = Image.new("RGBA", (240, 240), (*cls.BG_COLOR, 255))
            offset = ((240 - rgba.width) // 2, (240 - rgba.height) // 2)
            bg.alpha_composite(rgba, dest=offset)
            rgb_img = bg.convert("RGB")
        else:
            rgb_img = ImageOps.fit(frame_img.convert("RGB"), (240, 240), method=Image.Resampling.BILINEAR)

        arr = np.asarray(rgb_img, dtype=np.uint8).copy()
        arr[~_CIRCLE_MASK_240] = cls.BG_COLOR
        return arr

    @staticmethod
    def _rgb_array_to_rgb565_bytes(arr: np.ndarray) -> bytes:
        """
        Векторизоване перетворення (H, W, 3) uint8 -> RGB565 Big-Endian
        із застосуванням Bayer 4x4 Ordered Dithering для усунення градієнтної смугастості на GC9A01.
        """
        h, w = arr.shape[:2]
        tile_y = (h + 3) // 4
        tile_x = (w + 3) // 4
        bayer_grid = np.tile(BAYER_4X4, (tile_y, tile_x))[:h, :w]

        # Розраховуємо делікатне зміщення для кожного кванту
        dither_r = (bayer_grid / 16.0 - 0.5) * 8.0
        dither_g = (bayer_grid / 16.0 - 0.5) * 4.0
        dither_b = (bayer_grid / 16.0 - 0.5) * 8.0

        r_dith = np.clip(arr[:, :, 0].astype(np.float32) + dither_r, 0, 255).astype(np.uint16)
        g_dith = np.clip(arr[:, :, 1].astype(np.float32) + dither_g, 0, 255).astype(np.uint16)
        b_dith = np.clip(arr[:, :, 2].astype(np.float32) + dither_b, 0, 255).astype(np.uint16)

        r = (r_dith & 0xF8) << 8
        g = (g_dith & 0xFC) << 3
        b = b_dith >> 3
        rgb565 = r | g | b
        return rgb565.astype(">u2").tobytes()

    @staticmethod
    def _extract_ambilight_color(arr: np.ndarray) -> Tuple[int, int, int]:
        """Визначає насичений домінуючий колір кадру для підсвітки LED-стрічки SK6812."""
        flat = arr[_CIRCLE_MASK_240].reshape(-1, 3).astype(np.float32)
        brightness = np.max(flat, axis=1)
        saturation = brightness - np.min(flat, axis=1)
        mask = (brightness > 45.0) & (saturation > 25.0)
        vivid = flat[mask]
        if len(vivid) < 50:
            vivid = flat[brightness > 30.0]
        if len(vivid) == 0:
            return (56, 189, 248)
        mean_rgb = np.mean(vivid, axis=0)
        peak = float(np.max(mean_rgb))
        if peak > 0:
            mean_rgb = np.clip(mean_rgb * (235.0 / peak), 0, 255)
        return (int(mean_rgb[0]), int(mean_rgb[1]), int(mean_rgb[2]))

    @classmethod
    def _process_media_sync(
        cls,
        raw_bytes: bytes,
        save_name: str = "",
        reaction_slot: int = 0,
    ) -> Dict[str, Any]:
        from PIL import Image

        frames_bytes: List[bytes] = []
        ambi_rgb: Tuple[int, int, int] = (56, 189, 248)
        fps = settings.GIF_DEFAULT_FPS

        with Image.open(io.BytesIO(raw_bytes)) as img:
            is_anim = getattr(img, "is_animated", False) and getattr(img, "n_frames", 1) > 1
            total_src_frames = getattr(img, "n_frames", 1) if is_anim else 1

            if is_anim:
                duration_ms = img.info.get("duration", 66) or 66
                fps = max(6, min(25, int(round(1000.0 / max(20.0, float(duration_ms))))))
                max_f = settings.GIF_MAX_FRAMES
                step = max(1, total_src_frames // max_f)
                frame_indices = list(range(0, total_src_frames, step))[:max_f]

                for idx_pos, f_idx in enumerate(frame_indices):
                    img.seek(f_idx)
                    arr = cls._fit_and_composite_frame(img)
                    if idx_pos == len(frame_indices) // 2:
                        ambi_rgb = cls._extract_ambilight_color(arr)
                    frames_bytes.append(cls._rgb_array_to_rgb565_bytes(arr))
            else:
                arr = cls._fit_and_composite_frame(img)
                ambi_rgb = cls._extract_ambilight_color(arr)
                frames_bytes.append(cls._rgb_array_to_rgb565_bytes(arr))

        num_frames = len(frames_bytes)
        media_type = "gif" if num_frames > 1 else "sticker"

        mid_frame = frames_bytes[num_frames // 2]
        with open(cls.RAM_FRAME_PATH, "wb") as f:
            f.write(mid_frame)

        body_blob = b"".join(frames_bytes)
        crc_val = zlib.crc32(body_blob) & 0xFFFFFFFF
        crc_hex = f"{crc_val:08x}"

        header = struct.pack(
            "<4sBBBBBBBBI",
            b"MBX5",
            5,
            num_frames,
            fps,
            0,
            ambi_rgb[0],
            ambi_rgb[1],
            ambi_rgb[2],
            0,
            crc_val,
        )
        full_mbx = header + body_blob

        with open(cls.RAM_ANIM_PATH, "wb") as f:
            f.write(full_mbx)

        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", save_name)[:28].lower()
        if not safe_name:
            safe_name = f"custom_slot_{reaction_slot}" if reaction_slot > 0 else f"live_{crc_hex[:6]}"

        if save_name or reaction_slot > 0:
            perm_path = os.path.join(settings.STICKERS_DIR, f"{safe_name}.mbx")
            with open(perm_path, "wb") as f:
                f.write(full_mbx)
            memory_guard.register_file_write(len(full_mbx))
            db_service.upsert_custom_media(
                name=safe_name,
                media_type=media_type,
                file_path=perm_path,
                crc32_hex=crc_hex,
                frames=num_frames,
                fps=fps,
                byte_size=len(full_mbx),
                ambi_rgb=ambi_rgb,
                reaction_slot=reaction_slot,
            )

        del frames_bytes
        del body_blob
        memory_guard.trim_ram()

        return {
            "name": safe_name,
            "media_type": media_type,
            "crc": crc_hex,
            "frames": num_frames,
            "fps": fps,
            "size_bytes": len(full_mbx),
            "ambi_rgb": list(ambi_rgb),
        }

    async def process_and_enqueue(
        self,
        raw_bytes: bytes,
        base_url: str = "",
        save_name: str = "",
        reaction_slot: int = 0,
        set_as_saver: bool = False,
        media_timer_min: Optional[int] = None,
    ) -> Dict[str, Any]:
        meta = await asyncio.to_thread(self._process_media_sync, raw_bytes, save_name, reaction_slot)
        host = state_service.resolve_public_url(base_url)

        if save_name or reaction_slot > 0:
            media_url = f"{host}/api/v5/media/{meta['name']}.mbx?c={meta['crc']}"
        else:
            media_url = f"{host}/api/v5/media/live.mbx?c={meta['crc']}"

        frame_url = f"{host}/api/v1/media/frame.rgb565?v={int(time.time())}"

        cmd_dict: Dict[str, Any] = {
            "media_name": meta["name"],
            "media_url": media_url,
            "img_url": frame_url,
            "media_crc": meta["crc"],
            "media_frames": meta["frames"],
            "media_fps": meta["fps"],
            "ambi_rgb": meta["ambi_rgb"],
        }
        if set_as_saver:
            cmd_dict["set_saver"] = True
        if media_timer_min is not None:
            cmd_dict["media_timer"] = int(media_timer_min)

        state_service.enqueue_raw(cmd_dict)
        meta["media_url"] = media_url
        return meta

    def stop_media(self) -> None:
        state_service.enqueue_raw({
            "stop_media": True,
        })

    def set_timer(self, minutes: int) -> None:
        state_service.enqueue_raw({
            "media_timer": int(minutes),
        })

    def trigger_saved_media(
        self,
        name: str,
        base_url: str = "",
        set_as_saver: bool = False,
        media_timer_min: Optional[int] = None,
    ) -> Dict[str, Any]:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:28].lower()
        info = db_service.get_media_by_name(safe_name)
        if not info or not os.path.exists(info["file_path"]):
            raise HTTPException(status_code=404, detail=f"Кастомну гіфку/наліпку '{safe_name}' не знайдено")

        with open(info["file_path"], "rb") as src:
            raw_mbx = src.read()
        with open(self.RAM_ANIM_PATH, "wb") as dst:
            dst.write(raw_mbx)
        if len(raw_mbx) >= 16 + 115200:
            with open(self.RAM_FRAME_PATH, "wb") as f_dst:
                f_dst.write(raw_mbx[16:16 + 115200])

        host = state_service.resolve_public_url(base_url)
        media_url = f"{host}/api/v5/media/{safe_name}.mbx?c={info['crc32_hex']}"
        frame_url = f"{host}/api/v1/media/frame.rgb565?v={int(time.time())}"
        ambi = [info["ambi_r"], info["ambi_g"], info["ambi_b"]]

        cmd_dict: Dict[str, Any] = {
            "media_name": safe_name,
            "media_url": media_url,
            "img_url": frame_url,
            "media_crc": info["crc32_hex"],
            "media_frames": info["frames"],
            "media_fps": info["fps"],
            "ambi_rgb": ambi,
        }
        if set_as_saver:
            cmd_dict["set_saver"] = True
        if media_timer_min is not None:
            cmd_dict["media_timer"] = int(media_timer_min)

        state_service.enqueue_raw(cmd_dict)
        return {
            "ok": True,
            "name": safe_name,
            "media_type": info["media_type"],
            "frames": info["frames"],
            "fps": info["fps"],
            "crc": info["crc32_hex"],
            "ambi_rgb": ambi,
            "media_url": media_url,
            "is_saver": set_as_saver,
        }

    async def handle_upload(
        self,
        file: UploadFile,
        base_url: str = "",
        save_name: str = "",
        reaction_slot: int = 0,
        set_as_saver: bool = False,
        media_timer_min: Optional[int] = None,
    ) -> Dict[str, Any]:
        raw = await file.read(10 * 1024 * 1024)
        if not raw:
            raise HTTPException(status_code=400, detail="Порожній файл")
        try:
            return await self.process_and_enqueue(raw, base_url, save_name, reaction_slot, set_as_saver, media_timer_min)
        except Exception as e:
            raise HTTPException(status_code=400, detail=f"Помилка конвертації медіа: {e}")


media_service = MediaService()
