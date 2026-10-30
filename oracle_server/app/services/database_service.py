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
            conn.execute("PRAGMA cache_size=-2000;")  # 2.0 МБ кешу
            conn.execute("PRAGMA mmap_size=16777216;") # 16 МБ пам'яті під відображення для надшвидкого читання
            conn.execute("PRAGMA busy_timeout=5000;")
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
        conn.execute("""
            CREATE TABLE IF NOT EXISTS event_log (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ts FLOAT NOT NULL,
                actor VARCHAR(24) NOT NULL,
                category VARCHAR(24) NOT NULL,
                action VARCHAR(64) NOT NULL,
                details TEXT DEFAULT '{}'
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS box_health_history (
                ts INTEGER PRIMARY KEY,
                rssi INTEGER NOT NULL,
                heap INTEGER NOT NULL,
                psram INTEGER NOT NULL,
                uptime INTEGER NOT NULL,
                reset_reason INTEGER DEFAULT 0
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS telemetry_rollups (
                date_str VARCHAR(10) PRIMARY KEY,
                temp_min FLOAT,
                temp_max FLOAT,
                temp_avg FLOAT,
                hum_avg FLOAT,
                pres_avg FLOAT,
                outage_count INTEGER DEFAULT 0,
                outage_sec FLOAT DEFAULT 0,
                pomo_sessions INTEGER DEFAULT 0,
                reboots_count INTEGER DEFAULT 0,
                created_at FLOAT NOT NULL
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS pending_commands (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                cmd_json TEXT NOT NULL,
                created_at FLOAT NOT NULL
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS push_subscriptions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                endpoint TEXT UNIQUE NOT NULL,
                sub_json TEXT NOT NULL,
                created_at FLOAT NOT NULL
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS device_claim (
                claim_hash VARCHAR(64) PRIMARY KEY,
                claimed_by_chat_id VARCHAR(64),
                claimed_at FLOAT,
                created_at FLOAT NOT NULL
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS setup_steps (
                step_id VARCHAR(32) PRIMARY KEY,
                status VARCHAR(16) NOT NULL,
                completed_at FLOAT,
                updated_at FLOAT NOT NULL
            );
        """)
        # Створення високопродуктивних B-Tree індексів
        conn.execute("CREATE INDEX IF NOT EXISTS idx_cal_start_ts ON calendar_events(start_ts);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_outages_disc ON power_outages(disconnected_at);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_media_slot ON custom_media(reaction_slot);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_climate_ts ON climate_history(ts);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_event_log_ts ON event_log(ts);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_event_log_cat ON event_log(category, ts);")
        conn.execute("CREATE INDEX IF NOT EXISTS idx_claim_chat ON device_claim(claimed_by_chat_id);")

        # Міграція: додаємо ws_rec та ota до box_health_history якщо вони відсутні
        for col_name, col_type in (("ws_rec", "INTEGER DEFAULT 0"), ("ota", "INTEGER DEFAULT 0")):
            try:
                conn.execute(f"ALTER TABLE box_health_history ADD COLUMN {col_name} {col_type};")
            except Exception:
                pass

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

    def add_climate_batch(self, samples: List[Dict[str, Any]]) -> int:
        """Пакетне збереження офлайн-телеметрії («чорної скриньки» шкатулки під час блекауту)."""
        if not samples:
            return 0
        try:
            conn = self._get_conn()
            inserted = 0
            for s in samples:
                ts = int(s.get("ts", time.time()))
                temp = round(float(s.get("temp", 22.0)), 1)
                hum = round(float(s.get("hum", 50.0)), 0)
                pres = round(float(s.get("pres", 750.0)), 1)
                light = int(s.get("light", 50))
                conn.execute(
                    "INSERT OR REPLACE INTO climate_history (ts, temp, hum, pres, light) VALUES (?, ?, ?, ?, ?)",
                    (ts, temp, hum, pres, light),
                )
                inserted += 1
            conn.commit()
            return inserted
        except Exception as e:
            logger.warning("add_climate_batch error: %s", e)
            return 0

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

    def purge_google_calendar_events(self) -> int:
        """Видаляє всі події, синхронізовані з Google Calendar."""
        try:
            conn = self._get_conn()
            cur = conn.execute("DELETE FROM calendar_events WHERE source = 'google'")
            conn.commit()
            return cur.rowcount
        except Exception as e:
            logger.error("purge_google_calendar_events error: %s", e)
            return 0

    # --- ЖУРНАЛ ПОДІЙ ТА АУДИТ (EVENT LOG) ---
    def log_event(self, actor: str, category: str, action: str, details: Optional[Dict[str, Any]] = None) -> None:
        """Записує подію до аудиторського журналу."""
        now = time.time()
        d_str = orjson.dumps(details or {}).decode("utf-8")
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT INTO event_log (ts, actor, category, action, details) VALUES (?, ?, ?, ?, ?)",
                (now, actor[:24], category[:24], action[:64], d_str),
            )
            # Авто-ротація логу: видаляємо старіші ніж 14 днів або зберігаємо топ 1000
            if int(now) % 3600 < 60:
                conn.execute(
                    "DELETE FROM event_log WHERE id NOT IN (SELECT id FROM event_log ORDER BY id DESC LIMIT 1000)"
                )
            conn.commit()
        except Exception as e:
            logger.warning("log_event error: %s", e)

    def get_recent_events(self, limit: int = 50, category: Optional[str] = None) -> List[Dict[str, Any]]:
        try:
            conn = self._get_conn()
            if category:
                rows = conn.execute(
                    "SELECT id, ts, actor, category, action, details FROM event_log "
                    "WHERE category = ? ORDER BY id DESC LIMIT ?",
                    (category, limit),
                ).fetchall()
            else:
                rows = conn.execute(
                    "SELECT id, ts, actor, category, action, details FROM event_log ORDER BY id DESC LIMIT ?",
                    (limit,),
                ).fetchall()
            res = []
            for r in rows:
                item = dict(r)
                try:
                    item["details"] = orjson.loads(item.get("details", "{}"))
                except Exception:
                    item["details"] = {}
                res.append(item)
            return res
        except Exception as e:
            logger.warning("get_recent_events error: %s", e)
            return []

    # --- ЗДОРОВ'Я ПЛАТИ ТА АПАРАТНА ТЕЛЕМЕТРІЯ ---
    def record_box_health(
        self,
        rssi: int,
        heap: int,
        psram: int,
        uptime: int,
        reset_reason: int = 0,
        ws_rec: int = 0,
        ota: int = 0,
    ) -> None:
        now_ts = int(time.time())
        cutoff = now_ts - (7 * 86400)
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT OR REPLACE INTO box_health_history (ts, rssi, heap, psram, uptime, reset_reason, ws_rec, ota) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (now_ts, int(rssi), int(heap), int(psram), int(uptime), int(reset_reason), int(ws_rec), int(ota)),
            )
            if now_ts % 7200 < 300:
                conn.execute("DELETE FROM box_health_history WHERE ts < ?", (cutoff,))
            conn.commit()
        except Exception as e:
            logger.warning("record_box_health error: %s", e)

    def get_box_health_series(self, hours: int = 24) -> List[Dict[str, Any]]:
        cutoff = int(time.time()) - (hours * 3600)
        try:
            rows = self._get_conn().execute(
                "SELECT ts, rssi, heap, psram, uptime, reset_reason, COALESCE(ws_rec, 0) as ws_rec, COALESCE(ota, 0) as ota "
                "FROM box_health_history WHERE ts >= ? ORDER BY ts ASC",
                (cutoff,),
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as e:
            logger.warning("get_box_health_series error: %s", e)
            return []

    # --- ПЕРСИСТЕНТНА ЧЕРГА КОМАНД (PERSISTENT COMMAND QUEUE) ---
    def save_pending_command(self, cmd: Dict[str, Any]) -> int:
        try:
            conn = self._get_conn()
            cur = conn.execute(
                "INSERT INTO pending_commands (cmd_json, created_at) VALUES (?, ?)",
                (orjson.dumps(cmd).decode("utf-8"), time.time()),
            )
            conn.commit()
            return cur.lastrowid or 0
        except Exception as e:
            logger.warning("save_pending_command error: %s", e)
            return 0

    def load_pending_commands(self) -> List[Dict[str, Any]]:
        try:
            rows = self._get_conn().execute(
                "SELECT cmd_json FROM pending_commands ORDER BY id ASC LIMIT 50"
            ).fetchall()
            cmds = []
            for r in rows:
                try:
                    cmds.append(orjson.loads(r["cmd_json"]))
                except Exception:
                    pass
            return cmds
        except Exception as e:
            logger.warning("load_pending_commands error: %s", e)
            return []

    def sync_pending_commands(self, cmds: List[Dict[str, Any]]) -> None:
        try:
            conn = self._get_conn()
            with conn:
                conn.execute("DELETE FROM pending_commands")
                if cmds:
                    now = time.time()
                    conn.executemany(
                        "INSERT INTO pending_commands (cmd_json, created_at) VALUES (?, ?)",
                        [(orjson.dumps(cmd).decode("utf-8"), cmd.get("created_at", now)) for cmd in cmds],
                    )
        except Exception as e:
            logger.warning("sync_pending_commands error: %s", e)

    def clear_pending_commands(self) -> None:
        try:
            conn = self._get_conn()
            conn.execute("DELETE FROM pending_commands")
            conn.commit()
        except Exception as e:
            logger.warning("clear_pending_commands error: %s", e)

    # --- WEB PUSH ПІДПИСКИ (VAPID) ---
    def save_push_subscription(self, sub: Dict[str, Any]) -> bool:
        endpoint = sub.get("endpoint")
        if not endpoint:
            return False
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT OR REPLACE INTO push_subscriptions (endpoint, sub_json, created_at) VALUES (?, ?, ?)",
                (endpoint, orjson.dumps(sub).decode("utf-8"), time.time()),
            )
            conn.commit()
            return True
        except Exception as e:
            logger.warning("save_push_subscription error: %s", e)
            return False

    def get_push_subscriptions(self) -> List[Dict[str, Any]]:
        try:
            rows = self._get_conn().execute(
                "SELECT sub_json FROM push_subscriptions ORDER BY id DESC"
            ).fetchall()
            subs = []
            for r in rows:
                try:
                    subs.append(orjson.loads(r["sub_json"]))
                except Exception:
                    pass
            return subs
        except Exception as e:
            logger.warning("get_push_subscriptions error: %s", e)
            return []

    def delete_push_subscription(self, endpoint: str) -> None:
        try:
            conn = self._get_conn()
            conn.execute("DELETE FROM push_subscriptions WHERE endpoint = ?", (endpoint,))
            conn.commit()
        except Exception as e:
            logger.warning("delete_push_subscription error: %s", e)

    # --- ДОБОВА АГРЕГАЦІЯ ТА РОЛАПИ (ROLLUPS) ---
    def save_daily_rollup(self, date_str: str, data: Dict[str, Any]) -> None:
        now = time.time()
        try:
            conn = self._get_conn()
            conn.execute(
                """
                INSERT INTO telemetry_rollups
                (date_str, temp_min, temp_max, temp_avg, hum_avg, pres_avg, outage_count, outage_sec, pomo_sessions, reboots_count, created_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(date_str) DO UPDATE SET
                    temp_min=excluded.temp_min,
                    temp_max=excluded.temp_max,
                    temp_avg=excluded.temp_avg,
                    hum_avg=excluded.hum_avg,
                    pres_avg=excluded.pres_avg,
                    outage_count=excluded.outage_count,
                    outage_sec=excluded.outage_sec,
                    pomo_sessions=excluded.pomo_sessions,
                    reboots_count=excluded.reboots_count,
                    created_at=excluded.created_at
                """,
                (
                    date_str,
                    data.get("temp_min"),
                    data.get("temp_max"),
                    data.get("temp_avg"),
                    data.get("hum_avg"),
                    data.get("pres_avg"),
                    data.get("outage_count", 0),
                    data.get("outage_sec", 0.0),
                    data.get("pomo_sessions", 0),
                    data.get("reboots_count", 0),
                    now,
                ),
            )
            conn.commit()
        except Exception as e:
            logger.error("save_daily_rollup error: %s", e)

    def get_daily_rollups(self, days: int = 14) -> List[Dict[str, Any]]:
        try:
            rows = self._get_conn().execute(
                "SELECT * FROM telemetry_rollups ORDER BY date_str DESC LIMIT ?",
                (days,),
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as e:
            logger.warning("get_daily_rollups error: %s", e)
            return []

    # --- ЧИСТЕ ПЕРЕДПОДАРУНКОВЕ СКИДАННЯ (SANDBOX PURGE) ---
    def purge_sandbox_test_data(self) -> Dict[str, int]:
        """
        Повне очищення тестових даних перед подарунком:
        - Видаляє тестові події календаря, історію клімату, блекаути, здоров'я плати та лог подій.
        - Скидає прив'язку пристрою (device_claim) та кроки налаштування (setup_steps).
        - ЗБЕРІГАЄ завантажені кастомні медіа (.mbx, MP3 звуки), пресети голосу та сесії.
        """
        stats = {}
        try:
            conn = self._get_conn()
            stats["calendar_events"] = conn.execute("DELETE FROM calendar_events").rowcount
            stats["climate_history"] = conn.execute("DELETE FROM climate_history").rowcount
            stats["power_outages"] = conn.execute("DELETE FROM power_outages").rowcount
            stats["box_health"] = conn.execute("DELETE FROM box_health_history").rowcount
            stats["event_log"] = conn.execute("DELETE FROM event_log").rowcount
            stats["telemetry_rollups"] = conn.execute("DELETE FROM telemetry_rollups").rowcount
            stats["device_claim"] = conn.execute("DELETE FROM device_claim").rowcount
            stats["setup_steps"] = conn.execute("DELETE FROM setup_steps").rowcount
            conn.commit()

            # Скидаємо збереженого власника в KV кеші
            self._kv_cache.pop("owner_chat_id", None)
            conn.execute("DELETE FROM kv_settings WHERE key = 'owner_chat_id'")
            conn.commit()

            self.init_setup_steps()
            logger.info("Sandbox test data purged successfully: %s", stats)
            return stats
        except Exception as e:
            logger.error("purge_sandbox_test_data error: %s", e)
            return stats

    # --- ОНБОРДИНГ, ПРИВ'ЯЗКА ТА ЕТАПИ НАЛАШТУВАННЯ ---
    def init_setup_steps(self) -> None:
        """Ініціалізація етапів першого підключення за замовчуванням."""
        now = time.time()
        default_steps = [
            ("wifi", "done", now),          # Плата вже на зв'язку з сервером
            ("pairing", "pending", None),   # Сканування QR та /start в боті
            ("mood", "pending", None),      # Вибір колірної теми / настрою
            ("spotify", "pending", None),   # Підключення Spotify
            ("calendar", "pending", None),  # Підключення Google Calendar
            ("quiet_hours", "pending", None),# Налаштування нічного режиму
            ("portal_tour", "pending", None),# Ознайомлення з веб-панеллю
        ]
        try:
            conn = self._get_conn()
            for step_id, status, comp_at in default_steps:
                conn.execute(
                    "INSERT OR IGNORE INTO setup_steps (step_id, status, completed_at, updated_at) VALUES (?, ?, ?, ?)",
                    (step_id, status, comp_at, now),
                )
            conn.commit()
        except Exception as e:
            logger.warning("init_setup_steps error: %s", e)

    def register_device_claim(self, secret: str) -> None:
        """Реєструє або оновлює секрет прив'язки від ESP32 (зберігається лише SHA-256 хеш)."""
        if not secret or len(secret) < 6:
            return
        claim_hash = hashlib.sha256(secret.strip().encode("utf-8")).hexdigest()
        now = time.time()
        try:
            conn = self._get_conn()
            conn.execute(
                "INSERT OR IGNORE INTO device_claim (claim_hash, claimed_by_chat_id, claimed_at, created_at) VALUES (?, NULL, NULL, ?)",
                (claim_hash, now),
            )
            conn.commit()
        except Exception as e:
            logger.error("register_device_claim error: %s", e)

    def claim_device(self, secret: str, chat_id: str) -> bool:
        """
        Атомарна прив'язка шкатулки до Telegram chat_id власниці:
        - Перевіряє або реєструє хеш секрету.
        - Запобігає захопленню чужою особою, якщо вже прив'язано.
        - Оновлює статус етапу 'pairing' на 'done'.
        """
        if not secret or not chat_id:
            return False
        claim_hash = hashlib.sha256(secret.strip().encode("utf-8")).hexdigest()
        now = time.time()
        chat_id_str = str(chat_id).strip()
        try:
            conn = self._get_conn()
            row = conn.execute(
                "SELECT claimed_by_chat_id FROM device_claim WHERE claim_hash = ?",
                (claim_hash,),
            ).fetchone()

            if not row:
                # Перша реєстрація секрету разом із прив'язкою
                conn.execute(
                    "INSERT INTO device_claim (claim_hash, claimed_by_chat_id, claimed_at, created_at) VALUES (?, ?, ?, ?)",
                    (claim_hash, chat_id_str, now, now),
                )
            else:
                existing = row["claimed_by_chat_id"]
                if existing and existing != chat_id_str:
                    logger.warning("Claim rejected: already claimed by chat_id %s", existing)
                    return False
                conn.execute(
                    "UPDATE device_claim SET claimed_by_chat_id = ?, claimed_at = ? WHERE claim_hash = ?",
                    (chat_id_str, now, claim_hash),
                )
            conn.commit()

            self.set_owner_chat_id(chat_id_str)
            self.update_setup_step("pairing", "done")
            logger.info("Device successfully claimed by owner chat_id: %s", chat_id_str)
            return True
        except Exception as e:
            logger.error("claim_device error: %s", e)
            return False

    def is_device_claimed(self) -> bool:
        """Перевіряє, чи шафа вже прив'язана до власниці."""
        if self.get_owner_chat_id():
            return True
        try:
            row = self._get_conn().execute(
                "SELECT 1 FROM device_claim WHERE claimed_by_chat_id IS NOT NULL LIMIT 1"
            ).fetchone()
            return bool(row)
        except Exception:
            return False

    def get_owner_chat_id(self) -> Optional[str]:
        """Повертає Telegram chat_id поточної законної власниці."""
        val = self.get_setting("owner_chat_id", None)
        if val:
            return str(val)
        if settings.TG_GIRL_ID:
            return str(settings.TG_GIRL_ID)
        return None

    def set_owner_chat_id(self, chat_id: str) -> None:
        """Зберігає Telegram chat_id власниці в KV-налаштуваннях."""
        self.set_setting("owner_chat_id", str(chat_id).strip())

    def get_setup_state(self) -> Dict[str, Any]:
        """Повертає поточний статус усіх етапів першого налаштування."""
        self.init_setup_steps()
        try:
            rows = self._get_conn().execute("SELECT * FROM setup_steps").fetchall()
            return {
                r["step_id"]: {
                    "status": r["status"],
                    "completed_at": r["completed_at"],
                    "updated_at": r["updated_at"],
                }
                for r in rows
            }
        except Exception as e:
            logger.error("get_setup_state error: %s", e)
            return {}

    def update_setup_step(self, step_id: str, status: str) -> None:
        """Оновлює статус етапу: 'pending', 'done', 'skipped', 'dismissed'."""
        now = time.time()
        completed_at = now if status == "done" else None
        try:
            conn = self._get_conn()
            conn.execute(
                """
                INSERT INTO setup_steps (step_id, status, completed_at, updated_at)
                VALUES (?, ?, ?, ?)
                ON CONFLICT(step_id) DO UPDATE SET
                    status=excluded.status,
                    completed_at=COALESCE(excluded.completed_at, setup_steps.completed_at),
                    updated_at=excluded.updated_at
                """,
                (step_id, status, completed_at, now),
            )
            conn.commit()
        except Exception as e:
            logger.error("update_setup_step error for '%s': %s", step_id, e)

    def get_theme_mood(self) -> str:
        """Повертає поточну тему настрою: 'cozy' (Затишок), 'sakura' (Сакура), 'ocean' (Океан), 'nordic' (Північ), 'chornobyl' (Чорнобиль)."""
        return str(self.get_setting("theme_mood", "cozy"))

    def set_theme_mood(self, mood: str) -> str:
        """Встановлює тему настрою та автоматично позначає крок 'mood' як завершений."""
        valid_moods = {"cozy", "sakura", "ocean", "nordic", "chornobyl", "chernobyl"}
        clean_mood = mood.lower().strip()
        if clean_mood == "chernobyl":
            clean_mood = "chornobyl"
        elif clean_mood not in valid_moods:
            clean_mood = "cozy"
        self.set_setting("theme_mood", clean_mood)
        self.update_setup_step("mood", "done")
        return clean_mood

    def get_viewer_alerts_enabled(self) -> bool:
        """Повертає статус технічних сповіщень для глядача (admin/viewer)."""
        return bool(self.get_setting("viewer_alerts_enabled", True))

    def set_viewer_alerts_enabled(self, enabled: bool) -> None:
        """Вмикає або вимикає надсилання технічних сповіщень глядачу."""
        self.set_setting("viewer_alerts_enabled", bool(enabled))


db_service = DatabaseService()
