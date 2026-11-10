from __future__ import annotations
import io
import os
import re
import time
import zlib
import struct
import asyncio
from typing import Dict, Any, Tuple, List, Optional
from fastapi import UploadFile, HTTPException

from app.core.config import settings
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service

_NP_CACHE = {}

def _get_np_resources():
    """Lazy-loading NumPy ресурсів: 0 MB витрат RAM при старті сервера."""
    if "np" not in _NP_CACHE:
        import numpy as np
        y, x = np.ogrid[:240, :240]
        center = (240 - 1) / 2.0
        mask = (x - center) ** 2 + (y - center) ** 2 <= (center ** 2)
        bayer = np.array([
            [ 0,  8,  2, 10],
            [12,  4, 14,  6],
            [ 3, 11,  1,  9],
            [15,  7, 13,  5]
        ], dtype=np.float32)
        _NP_CACHE["np"] = np
        _NP_CACHE["mask"] = mask
        _NP_CACHE["bayer"] = bayer
    return _NP_CACHE["np"], _NP_CACHE["mask"], _NP_CACHE["bayer"]


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
    def _fit_and_composite_frame(cls, frame_img) -> Any:
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

        np, mask, _ = _get_np_resources()
        arr = np.asarray(rgb_img, dtype=np.uint8).copy()
        arr[~mask] = cls.BG_COLOR
        return arr

    @staticmethod
    def _rgb_array_to_rgb565_bytes(arr) -> bytes:
        """
        Векторизоване перетворення (H, W, 3) uint8 -> RGB565 Big-Endian
        із застосуванням Bayer 4x4 Ordered Dithering для усунення градієнтної смугастості на GC9A01.
        """
        np, _, bayer = _get_np_resources()
        h, w = arr.shape[:2]
        tile_y = (h + 3) // 4
        tile_x = (w + 3) // 4
        bayer_grid = np.tile(bayer, (tile_y, tile_x))[:h, :w]

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
    def _extract_ambilight_triad(arr) -> Tuple[Tuple[int, int, int], Tuple[int, int, int], Tuple[int, int, int]]:
        """Розумна екстракція 3-зонної палітри (Primary, Secondary, Accent) для SK6812."""
        import colorsys
        np, mask, _ = _get_np_resources()
        flat = arr[mask].reshape(-1, 3).astype(np.float32) / 255.0
        max_c = np.max(flat, axis=1)
        min_c = np.min(flat, axis=1)
        delta = max_c - min_c

        v_chan = max_c
        s_chan = np.zeros_like(max_c)
        nz = max_c > 1e-4
        s_chan[nz] = delta[nz] / max_c[nz]

        valid = (v_chan >= 0.18) & (s_chan >= 0.20)
        if np.sum(valid) < 25:
            valid = (v_chan >= 0.10) & (s_chan >= 0.10)

        if np.sum(valid) == 0:
            return ((56, 189, 248), (248, 113, 113), (250, 204, 21))

        r_c, g_c, b_c = flat[:, 0], flat[:, 1], flat[:, 2]
        h_chan = np.zeros_like(max_c)
        mr = nz & (delta > 1e-4) & (max_c == r_c)
        mg = nz & (delta > 1e-4) & (max_c == g_c)
        mb = nz & (delta > 1e-4) & (max_c == b_c)
        h_chan[mr] = ((g_c[mr] - b_c[mr]) / delta[mr]) % 6.0
        h_chan[mg] = ((b_c[mg] - r_c[mg]) / delta[mg]) + 2.0
        h_chan[mb] = ((r_c[mb] - g_c[mb]) / delta[mb]) + 4.0
        h_chan = h_chan / 6.0

        h_val = h_chan[valid]
        w_val = (s_chan[valid] * v_chan[valid]) ** 1.5

        bins = np.clip((h_val * 24.0).astype(int), 0, 23)
        bweights = np.bincount(bins, weights=w_val, minlength=24)
        h1 = (int(np.argmax(bweights)) + 0.5) / 24.0

        diff1 = np.minimum(np.abs(h_val - h1), 1.0 - np.abs(h_val - h1))
        mask2 = diff1 >= (35.0 / 360.0)
        if np.sum(mask2) >= 8:
            b2 = np.clip((h_val[mask2] * 24.0).astype(int), 0, 23)
            bw2 = np.bincount(b2, weights=w_val[mask2], minlength=24)
            h2 = (int(np.argmax(bw2)) + 0.5) / 24.0
        else:
            h2 = (h1 + 0.5) % 1.0

        diff2 = np.minimum(np.abs(h_val - h2), 1.0 - np.abs(h_val - h2))
        mask3 = (diff1 >= (30.0 / 360.0)) & (diff2 >= (30.0 / 360.0))
        if np.sum(mask3) >= 6:
            score3 = s_chan[valid][mask3] * v_chan[valid][mask3]
            h3 = float(h_val[mask3][int(np.argmax(score3))])
        else:
            h3 = (h1 + 0.25) % 1.0

        rgb1 = [int(np.clip(c * 255.0, 0, 255)) for c in colorsys.hsv_to_rgb(h1, 0.88, 1.0)]
        rgb2 = [int(np.clip(c * 255.0, 0, 255)) for c in colorsys.hsv_to_rgb(h2, 0.82, 1.0)]
        rgb3 = [int(np.clip(c * 255.0, 0, 255)) for c in colorsys.hsv_to_rgb(h3, 0.95, 1.0)]
        return (tuple(rgb1), tuple(rgb2), tuple(rgb3))

    @staticmethod
    def _extract_ambilight_color(arr) -> Tuple[int, int, int]:
        p, _, _ = MediaService._extract_ambilight_triad(arr)
        return p
    @classmethod
    def _process_media_sync(
        cls,
        raw_bytes: bytes,
        save_name: str = "",
        reaction_slot: int = 0,
    ) -> Dict[str, Any]:
        from PIL import Image

        # Автоматичне визначення відеоконтейнерів (Telegram передає GIF як MP4 або WebM)
        is_video = (len(raw_bytes) > 8 and raw_bytes[4:8] == b"ftyp") or raw_bytes[:4] == b"\x1a\x45\xdf\xa3"
        if is_video:
            return cls._process_video_note_sync(raw_bytes, save_name, reaction_slot)

        frames_bytes: List[bytes] = []
        ambi_rgb: Tuple[int, int, int] = (56, 189, 248)
        ambi2_rgb: Tuple[int, int, int] = (248, 113, 113)
        ambi3_rgb: Tuple[int, int, int] = (250, 204, 21)
        fps = settings.GIF_DEFAULT_FPS

        try:
            img = Image.open(io.BytesIO(raw_bytes))
        except Exception as img_err:
            logger.info("Image.open failed (%s), attempting video/ffmpeg extraction...", img_err)
            return cls._process_video_note_sync(raw_bytes, save_name, reaction_slot)

        with img:
            is_anim = getattr(img, "is_animated", False) and getattr(img, "n_frames", 1) > 1
            total_src_frames = getattr(img, "n_frames", 1) if is_anim else 1

            if is_anim:
                duration_ms = img.info.get("duration", 66) or 66
                fps = max(6, min(25, int(round(1000.0 / max(20.0, float(duration_ms))))))
                max_f = min(settings.GIF_MAX_FRAMES, 18)
                step = max(1, total_src_frames // max_f)
                frame_indices = list(range(0, total_src_frames, step))[:max_f]

                for idx_pos, f_idx in enumerate(frame_indices):
                    img.seek(f_idx)
                    arr = cls._fit_and_composite_frame(img)
                    if idx_pos == len(frame_indices) // 2:
                        p, s, a = cls._extract_ambilight_triad(arr)
                        ambi_rgb, ambi2_rgb, ambi3_rgb = p, s, a
                    frames_bytes.append(cls._rgb_array_to_rgb565_bytes(arr))
            else:
                arr = cls._fit_and_composite_frame(img)
                p, s, a = cls._extract_ambilight_triad(arr)
                ambi_rgb, ambi2_rgb, ambi3_rgb = p, s, a
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

        title = save_name.strip() if save_name else (f"Стікер {crc_hex[:4]}" if media_type == "sticker" else f"Гіфка {crc_hex[:4]}")
        ascii_candidate = re.sub(r"[^a-zA-Z0-9_-]", "", save_name)[:28].lower()
        if not ascii_candidate:
            safe_name = f"custom_slot_{reaction_slot}" if reaction_slot > 0 else f"live_{crc_hex[:6]}"
        else:
            safe_name = ascii_candidate

        if save_name or reaction_slot > 0:
            perm_path = os.path.join(settings.STICKERS_DIR, f"{safe_name}.mbx")
            with open(perm_path, "wb") as f:
                f.write(full_mbx)
            memory_guard.register_file_write(len(full_mbx))
            db_service.upsert_custom_media(
                name=safe_name,
                title=title,
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
            "title": title,
            "media_type": media_type,
            "crc": crc_hex,
            "frames": num_frames,
            "fps": fps,
            "size_bytes": len(full_mbx),
            "ambi_rgb": list(ambi_rgb),
            "ambi2_rgb": list(ambi2_rgb),
            "ambi3_rgb": list(ambi3_rgb),
        }

    @classmethod
    def _process_video_note_sync(
        cls,
        raw_bytes: bytes,
        save_name: str = "",
        reaction_slot: int = 0,
    ) -> Dict[str, Any]:
        import subprocess, tempfile, glob
        from PIL import Image

        with tempfile.TemporaryDirectory() as tmpdir:
            in_mp4 = os.path.join(tmpdir, "input.mp4")
            with open(in_mp4, "wb") as f:
                f.write(raw_bytes)
            out_pattern = os.path.join(tmpdir, "frame_%03d.png")

            # Екстракція до 18 кадрів при 10 fps (ліміт PSRAM/LittleFS 2.0MB)
            cmd = [
                "ffmpeg", "-y", "-i", in_mp4,
                "-vf", "fps=10,scale=240:240:force_original_aspect_ratio=increase,crop=240:240",
                "-frames:v", "18",
                out_pattern
            ]
            try:
                subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
            except Exception:
                # Фоллбек: якщо ffmpeg недоступний, пробуємо обробити як звичайне медіа
                return cls._process_media_sync(raw_bytes, save_name, reaction_slot)

            frame_files = sorted(glob.glob(os.path.join(tmpdir, "frame_*.png")))
            if not frame_files:
                return cls._process_media_sync(raw_bytes, save_name, reaction_slot)

            frames_bytes: List[bytes] = []
            ambi_rgb = (56, 189, 248)
            ambi2_rgb = (248, 113, 113)
            ambi3_rgb = (250, 204, 21)

            mid_idx = len(frame_files) // 2
            for idx_pos, fpath in enumerate(frame_files):
                with Image.open(fpath) as img:
                    arr = cls._fit_and_composite_frame(img)
                    if idx_pos == mid_idx:
                        p, s, a = cls._extract_ambilight_triad(arr)
                        ambi_rgb, ambi2_rgb, ambi3_rgb = p, s, a
                    frames_bytes.append(cls._rgb_array_to_rgb565_bytes(arr))

            num_frames = len(frames_bytes)
            fps = 10

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
                safe_name = f"vn_{crc_hex[:6]}"

            perm_path = os.path.join(settings.STICKERS_DIR, f"{safe_name}.mbx")
            with open(perm_path, "wb") as f:
                f.write(full_mbx)
            memory_guard.register_file_write(len(full_mbx))
            db_service.upsert_custom_media(
                name=safe_name,
                media_type="video_note",
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
                "media_type": "video_note",
                "crc": crc_hex,
                "frames": num_frames,
                "fps": fps,
                "size_bytes": len(full_mbx),
                "ambi_rgb": list(ambi_rgb),
                "ambi2_rgb": list(ambi2_rgb),
                "ambi3_rgb": list(ambi3_rgb),
            }

    async def process_video_note_and_enqueue(
        self,
        raw_bytes: bytes,
        base_url: str = "",
        save_name: str = "",
        reaction_slot: int = 0,
        set_as_saver: bool = False,
        media_timer_min: Optional[int] = None,
    ) -> Dict[str, Any]:
        meta = await asyncio.to_thread(self._process_video_note_sync, raw_bytes, save_name, reaction_slot)
        host = state_service.resolve_public_url(base_url)
        if "89.168.126.103" not in host:
            host = "http://89.168.126.103:8000"
        media_url = f"{host}/api/v5/media/{meta['name']}.mbx?c={meta['crc']}"
        frame_url = f"{host}/api/v1/media/frame.rgb565?v={int(time.time())}"

        cmd_dict: Dict[str, Any] = {
            "media_name": meta["name"],
            "media_url": media_url,
            "img_url": frame_url,
            "media_crc": meta["crc"],
            "media_frames": meta["frames"],
            "media_fps": meta["fps"],
            "ambi_rgb": meta["ambi_rgb"],
            "ambi2_rgb": meta.get("ambi2_rgb", [248, 113, 113]),
            "ambi3_rgb": meta.get("ambi3_rgb", [250, 204, 21]),
        }
        if set_as_saver:
            cmd_dict["set_saver"] = True
        if media_timer_min is not None:
            cmd_dict["media_timer"] = int(media_timer_min)

        state_service.enqueue_raw(cmd_dict)
        meta["media_url"] = media_url
        return meta

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
        if "89.168.126.103" not in host:
            host = "http://89.168.126.103:8000"

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
            "ambi2_rgb": meta.get("ambi2_rgb", [248, 113, 113]),
            "ambi3_rgb": meta.get("ambi3_rgb", [250, 204, 21]),
        }
        if set_as_saver:
            cmd_dict["set_saver"] = True
            db_service.set_active_saver(meta["name"])
        if media_timer_min is not None:
            cmd_dict["media_timer"] = int(media_timer_min)

        state_service.enqueue_raw(cmd_dict)
        from app.core.events import realtime_hub
        await realtime_hub.push_to_box(cmd_dict)
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
        if "89.168.126.103" not in host:
            host = "http://89.168.126.103:8000"
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
            "ambi2_rgb": [info.get("ambi_r", 56), info.get("ambi_g", 189), info.get("ambi_b", 248)],
            "ambi3_rgb": [250, 204, 21],
        }
        if set_as_saver:
            cmd_dict["set_saver"] = True
            db_service.set_active_saver(safe_name)
        if media_timer_min is not None:
            cmd_dict["media_timer"] = int(media_timer_min)

        state_service.enqueue_raw(cmd_dict)
        from app.core.events import realtime_hub
        try:
            import asyncio
            loop = asyncio.get_event_loop()
            if loop.is_running():
                asyncio.create_task(realtime_hub.push_to_box(cmd_dict))
        except Exception:
            pass
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
