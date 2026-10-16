import gc
import os
import re
import time
from typing import Dict, Any, Optional
from fastapi import UploadFile, HTTPException
from app.core.config import settings
from app.services.state_service import state_service
from app.services.storage_service import storage_manager


class AudioService:
    __slots__ = ()
    TTS_FILE = os.path.join(settings.MEDIA_DIR, "tts_latest.mp3")
    VOICE = "uk-UA-PolinaNeural"

    REACTION_SOUNDS = {
        2: "fire",
        3: "zap",
        4: "cat",
        5: "ufo",
        6: "crown",
    }

    async def speak_text(self, text: str, base_url: str = "", enqueue: bool = True) -> Dict[str, Any]:
        clean_text = text.strip()[:240]
        if not clean_text:
            return {"ok": False, "error": "Empty text"}

        import edge_tts

        communicate = edge_tts.Communicate(clean_text, self.VOICE)
        await communicate.save(self.TTS_FILE)
        del communicate
        gc.collect()
        storage_manager.enforce_quota()

        host = (base_url or settings.PUBLIC_SERVER_URL).rstrip("/")
        audio_url = f"{host}/api/v1/media/tts.mp3?v={int(time.time())}"

        if enqueue:
            state_service.enqueue_raw({
                "play_url": audio_url,
                "sender": "Voice AI",
                "msg": clean_text[:60],
            })

        return {"ok": True, "audio_url": audio_url, "text": clean_text}

    def get_reaction_sound_url(self, react_code: int, base_url: str = "") -> Optional[str]:
        sound_name = self.REACTION_SOUNDS.get(react_code)
        if not sound_name:
            return None
        path = os.path.join(settings.SOUNDS_DIR, f"{sound_name}.mp3")
        if os.path.exists(path):
            host = (base_url or settings.PUBLIC_SERVER_URL).rstrip("/")
            return f"{host}/api/v1/media/sound/{sound_name}.mp3"
        return None

    async def save_custom_sound(self, name: str, file: UploadFile) -> Dict[str, Any]:
        safe_name = re.sub(r"[^a-zA-Z0-9_-]", "", name)[:24]
        if not safe_name:
            raise HTTPException(status_code=400, detail="Невірна назва звуку")
        raw = await file.read(2 * 1024 * 1024)  # До 2 МБ на звуковий ефект
        if len(raw) > 2 * 1024 * 1024:
            raise HTTPException(status_code=413, detail="MP3 файл перевищує 2 МБ")

        out_path = os.path.join(settings.SOUNDS_DIR, f"{safe_name}.mp3")
        with open(out_path, "wb") as f:
            f.write(raw)

        storage_manager.enforce_quota()
        return {"ok": True, "sound": safe_name, "size": len(raw)}


audio_service = AudioService()