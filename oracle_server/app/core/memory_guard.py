import gc
import os
import ctypes
from typing import List, Tuple, Dict, Any
from app.core.config import settings


class MemoryAndStorageGuard:
    """
    Апаратний захист 1GB RAM VPS:
    1. Повертає фрагментовані сторінки C-кучі (Pillow/NumPy/EdgeTTS) назад ядру Linux через malloc_trim(0).
    2. Веде O(1) облік дискової квоти без блокуючих викликів os.walk() на кожному запиті.
    """

    __slots__ = ("_libc", "_cached_disk_bytes", "_initialized")

    def __init__(self) -> None:
        self._libc = None
        self._cached_disk_bytes: int = 0
        self._initialized: bool = False
        try:
            self._libc = ctypes.CDLL("libc.so.6")
        except Exception:
            self._libc = None

    def trim_ram(self) -> float:
        """Збирає сміття Python та примусово стискає C-арени glibc. Повертає поточний VmRSS у МБ."""
        gc.collect()
        if self._libc is not None:
            try:
                self._libc.malloc_trim(0)
            except Exception:
                pass
        return self.get_rss_mb()

    @staticmethod
    def get_rss_mb() -> float:
        try:
            with open("/proc/self/status", "r", encoding="utf-8") as f:
                for line in f:
                    if line.startswith("VmRSS:"):
                        return round(int(line.split()[1]) / 1024.0, 2)
        except Exception:
            pass
        return 0.0

    def register_file_write(self, size_bytes: int) -> None:
        """Миттєвий O(1) облік нового файлу та запуск LRU-очищення лише при реальному перевищенні ліміту."""
        if not self._initialized:
            self.recalculate_disk_usage()
        self._cached_disk_bytes += max(0, size_bytes)
        if self._cached_disk_bytes > settings.MAX_STORAGE_BYTES:
            self.enforce_quota()

    def recalculate_disk_usage(self) -> int:
        total = 0
        for root, _, filenames in os.walk(settings.STORAGE_DIR):
            for fname in filenames:
                try:
                    total += os.path.getsize(os.path.join(root, fname))
                except OSError:
                    pass
        self._cached_disk_bytes = total
        self._initialized = True
        return total

    def enforce_quota(self) -> int:
        """LRU-очищення: видаляє найстаріші медіафайли, якщо квота перевищена."""
        files: List[Tuple[float, int, str]] = []
        total_bytes = 0

        for root, _, filenames in os.walk(settings.STORAGE_DIR):
            for f in filenames:
                if f.endswith(".db") or f.endswith(".db-wal") or f.endswith(".db-shm") or f.endswith("latest_firmware.bin"):
                    continue
                full_path = os.path.join(root, f)
                try:
                    st = os.stat(full_path)
                    total_bytes += st.st_size
                    files.append((st.st_mtime, st.st_size, full_path))
                except OSError:
                    pass

        self._cached_disk_bytes = total_bytes
        self._initialized = True

        if total_bytes <= settings.MAX_STORAGE_BYTES:
            return 0

        files.sort(key=lambda x: x[0])
        removed_bytes = 0
        target_bytes = int(settings.MAX_STORAGE_BYTES * 0.80)

        for _, size, path in files:
            try:
                os.remove(path)
                total_bytes -= size
                removed_bytes += size
                if total_bytes <= target_bytes:
                    break
            except OSError:
                pass

        self._cached_disk_bytes = total_bytes
        return removed_bytes

    def get_telemetry(self) -> Dict[str, Any]:
        if not self._initialized:
            self.recalculate_disk_usage()
        return {
            "ram_rss_mb": self.get_rss_mb(),
            "storage_used_mb": round(self._cached_disk_bytes / (1024 * 1024), 2),
            "storage_limit_mb": settings.MAX_STORAGE_BYTES // (1024 * 1024),
        }


memory_guard = MemoryAndStorageGuard()