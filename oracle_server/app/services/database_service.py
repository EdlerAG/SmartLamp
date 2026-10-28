# Файл: oracle_server/app/services/database_service.py
import os
import time
import sqlite3
import hashlib
from typing import Dict, Any, List, Optional
import orjson

from app.core.config import settings
from app.core.logger import logger


class DatabaseService:
    """
    Zero-I/O SQLite WAL сервіс:
    - Тримає єдине відкрите з'єднання (жодних витрат на перевідкриття файлу та PRAGMA).
    - Кешує каталог кастомних гіфок/наліпок та налаштування в RAM (читання за 0.001 мс без диска).
    - Підтримка таблиці безпечних криптографічних сесій web_sessions.
    """

    __slots__ = ("_db_path", "_conn", "_media_cache", "_kv_cache")

    def __init__(self, db_path: str = settings.DB_PATH) -> None:
        self._db_path = db_path
        self._conn: Optional[sqlite3.Connection] = None
        self._media_cache: Optional[List[Dict[str, Any]]] = None
        self._kv_cache: Dict[str, Any] = {}
        self._init_schema()

    def _get_conn(self) -> sqlite3.Connection:
        if self._conn is None:
            os.makedirs(os.path.dirname(self._db_path), exist_ok=True)
            conn = sqlite3.connect(self._db_path, timeout=20.0, check_same_thread=False)
            conn.row_factory = sqlite3.Row
            conn.execute("PRAGMA journal_mode=WAL;")
            conn.execute("PRAGMA synchronous=NORMAL;")
            conn.execute("PRAGMA temp_store=MEMORY;")
            conn.execute("PRAGMA cache_size=-1500;")  # 1.5 МБ кешу
            conn.execute("PRAGMA mmap_size=0;")
            self._conn = conn
        return self._conn

    def close(self) -> None:
        if self._conn is not None:
            try:
                self._conn.execute("PRAGMA wal_checkpoint(TRUNCATE);")
                self._conn.close()
            except Exception as e:
                logger.warning("DB close warning: %s", e)
            self._conn = None

    def _init_schema(self) -> None:
        conn = self._get_conn()
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
                media_type VARCHAR(12) NOT NULL,
                file_path TEXT NOT NULL,
                crc32_hex VARCHAR(16) DEFAULT '',
                frames INTEGER DEFAULT 1,
                fps INTEGER DEFAULT 15,
                byte_size INTEGER DEFAULT 0,
                ambi_r INTEGER DEFAULT 56,
                ambi_g INTEGER DEFAULT 189,
                ambi_b INTEGER DEFAULT 248,
                reaction_slot INTEGER DEFAULT 0,
                created_at INTEGER DEFAULT 0
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS web_sessions (
                token_hash VARCHAR(64) PRIMARY KEY,
                role VARCHAR(16) NOT NULL,
                created_at FLOAT NOT NULL,
                expires_at FLOAT NOT NULL
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS calendar_events (
                id VARCHAR(64) PRIMARY KEY,
                summary TEXT NOT NULL,
                start_ts FLOAT NOT NULL,
                end_ts FLOAT DEFAULT 0,
                time_str VARCHAR(16) NOT NULL,
                date_str VARCHAR(16) NOT NULL,
                source VARCHAR(16) DEFAULT 'manual',
                created_at FLOAT NOT NULL
            );
        """)
        conn.commit()

    # --- СЕСІЇ ТА АВТОРИЗАЦІЯ ---
    def create_session(self, token: str, role: str = "admin", duration_sec: float = 86400 * 14) -> str:
        token_hash = hashlib.sha256(token.encode("utf-8")).hexdigest()
        now = time.time()
        expires = now + duration_sec
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT INTO web_sessions (token_hash, role, created_at, expires_at) VALUES (?, ?, ?, ?) "
                "ON CONFLICT(token_hash) DO UPDATE SET role=excluded.role, expires_at=excluded.expires_at",
                (token_hash, role, now, expires),
            )
            conn.commit()
            return token
        except Exception as e:
            logger.error("Failed to create session: %s", e)
            return token

    def validate_session(self, token: str) -> Optional[str]:
        if not token:
            return None
        token_hash = hashlib.sha256(token.encode("utf-8")).hexdigest()
        now = time.time()
        try:
            row = self._get_conn().execute(
                "SELECT role, expires_at FROM web_sessions WHERE token_hash = ?",
                (token_hash,),
            ).fetchone()
            if row and row["expires_at"] > now:
                return str(row["role"])
            if row:
                self.delete_session(token)
            return None
        except Exception as e:
            logger.warning("Session validation error: %s", e)
            return None

    def delete_session(self, token: str) -> None:
        token_hash = hashlib.sha256(token.encode("utf-8")).hexdigest()
        try:
            conn = self._get_conn()
            conn.execute("DELETE FROM web_sessions WHERE token_hash = ?", (token_hash,))
            conn.commit()
        except Exception as e:
            logger.warning("Session delete error: %s", e)

    def cleanup_expired_sessions(self) -> None:
        now = time.time()
        try:
            conn = self._get_conn()
            conn.execute("DELETE FROM web_sessions WHERE expires_at < ?", (now,))
            conn.commit()
        except Exception:
            pass

    # --- KV НАЛАШТУВАННЯ ---
    def get_setting(self, key: str, default: Any = None) -> Any:
        if key in self._kv_cache:
            return self._kv_cache[key]
        try:
            row = self._get_conn().execute("SELECT value FROM kv_settings WHERE key = ?", (key,)).fetchone()
            val = orjson.loads(row["value"]) if row else default
            self._kv_cache[key] = val
            return val
        except Exception as e:
            logger.warning("get_setting error for '%s': %s", key, e)
            return default

    def set_setting(self, key: str, value: Any) -> None:
        self._kv_cache[key] = value
        try:
            raw = orjson.dumps(value).decode("utf-8")
            conn = self._get_conn()
            conn.execute(
                "INSERT INTO kv_settings (key, value) VALUES (?, ?) "
                "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                (key, raw),
            )
            conn.commit()
        except Exception as e:
            logger.error("set_setting error for '%s': %s", key, e)

    # --- КЛІМАТ ТА БЛЕКАУТИ ---
    def add_climate_sample(self, temp: float, hum: float, pres: float, light: int) -> None:
        now_ts = int(time.time())
        cutoff = now_ts - (30 * 86400)
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT OR REPLACE INTO climate_history (ts, temp, hum, pres, light) VALUES (?, ?, ?, ?, ?)",
                (now_ts, round(temp, 1), round(hum, 0), round(pres, 1), int(light)),
            )
            if now_ts % 3600 < 300:
                conn.execute("DELETE FROM climate_history WHERE ts < ?", (cutoff,))
            conn.commit()
        except Exception as e:
            logger.warning("add_climate_sample error: %s", e)

    def get_climate_series(self, hours: int = 24) -> List[Dict[str, Any]]:
        cutoff = int(time.time()) - (hours * 3600)
        try:
            rows = self._get_conn().execute(
                "SELECT ts, temp AS t, hum AS h, pres AS p, light AS l "
                "FROM climate_history WHERE ts >= ? ORDER BY ts ASC",
                (cutoff,),
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as e:
            logger.warning("get_climate_series error: %s", e)
            return []

    def record_outage(self, disconnected_at: float, restored_at: float, duration_sec: float) -> None:
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT INTO power_outages (disconnected_at, restored_at, duration_sec) VALUES (?, ?, ?)",
                (round(disconnected_at, 1), round(restored_at, 1), round(duration_sec, 1)),
            )
            conn.commit()
        except Exception as e:
            logger.error("record_outage error: %s", e)

    def get_recent_outages(self, limit: int = 30) -> List[Dict[str, Any]]:
        try:
            rows = self._get_conn().execute(
                "SELECT disconnected_at, restored_at, duration_sec "
                "FROM power_outages ORDER BY id DESC LIMIT ?",
                (limit,),
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as e:
            logger.warning("get_recent_outages error: %s", e)
            return []

    # --- МЕДІА ТА АНІМАЦІЇ ---
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
            conn = self._get_conn()
            if reaction_slot > 0:
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
            self._media_cache = None
        except Exception as e:
            logger.error("upsert_custom_media error: %s", e)

    def bind_media_to_reaction_slot(self, name: str, slot: int) -> bool:
        try:
            conn = self._get_conn()
            conn.execute("UPDATE custom_media SET reaction_slot = 0 WHERE reaction_slot = ?", (slot,))
            cur = conn.execute("UPDATE custom_media SET reaction_slot = ? WHERE name = ?", (slot, name))
            conn.commit()
            self._media_cache = None
            return cur.rowcount > 0
        except Exception as e:
            logger.error("bind_media_to_reaction_slot error: %s", e)
            return False

    def get_media_by_name(self, name: str) -> Optional[Dict[str, Any]]:
        try:
            row = self._get_conn().execute("SELECT * FROM custom_media WHERE name = ?", (name,)).fetchone()
            return dict(row) if row else None
        except Exception as e:
            logger.warning("get_media_by_name error: %s", e)
            return None

    def get_media_for_reaction_slot(self, slot: int) -> Optional[Dict[str, Any]]:
        try:
            row = self._get_conn().execute(
                "SELECT * FROM custom_media WHERE reaction_slot = ? AND media_type IN ('sticker', 'gif') LIMIT 1",
                (slot,),
            ).fetchone()
            return dict(row) if row else None
        except Exception as e:
            logger.warning("get_media_for_reaction_slot error: %s", e)
            return None

    def list_custom_media(self) -> List[Dict[str, Any]]:
        if self._media_cache is not None:
            return self._media_cache
        try:
            rows = self._get_conn().execute(
                "SELECT name, media_type, crc32_hex, frames, fps, byte_size, "
                "ambi_r, ambi_g, ambi_b, reaction_slot, created_at "
                "FROM custom_media ORDER BY reaction_slot DESC, created_at DESC"
            ).fetchall()
            self._media_cache = [dict(r) for r in rows]
            return self._media_cache
        except Exception as e:
            logger.warning("list_custom_media error: %s", e)
            return []

    def delete_custom_media(self, name: str) -> Optional[str]:
        try:
            conn = self._get_conn()
            row = conn.execute("SELECT file_path FROM custom_media WHERE name = ?", (name,)).fetchone()
            if not row:
                return None
            conn.execute("DELETE FROM custom_media WHERE name = ?", (name,))
            conn.commit()
            self._media_cache = None
            return str(row["file_path"])
        except Exception as e:
            logger.error("delete_custom_media error: %s", e)
            return None

    # --- КАЛЕНДАР ТА РОЗКЛАД НАСТІ (УНІФІКОВАНА СИНХРОНІЗАЦІЯ) ---
    def save_calendar_event(
        self,
        event_id: str,
        summary: str,
        start_ts: float,
        end_ts: float = 0.0,
        time_str: str = "--:--",
        date_str: str = "",
        source: str = "manual",
    ) -> Dict[str, Any]:
        now = time.time()
        try:
            conn = self._get_conn()
            conn.execute(
                """
                INSERT INTO calendar_events (id, summary, start_ts, end_ts, time_str, date_str, source, created_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(id) DO UPDATE SET
                    summary = excluded.summary,
                    start_ts = excluded.start_ts,
                    end_ts = excluded.end_ts,
                    time_str = excluded.time_str,
                    date_str = excluded.date_str,
                    source = excluded.source
                """,
                (event_id, summary, start_ts, end_ts, time_str, date_str, source, now),
            )
            conn.commit()
            return {
                "id": event_id,
                "summary": summary,
                "start_ts": start_ts,
                "end_ts": end_ts,
                "time_str": time_str,
                "date_str": date_str,
                "source": source,
            }
        except Exception as e:
            logger.error("save_calendar_event error: %s", e)
            return {"id": event_id, "summary": summary}

    def delete_calendar_event(self, event_id: str) -> bool:
        try:
            conn = self._get_conn()
            cur = conn.execute("DELETE FROM calendar_events WHERE id = ?", (event_id,))
            conn.commit()
            return cur.rowcount > 0
        except Exception as e:
            logger.error("delete_calendar_event error: %s", e)
            return False

    def list_upcoming_calendar_events(self, limit: int = 20) -> List[Dict[str, Any]]:
        now = time.time()
        # Показуємо події, які ще не завершились (з запасом 30 хвилин після початку)
        cutoff = now - 1800.0
        try:
            conn = self._get_conn()
            rows = conn.execute(
                """
                SELECT id, summary, start_ts, end_ts, time_str, date_str, source
                FROM calendar_events
                WHERE start_ts >= ?
                ORDER BY start_ts ASC
                LIMIT ?
                """,
                (cutoff, limit),
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as e:
            logger.warning("list_upcoming_calendar_events error: %s", e)
            return []

    def get_next_calendar_event(self) -> Optional[Dict[str, Any]]:
        now = time.time()
        cutoff = now - 600.0  # подія ще активна протягом 10 хв після старту
        try:
            conn = self._get_conn()
            row = conn.execute(
                """
                SELECT id, summary, start_ts, end_ts, time_str, date_str, source
                FROM calendar_events
                WHERE start_ts >= ?
                ORDER BY start_ts ASC
                LIMIT 1
                """,
                (cutoff,),
            ).fetchone()
            if not row:
                return None
            d = dict(row)
            diff_sec = d["start_ts"] - now
            min_until = int(diff_sec // 60)
            return {
                "id": d["id"],
                "summary": d["summary"][:32],
                "time": d["time_str"],
                "date": d["date_str"],
                "start_ts": d["start_ts"],
                "is_today": True,
                "is_soon": (0 <= diff_sec <= 900),  # <= 15 хвилин
                "min_until": max(0, min_until),
                "source": d["source"],
            }
        except Exception as e:
            logger.warning("get_next_calendar_event error: %s", e)
            return None


db_service = DatabaseService()
