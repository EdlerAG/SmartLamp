import os
import re
import time
import zlib
import asyncio
from typing import Dict, Any, Optional, Tuple, List
from fastapi import UploadFile, HTTPException

from app.core.config import settings
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service

_CLEAN_TTS_RE = re.compile(
    r"[^\w\s\.,!\?\-—:;«»\"'°%+іїєґІЇЄҐа-яА-Яa-zA-Z0-9]",
    flags=re.UNICODE,
)


class AudioService:
    """
    Надшвидкий аудіо-конвеєр v4.5:
    - Підтримка власного клонованого голосу Ярослава (ElevenLabs / Fish Audio API).
    - Базовий безкоштовний нейро-голос Edge-TTS з можливістю перемикання між Ostap та Polina.
    - Тимчасові TTS та голосові потоки зберігаються у RAM-диску (/dev/shm) у кільцевому буфері на 4 слоти.
    - Банк голосових заміток та пресетів у storage/sounds/ з миттєвим повтором.
    """

    __slots__ = ("_ring_idx",)

    DEFAULT_VOICE = "uk-UA-OstapNeural" # Чоловічий голос за замовчуванням
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

    def get_configured_tts_provider(self) -> str:
        return db_service.get_setting("tts_provider", settings.TTS_PROVIDER)

    def get_configured_edge_voice(self) -> str:
        return db_service.get_setting("tts_edge_voice", settings.TTS_EDGE_VOICE)

    def get_voice_clone_credentials(self) -> Tuple[str, str]:
        api_key = db_service.get_setting("voice_clone_api_key", settings.VOICE_CLONE_API_KEY)
        voice_id = db_service.get_setting("voice_clone_id", settings.VOICE_CLONE_ID)
        return api_key, voice_id

    async def speak_text(self, text: str, base_url: str = "", enqueue: bool = True) -> Dict[str, Any]:
        clean_text = self._sanitize_for_tts(text)
        if not clean_text:
            return {"ok": False, "error": "Empty text"}

        provider = self.get_configured_tts_provider()
        api_key, voice_id = self.get_voice_clone_credentials()

        fname, out_path = self._next_ram_slot_path("tts", "mp3")
        generated_successfully = False

        # 1. Провайдер ElevenLabs (Клонований голос Ярослава)
        if provider == "elevenlabs" and api_key and voice_id:
            try:
                url = f"https://api.elevenlabs.io/v1/text-to-speech/{voice_id}"
                headers = {
                    "xi-api-key": api_key,
                    "Content-Type": "application/json",
                    "Accept": "audio/mpeg",
                }
                payload = {
                    "text": clean_text,
                    "model_id": "eleven_multilingual_v2",
                    "voice_settings": {
                        "stability": 0.5,
                        "similarity_boost": 0.8,
                    },
                }
                resp = await http_pool.get().post(url, json=payload, headers=headers, timeout=18.0)
                if resp.status_code == 200 and len(resp.content) > 100:
                    await asyncio.to_thread(self._write_bytes_sync, out_path, resp.content)
                    generated_successfully = True
            except Exception:
                generated_successfully = False

        # 2. Провайдер Fish Audio API (Альтернативний клон голосу)
        elif provider == "fish_audio" and api_key and voice_id:
            try:
                url = "https://api.fish.audio/v1/tts"
                headers = {
                    "Authorization": f"Bearer {api_key}",
                    "Content-Type": "application/json",
                }
                payload = {
                    "text": clean_text,
                    "reference_id": voice_id,
                    "format": "mp3",
                }
                resp = await http_pool.get().post(url, json=payload, headers=headers, timeout=18.0)
                if resp.status_code == 200 and len(resp.content) > 100:
                    await asyncio.to_thread(self._write_bytes_sync, out_path, resp.content)
                    generated_successfully = True
            except Exception:
                generated_successfully = False

        # 3. Дефолтний високоякісний Edge-TTS (Ostap або Polina)
        if not generated_successfully:
            import edge_tts
            voice = self.get_configured_edge_voice() or self.DEFAULT_VOICE
            communicate = edge_tts.Communicate(clean_text, voice)
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

        return {
            "ok": True,
            "audio_url": audio_url,
            "text": clean_text,
            "provider": provider if generated_successfully else "edge",
        }

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

    async def save_voice_preset(self, name: str, raw_bytes: bytes, base_url: str = "", ext: str = "ogg") -> str:
        """Зберігає голосове повідомлення у постійний банк storage/sounds/voice_{name}.ogg для миттєвого виклику."""
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24].lower() or f"v_{int(time.time())}"
        out_path = os.path.join(settings.SOUNDS_DIR, f"voice_{safe_name}.{ext}")
        await asyncio.to_thread(self._write_bytes_sync, out_path, raw_bytes)

        crc_hex = f"{(zlib.crc32(raw_bytes) & 0xFFFFFFFF):08x}"
        db_service.upsert_custom_media(
            name=f"voice_{safe_name}",
            media_type="voice",
            file_path=out_path,
            crc32_hex=crc_hex,
            frames=1,
            fps=0,
            byte_size=len(raw_bytes),
            reaction_slot=0,
        )
        host = state_service.resolve_public_url(base_url)
        return f"{host}/api/v1/media/sound/voice_{safe_name}.{ext}?c={crc_hex}"

    def list_voice_presets(self) -> List[Dict[str, Any]]:
        all_media = db_service.list_custom_media()
        return [m for m in all_media if m.get("media_type") == "voice" or m.get("name", "").startswith("voice_")]

    async def play_voice_preset(self, name: str, base_url: str = "") -> bool:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:32].lower()
        if not safe_name.startswith("voice_"):
            safe_name = f"voice_{safe_name}"

        candidate_exts = ["ogg", "mp3", "wav"]
        for ext in candidate_exts:
            path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.{ext}")
            if os.path.exists(path):
                host = state_service.resolve_public_url(base_url)
                url = f"{host}/api/v1/media/sound/{safe_name}.{ext}?v={int(time.time())}"
                state_service.enqueue_raw({
                    "play_url": url,
                    "sender": "Ярослав",
                    "msg": f"🎙 Голос: {safe_name.replace('voice_', '')}",
                })
                return True
        return False

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
