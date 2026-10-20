# Файл: oracle_server/app/core/memory_guard.py
import gc
import os
import ctypes
import asyncio
from typing import List, Tuple, Dict, Any, Optional
from app.core.config import settings


class MemoryAndStorageGuard:
    """
    Zero-Idle Апаратний Охоронець 1GB RAM VPS:
    - Не крутить жодних фонових циклів у простої (0.00% CPU).
    - Запускає gc.collect() + malloc_trim(0) лише після реальних медіа-навантажень.
    - Читає системну телеметрію всієї віртуалки Linux (/proc/meminfo, LoadAvg).
    """

    __slots__ = ("_libc", "_cached_disk_bytes", "_initialized", "_trim_handle")

    def __init__(self) -> None:
        self._libc = None
        self._cached_disk_bytes: int = 0
        self._initialized: bool = False
        self._trim_handle: Optional[asyncio.TimerHandle] = None
        try:
            self._libc = ctypes.CDLL("libc.so.6")
        except Exception:
            self._libc = None

    def trim_ram(self) -> float:
        """Примусово звільняє Python GC та C-арени glibc назад ядру Linux."""
        gc.collect()
        if self._libc is not None:
            try:
                self._libc.malloc_trim(0)
            except Exception:
                pass
        return self.get_rss_mb()

    def schedule_lazy_trim(self, delay_sec: float = 10.0) -> None:
        """Планує одноразове очищення пам'яті через N секунд після активності (0% CPU у простої)."""
        try:
            loop = asyncio.get_running_loop()
            if self._trim_handle is not None:
                self._trim_handle.cancel()
            self._trim_handle = loop.call_later(delay_sec, self.trim_ram)
        except RuntimeError:
            self.trim_ram()

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

    @staticmethod
    def get_vps_system_health() -> Dict[str, Any]:
        """Зчитує реальний стан 1GB VPS із ядра Linux за 0.1 мс без зовнішніх утиліт."""
        mem_total = 1024.0
        mem_avail = 512.0
        swap_used = 0.0
        try:
            info = {}
            with open("/proc/meminfo", "r", encoding="utf-8") as f:
                for line in f:
                    parts = line.split(":")
                    if len(parts) == 2:
                        info[parts[0].strip()] = int(parts[1].strip().split()[0])
            mem_total = round(info.get("MemTotal", 1048576) / 1024.0, 1)
            mem_avail = round(info.get("MemAvailable", 524288) / 1024.0, 1)
            swap_total = info.get("SwapTotal", 0)
            swap_free = info.get("SwapFree", 0)
            swap_used = round(max(0, swap_total - swap_free) / 1024.0, 1)
        except Exception:
            pass

        load_1m = 0.0
        try:
            load_1m = round(os.getloadavg()[0], 2)
        except Exception:
            pass

        return {
            "vps_total_mb": mem_total,
            "vps_avail_mb": mem_avail,
            "vps_swap_used_mb": swap_used,
            "cpu_load_1m": load_1m,
        }

    def register_file_write(self, size_bytes: int) -> None:
        if not self._initialized:
            self.recalculate_disk_usage()
        self._cached_disk_bytes += max(0, size_bytes)
        if self._cached_disk_bytes > settings.MAX_STORAGE_BYTES:
            self.enforce_quota()
        self.schedule_lazy_trim(8.0)

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
        files: List[Tuple[float, int, str]] = []
        total_bytes = 0
        for root, _, filenames in os.walk(settings.STORAGE_DIR):
            for f in filenames:
                if f.endswith((".db", ".db-wal", ".db-shm", "latest_firmware.bin")):
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
        sys_h = self.get_vps_system_health()
        return {
            "ram_rss_mb": self.get_rss_mb(),
            "storage_used_mb": round(self._cached_disk_bytes / (1024 * 1024), 2),
            "storage_limit_mb": settings.MAX_STORAGE_BYTES // (1024 * 1024),
            **sys_h,
        }


memory_guard = MemoryAndStorageGuard()