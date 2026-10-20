import os
import re
import time
import zlib
import asyncio
from typing import Dict, Any, Optional, Tuple
from fastapi import UploadFile, HTTPException

from app.core.config import settings
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service

_CLEAN_TTS_RE = re.compile(
    r"[^\w\s\.,!\?\-—:;«»\"'°%+іїєґІЇЄҐа-яА-Яa-zA-Z0-9]",
    flags=re.UNICODE,
)


class AudioService:
    """
    Надшвидкий аудіо-конвеєр v5.0:
    - Тимчасові TTS та голосові потоки зберігаються у RAM-диску (/dev/shm) у кільцевому буфері на 4 слоти.
    - Кастомні звуки реакцій зберігаються в storage/sounds/ та індексуються в SQLite.
    """

    __slots__ = ("_ring_idx",)

    VOICE = "uk-UA-PolinaNeural"
    REACTION_SLOTS = {
        2: "fire",
        3: "zap",
        4: "cat",
        5: "ufo",
        6: "crown",
    }

    def __init__(self) -> None:
        self._ring_idx: int = 0

    @staticmethod
    def _sanitize_for_tts(text: str) -> str:
        cleaned = _CLEAN_TTS_RE.sub(" ", text)
        return re.sub(r"\s+", " ", cleaned).strip()[:300]

    def _next_ram_slot_path(self, prefix: str, ext: str) -> Tuple[str, str]:
        self._ring_idx = (self._ring_idx + 1) % 4
        fname = f"{prefix}_{self._ring_idx}.{ext}"
        return fname, os.path.join(settings.RAM_MEDIA_DIR, fname)

    async def speak_text(self, text: str, base_url: str = "", enqueue: bool = True) -> Dict[str, Any]:
        clean_text = self._sanitize_for_tts(text)
        if not clean_text:
            return {"ok": False, "error": "Empty text"}

        import edge_tts

        fname, out_path = self._next_ram_slot_path("tts", "mp3")
        communicate = edge_tts.Communicate(clean_text, self.VOICE)
        await communicate.save(out_path)
        del communicate
        memory_guard.schedule_lazy_trim(8.0)

        host = state_service.resolve_public_url(base_url)
        audio_url = f"{host}/api/v5/ram/{fname}?v={int(time.time())}"

        if enqueue:
            state_service.enqueue_raw({
                "play_url": audio_url,
                "sender": "Voice AI",
                "msg": clean_text[:90],
            })

        return {"ok": True, "audio_url": audio_url, "text": clean_text}

    async def save_voice_stream(self, raw_bytes: bytes, sender: str = "Telegram", base_url: str = "", ext: str = "ogg") -> str:
        fname, out_path = self._next_ram_slot_path("voice", ext)
        await asyncio.to_thread(self._write_bytes_sync, out_path, raw_bytes)

        host = state_service.resolve_public_url(base_url)
        voice_url = f"{host}/api/v5/ram/{fname}?v={int(time.time())}"
        state_service.enqueue_raw({
            "play_url": voice_url,
            "sender": sender[:28],
            "msg": "🎙 Голосове повідомлення",
        })
        return voice_url

    @staticmethod
    def _write_bytes_sync(path: str, data: bytes) -> None:
        with open(path, "wb") as f:
            f.write(data)

    async def save_and_play_mp3_bytes(
        self,
        raw_mp3: bytes,
        name: str = "",
        sender: str = "Telegram",
        base_url: str = "",
        reaction_slot: int = 0,
    ) -> str:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24].lower()
        host = state_service.resolve_public_url(base_url)

        if reaction_slot in self.REACTION_SLOTS:
            safe_name = self.REACTION_SLOTS[reaction_slot]

        if safe_name:
            out_path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
            await asyncio.to_thread(self._write_bytes_sync, out_path, raw_mp3)
            crc_hex = f"{(zlib.crc32(raw_mp3) & 0xFFFFFFFF):08x}"
            memory_guard.register_file_write(len(raw_mp3))
            db_service.upsert_custom_media(
                name=safe_name,
                media_type="sound",
                file_path=out_path,
                crc32_hex=crc_hex,
                frames=1,
                fps=0,
                byte_size=len(raw_mp3),
                reaction_slot=reaction_slot,
            )
            audio_url = f"{host}/api/v1/media/sound/{safe_name}.mp3?c={crc_hex}"
        else:
            fname, out_path = self._next_ram_slot_path("audio", "mp3")
            await asyncio.to_thread(self._write_bytes_sync, out_path, raw_mp3)
            audio_url = f"{host}/api/v5/ram/{fname}?v={int(time.time())}"

        state_service.enqueue_raw({
            "play_url": audio_url,
            "sender": sender[:28],
            "msg": f"🎵 Аудіо: {safe_name or 'MP3'}",
        })
        return audio_url

    def get_reaction_sound_url(self, react_code: int, base_url: str = "") -> Optional[str]:
        sound_name = self.REACTION_SLOTS.get(react_code)
        if not sound_name:
            return None
        path = os.path.join(settings.SOUNDS_DIR, f"{sound_name}.mp3")
        if os.path.exists(path):
            host = state_service.resolve_public_url(base_url)
            return f"{host}/api/v1/media/sound/{sound_name}.mp3"
        return None

    async def save_custom_sound_upload(self, name: str, file: UploadFile, base_url: str = "") -> Dict[str, Any]:
        raw = await file.read(2 * 1024 * 1024 + 1)
        if len(raw) > 2 * 1024 * 1024:
            raise HTTPException(status_code=413, detail="MP3 файл перевищує ліміт 2 МБ")
        url = await self.save_and_play_mp3_bytes(raw, name=name, sender="Web Studio", base_url=base_url)
        return {"ok": True, "sound": name, "size": len(raw), "url": url}


audio_service = AudioService()