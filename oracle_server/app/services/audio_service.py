import gc
import os
import re
import time
from typing import Dict, Any, Optional
from fastapi import UploadFile, HTTPException
from app.core.config import settings
from app.services.state_service import state_service
from app.services.storage_service import storage_manager

_CLEAN_TTS_RE = re.compile(
    r"[^\w\s\.,!\?\-—:;«»\"'°%+іїєґІЇЄҐа-яА-Яa-zA-Z0-9]",
    flags=re.UNICODE,
)


class AudioService:
    __slots__ = ()
    TTS_FILE = os.path.join(settings.MEDIA_DIR, "tts_latest.mp3")
    VOICE_OGG_FILE = os.path.join(settings.MEDIA_DIR, "voice_latest.ogg")
    VOICE = "uk-UA-PolinaNeural"

    REACTION_SOUNDS = {
        2: "fire",
        3: "zap",
        4: "cat",
        5: "ufo",
        6: "crown",
    }

    @staticmethod
    def _sanitize_for_tts(text: str) -> str:
        cleaned = _CLEAN_TTS_RE.sub(" ", text)
        return re.sub(r"\s+", " ", cleaned).strip()[:280]

    async def speak_text(self, text: str, base_url: str = "", enqueue: bool = True) -> Dict[str, Any]:
        clean_text = self._sanitize_for_tts(text)
        if not clean_text:
            return {"ok": False, "error": "Empty text"}

        import edge_tts

        communicate = edge_tts.Communicate(clean_text, self.VOICE)
        await communicate.save(self.TTS_FILE)
        del communicate
        gc.collect()
        storage_manager.enforce_quota()

        host = state_service.resolve_public_url(base_url)
        audio_url = f"{host}/api/v1/media/tts.mp3?v={int(time.time())}"

        if enqueue:
            state_service.enqueue_raw({
                "play_url": audio_url,
                "sender": "Voice AI",
                "msg": clean_text[:60],
            })

        return {"ok": True, "audio_url": audio_url, "text": clean_text}

    def save_voice_ogg(self, raw_ogg: bytes, sender: str = "Telegram", base_url: str = "") -> str:
        with open(self.VOICE_OGG_FILE, "wb") as f:
            f.write(raw_ogg)
        storage_manager.enforce_quota()

        host = state_service.resolve_public_url(base_url)
        voice_url = f"{host}/api/v1/media/voice.ogg?v={int(time.time())}"
        state_service.enqueue_raw({
            "play_url": voice_url,
            "sender": sender[:24],
            "msg": "VOICE MESSAGE",
        })
        return voice_url

    def save_and_play_mp3_bytes(self, raw_mp3: bytes, name: str = "", sender: str = "Telegram", base_url: str = "") -> str:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24]
        host = state_service.resolve_public_url(base_url)

        if safe_name in ("fire", "zap", "cat", "ufo", "crown"):
            out_path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
            with open(out_path, "wb") as f:
                f.write(raw_mp3)
            audio_url = f"{host}/api/v1/media/sound/{safe_name}.mp3?v={int(time.time())}"
        else:
            with open(self.TTS_FILE, "wb") as f:
                f.write(raw_mp3)
            audio_url = f"{host}/api/v1/media/tts.mp3?v={int(time.time())}"

        storage_manager.enforce_quota()
        state_service.enqueue_raw({
            "play_url": audio_url,
            "sender": sender[:24],
            "msg": f"AUDIO: {safe_name or 'MP3'}",
        })
        return audio_url

    def get_reaction_sound_url(self, react_code: int, base_url: str = "") -> Optional[str]:
        sound_name = self.REACTION_SOUNDS.get(react_code)
        if not sound_name:
            return None
        path = os.path.join(settings.SOUNDS_DIR, f"{sound_name}.mp3")
        if os.path.exists(path):
            host = state_service.resolve_public_url(base_url)
            return f"{host}/api/v1/media/sound/{sound_name}.mp3"
        return None

    async def save_custom_sound(self, name: str, file: UploadFile) -> Dict[str, Any]:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24]
        if not safe_name:
            raise HTTPException(status_code=400, detail="Невірна назва звуку")
        raw = await file.read(2 * 1024 * 1024 + 1)
        if len(raw) > 2 * 1024 * 1024:
            raise HTTPException(status_code=413, detail="MP3 файл перевищує 2 МБ")

        out_path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
        with open(out_path, "wb") as f:
            f.write(raw)

        storage_manager.enforce_quota()
        return {"ok": True, "sound": safe_name, "size": len(raw)}


audio_service = AudioService()