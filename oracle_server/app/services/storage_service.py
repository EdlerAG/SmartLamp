import os
from typing import List, Tuple, Dict, Any
from app.core.config import settings


class StorageManager:
    """Контролює квоту диска (LRU-очищення) та веде каталог завантажених звуків і наліпок."""

    __slots__ = ()

    def enforce_quota(self) -> int:
        """Перевіряє загальний обсяг storage/ і видаляє найстаріші файли кешу при перевищенні ліміту."""
        files: List[Tuple[float, int, str]] = []
        total_bytes = 0

        for root, _, filenames in os.walk(settings.STORAGE_DIR):
            for f in filenames:
                if f.endswith("hub_state.json") or f.endswith("latest_firmware.bin"):
                    continue
                full_path = os.path.join(root, f)
                try:
                    st = os.stat(full_path)
                    total_bytes += st.st_size
                    files.append((st.st_mtime, st.st_size, full_path))
                except OSError:
                    pass

        if total_bytes <= settings.MAX_STORAGE_BYTES:
            return 0

        # Сортуємо від найстаріших до найновіших (LRU)
        files.sort(key=lambda x: x[0])
        removed_bytes = 0
        for _, size, path in files:
            try:
                os.remove(path)
                total_bytes -= size
                removed_bytes += size
                if total_bytes <= int(settings.MAX_STORAGE_BYTES * 0.8):
                    break
            except OSError:
                pass
        return removed_bytes

    def get_storage_stats(self) -> Dict[str, Any]:
        total = 0
        sounds = []
        stickers = []
        for root, _, filenames in os.walk(settings.STORAGE_DIR):
            for f in filenames:
                p = os.path.join(root, f)
                try:
                    total += os.path.getsize(p)
                except OSError:
                    pass
                if root == settings.SOUNDS_DIR and f.endswith(".mp3"):
                    sounds.append(f[:-4])
                elif root == settings.STICKERS_DIR and f.endswith(".rgb565"):
                    stickers.append(f[:-7])

        return {
            "used_mb": round(total / (1024 * 1024), 2),
            "limit_mb": settings.MAX_STORAGE_BYTES // (1024 * 1024),
            "saved_sounds": sorted(sounds),
            "saved_stickers": sorted(stickers),
        }


storage_manager = StorageManager()