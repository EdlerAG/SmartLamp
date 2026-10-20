import os
import time
import sqlite3
from typing import Dict, Any, List, Optional
import orjson

from app.core.config import settings


class DatabaseService:
    """
    Надлегке сховище SQLite WAL (ліміт кешу 2 МБ, mmap_size=0).
    Замінює повільний hub_state.json і керує каталогом кастомних GIF/наліпок,
    історією клімату, журналом блекаутів та налаштуваннями ШІ-персон.
    """

    __slots__ = ("_db_path",)

    def __init__(self, db_path: str = settings.DB_PATH) -> None:
        self._db_path = db_path
        self._init_schema()

    def _connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self._db_path, timeout=20.0, check_same_thread=False)
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA journal_mode=WAL;")
        conn.execute("PRAGMA synchronous=NORMAL;")
        conn.execute("PRAGMA temp_store=MEMORY;")
        conn.execute("PRAGMA cache_size=-2000;")  # Жорсткий ліміт кешу 2 МБ для 1GB VPS
        conn.execute("PRAGMA mmap_size=0;")
        return conn

    def _init_schema(self) -> None:
        os.makedirs(os.path.dirname(self._db_path), exist_ok=True)
        with self._connect() as conn:
            conn.execute("""
                CREATE TABLE IF NOT EXISTS kv_settings (
                    key VARCHAR(64) PRIMARY KEY,
                    value TEXT NOT NULL
                );
            """)
            conn.execute("""
                CREATE TABLE IF NOT EXISTS climate_history (
                    ts INTEGER PRIMARY KEY,
                    temp FLOAT NOT NULL,
                    hum FLOAT NOT NULL,
                    pres FLOAT NOT NULL,
                    light INTEGER DEFAULT 0
                );
            """)
            conn.execute("""
                CREATE TABLE IF NOT EXISTS power_outages (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    disconnected_at FLOAT NOT NULL,
                    restored_at FLOAT NOT NULL,
                    duration_sec FLOAT NOT NULL
                );
            """)
            conn.execute("""
                CREATE TABLE IF NOT EXISTS custom_media (
                    name VARCHAR(32) PRIMARY KEY,
                    media_type VARCHAR(12) NOT NULL,  -- 'sticker', 'gif', 'sound'
                    file_path TEXT NOT NULL,
                    crc32_hex VARCHAR(16) DEFAULT '',
                    frames INTEGER DEFAULT 1,
                    fps INTEGER DEFAULT 15,
                    byte_size INTEGER DEFAULT 0,
                    ambi_r INTEGER DEFAULT 56,
                    ambi_g INTEGER DEFAULT 189,
                    ambi_b INTEGER DEFAULT 248,
                    reaction_slot INTEGER DEFAULT 0,  -- 0 = галерея, 2..6 = прив'язка до 5 кнопок реакцій
                    created_at INTEGER DEFAULT 0
                );
            """)
            conn.commit()

    # --- Керування налаштуваннями (Key-Value) ---
    def get_setting(self, key: str, default: Any = None) -> Any:
        try:
            with self._connect() as conn:
                row = conn.execute("SELECT value FROM kv_settings WHERE key = ?", (key,)).fetchone()
                return orjson.loads(row["value"]) if row else default
        except Exception:
            return default

    def set_setting(self, key: str, value: Any) -> None:
        try:
            raw = orjson.dumps(value).decode("utf-8")
            with self._connect() as conn:
                conn.execute(
                    "INSERT INTO kv_settings (key, value) VALUES (?, ?) "
                    "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                    (key, raw),
                )
                conn.commit()
        except Exception:
            pass

    # --- Кліматична телеметрія ---
    def add_climate_sample(self, temp: float, hum: float, pres: float, light: int) -> None:
        now_ts = int(time.time())
        cutoff = now_ts - (30 * 86400)  # Зберігаємо 30 днів історії
        try:
            with self._connect() as conn:
                conn.execute(
                    "INSERT OR REPLACE INTO climate_history (ts, temp, hum, pres, light) VALUES (?, ?, ?, ?, ?)",
                    (now_ts, round(temp, 1), round(hum, 0), round(pres, 1), int(light)),
                )
                if now_ts % 3600 < 300:
                    conn.execute("DELETE FROM climate_history WHERE ts < ?", (cutoff,))
                conn.commit()
        except Exception:
            pass

    def get_climate_series(self, hours: int = 24) -> List[Dict[str, Any]]:
        cutoff = int(time.time()) - (hours * 3600)
        try:
            with self._connect() as conn:
                rows = conn.execute(
                    "SELECT ts, temp AS t, hum AS h, pres AS p, light AS l "
                    "FROM climate_history WHERE ts >= ? ORDER BY ts ASC",
                    (cutoff,),
                ).fetchall()
                return [dict(r) for r in rows]
        except Exception:
            return []

    # --- Блекаути ---
    def record_outage(self, disconnected_at: float, restored_at: float, duration_sec: float) -> None:
        try:
            with self._connect() as conn:
                conn.execute(
                    "INSERT INTO power_outages (disconnected_at, restored_at, duration_sec) VALUES (?, ?, ?)",
                    (round(disconnected_at, 1), round(restored_at, 1), round(duration_sec, 1)),
                )
                conn.commit()
        except Exception:
            pass

    def get_recent_outages(self, limit: int = 30) -> List[Dict[str, Any]]:
        try:
            with self._connect() as conn:
                rows = conn.execute(
                    "SELECT disconnected_at, restored_at, duration_sec "
                    "FROM power_outages ORDER BY id DESC LIMIT ?",
                    (limit,),
                ).fetchall()
                return [dict(r) for r in rows]
        except Exception:
            return []

    # --- Каталог кастомних наліпок, GIF (.mbx) та звуків ---
    def upsert_custom_media(
        self,
        name: str,
        media_type: str,
        file_path: str,
        crc32_hex: str,
        frames: int,
        fps: int,
        byte_size: int,
        ambi_rgb: tuple[int, int, int] = (56, 189, 248),
        reaction_slot: int = 0,
    ) -> None:
        try:
            with self._connect() as conn:
                if reaction_slot > 0:
                    # Звільняємо слот реакції від попередньої гіфки
                    conn.execute(
                        "UPDATE custom_media SET reaction_slot = 0 WHERE reaction_slot = ? AND media_type != 'sound'",
                        (reaction_slot,),
                    )
                conn.execute(
                    """
                    INSERT INTO custom_media
                    (name, media_type, file_path, crc32_hex, frames, fps, byte_size, ambi_r, ambi_g, ambi_b, reaction_slot, created_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    ON CONFLICT(name) DO UPDATE SET
                        media_type=excluded.media_type,
                        file_path=excluded.file_path,
                        crc32_hex=excluded.crc32_hex,
                        frames=excluded.frames,
                        fps=excluded.fps,
                        byte_size=excluded.byte_size,
                        ambi_r=excluded.ambi_r,
                        ambi_g=excluded.ambi_g,
                        ambi_b=excluded.ambi_b,
                        reaction_slot=CASE WHEN excluded.reaction_slot > 0 THEN excluded.reaction_slot ELSE custom_media.reaction_slot END,
                        created_at=excluded.created_at
                    """,
                    (
                        name, media_type, file_path, crc32_hex, int(frames), int(fps),
                        int(byte_size), int(ambi_rgb[0]), int(ambi_rgb[1]), int(ambi_rgb[2]),
                        int(reaction_slot), int(time.time()),
                    ),
                )
                conn.commit()
        except Exception:
            pass

    def bind_media_to_reaction_slot(self, name: str, slot: int) -> bool:
        """Прив'язує будь-яку кастомну гіфку чи наліпку до однієї з 5 кнопок реакцій (2..6)."""
        try:
            with self._connect() as conn:
                conn.execute("UPDATE custom_media SET reaction_slot = 0 WHERE reaction_slot = ?", (slot,))
                cur = conn.execute("UPDATE custom_media SET reaction_slot = ? WHERE name = ?", (slot, name))
                conn.commit()
                return cur.rowcount > 0
        except Exception:
            return False

    def get_media_by_name(self, name: str) -> Optional[Dict[str, Any]]:
        try:
            with self._connect() as conn:
                row = conn.execute("SELECT * FROM custom_media WHERE name = ?", (name,)).fetchone()
                return dict(row) if row else None
        except Exception:
            return None

    def get_media_for_reaction_slot(self, slot: int) -> Optional[Dict[str, Any]]:
        try:
            with self._connect() as conn:
                row = conn.execute(
                    "SELECT * FROM custom_media WHERE reaction_slot = ? AND media_type IN ('sticker', 'gif') LIMIT 1",
                    (slot,),
                ).fetchone()
                return dict(row) if row else None
        except Exception:
            return None

    def list_custom_media(self) -> List[Dict[str, Any]]:
        try:
            with self._connect() as conn:
                rows = conn.execute(
                    "SELECT name, media_type, crc32_hex, frames, fps, byte_size, "
                    "ambi_r, ambi_g, ambi_b, reaction_slot, created_at "
                    "FROM custom_media ORDER BY reaction_slot DESC, created_at DESC"
                ).fetchall()
                return [dict(r) for r in rows]
        except Exception:
            return []

    def delete_custom_media(self, name: str) -> Optional[str]:
        try:
            with self._connect() as conn:
                row = conn.execute("SELECT file_path FROM custom_media WHERE name = ?", (name,)).fetchone()
                if not row:
                    return None
                conn.execute("DELETE FROM custom_media WHERE name = ?", (name,))
                conn.commit()
                return str(row["file_path"])
        except Exception:
            return None


db_service = DatabaseService()