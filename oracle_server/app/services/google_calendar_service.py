# Файл: oracle_server/app/services/google_calendar_service.py
import time
import datetime
import urllib.parse
import secrets
import re
from typing import Dict, Any, Optional, List
import httpx

from app.core.config import settings
from app.core.http_client import http_pool
from app.core.logger import logger
from app.services.database_service import db_service
from app.services.state_service import state_service


class GoogleCalendarService:
    """
    Уніфікований сервіс Календаря Насті (v5.5):
    - Підтримка Google Calendar iCal 1-клік синхронізації (без потреби в Google Cloud Console).
    - Підтримка Google Calendar OAuth2 (читання + запис подій).
    - Вбудована SQLite база подій для швидкого додавання через Telegram-бота.
    - Автоматичне злиття та дедуплікація подій: найближча подія миттєво з'являється на лампі (Smart Hub).
    """

    __slots__ = ("_access_token", "_token_expires_at", "_cached_event", "_last_poll_time")

    AUTH_ENDPOINT = "https://accounts.google.com/o/oauth2/v2/auth"
    TOKEN_ENDPOINT = "https://oauth2.googleapis.com/token"
    CALENDAR_API = "https://www.googleapis.com/calendar/v3/calendars/primary/events"
    SCOPES = "https://www.googleapis.com/auth/calendar.events"

    @property
    def client_id(self) -> str:
        return db_service.get_setting("google_client_id", settings.GOOGLE_CLIENT_ID or "").strip()

    @property
    def client_secret(self) -> str:
        return db_service.get_setting("google_client_secret", settings.GOOGLE_CLIENT_SECRET or "").strip()

    @property
    def ical_url(self) -> str:
        return db_service.get_setting("google_calendar_ical_url", "").strip()

    def save_credentials(self, client_id: str, client_secret: str) -> None:
        """Зберігає ключі Google OAuth у SQLite WAL для динамічного налаштування."""
        db_service.set_setting("google_client_id", client_id.strip())
        db_service.set_setting("google_client_secret", client_secret.strip())
        db_service.log_event("system", "google", "save_credentials", {"client_id": client_id[:6]})

    def save_ical_url(self, url: str) -> None:
        """Зберігає приватне iCal-посилання Google Календаря для 1-клік синхронізації без OAuth."""
        db_service.set_setting("google_calendar_ical_url", url.strip())
        self._cached_event = None
        db_service.log_event("system", "google", "save_ical_url", {"url": url[:30]})

    def __init__(self) -> None:
        self._access_token: Optional[str] = None
        self._token_expires_at: float = 0.0
        self._cached_event: Optional[Dict[str, Any]] = None
        self._last_poll_time: float = 0.0

    def _get_redirect_uri(self, base_url: str = "") -> str:
        if settings.GOOGLE_REDIRECT_URI:
            return settings.GOOGLE_REDIRECT_URI
        host = state_service.resolve_public_url(base_url)
        return f"{host}/api/v5/google/callback"

    def get_authorize_url(self, base_url: str = "") -> str:
        cid = self.client_id
        if not cid:
            return ""
        params = {
            "client_id": cid,
            "redirect_uri": self._get_redirect_uri(base_url),
            "response_type": "code",
            "scope": self.SCOPES,
            "access_type": "offline",
            "prompt": "consent",
        }
        return f"{self.AUTH_ENDPOINT}?{urllib.parse.urlencode(params)}"

    def is_linked(self) -> bool:
        if self.ical_url:
            return True
        tokens = db_service.get_setting("google_calendar_tokens", {})
        return bool(tokens.get("refresh_token") and self.client_id and self.client_secret)

    def get_link_type(self) -> str:
        if self.ical_url:
            return "ical"
        if self.is_linked():
            return "oauth"
        return "none"

    async def handle_callback(self, code: str, base_url: str = "") -> bool:
        cid, csec = self.client_id, self.client_secret
        if not cid or not csec:
            logger.warning("Google Calendar client credentials not set")
            return False

        redirect_uri = self._get_redirect_uri(base_url)
        data = {
            "code": code,
            "client_id": cid,
            "client_secret": csec,
            "redirect_uri": redirect_uri,
            "grant_type": "authorization_code",
        }

        try:
            client = http_pool.get()
            res = await client.post(self.TOKEN_ENDPOINT, data=data, timeout=10.0)
            if res.status_code != 200:
                logger.error("Google token exchange error %d: %s", res.status_code, res.text)
                return False
            payload = res.json()
            self._access_token = payload.get("access_token")
            self._token_expires_at = time.time() + float(payload.get("expires_in", 3600)) - 60

            saved = db_service.get_setting("google_calendar_tokens", {})
            saved["refresh_token"] = payload.get("refresh_token", saved.get("refresh_token", ""))
            db_service.set_setting("google_calendar_tokens", saved)
            self._cached_event = None
            logger.info("Google Calendar successfully linked for Nastya!")
            return True
        except Exception as e:
            logger.error("Google callback exception: %s", e)
            return False

    def disconnect(self) -> None:
        db_service.set_setting("google_calendar_tokens", {})
        db_service.set_setting("google_calendar_ical_url", "")
        db_service.purge_google_calendar_events()
        self._access_token = None
        self._cached_event = None
        db_service.log_event("system", "calendar", "disconnect", {"status": "unlinked_and_purged"})
        logger.info("Google Calendar disconnected and events purged")

    async def _refresh_access_token(self) -> bool:
        tokens = db_service.get_setting("google_calendar_tokens", {})
        refresh_token = tokens.get("refresh_token")
        cid, csec = self.client_id, self.client_secret
        if not refresh_token or not cid or not csec:
            return False

        data = {
            "client_id": cid,
            "client_secret": csec,
            "refresh_token": refresh_token,
            "grant_type": "refresh_token",
        }

        try:
            client = http_pool.get()
            res = await client.post(self.TOKEN_ENDPOINT, data=data, timeout=8.0)
            if res.status_code == 200:
                payload = res.json()
                self._access_token = payload.get("access_token")
                self._token_expires_at = time.time() + float(payload.get("expires_in", 3600)) - 60
                return True
            logger.warning("Google token refresh failed (%d): %s", res.status_code, res.text)
            return False
        except Exception as e:
            logger.warning("Google token refresh exception: %s", e)
            return False

    async def _fetch_ical_events(self) -> List[Dict[str, Any]]:
        url = self.ical_url
        if not url:
            return []
        try:
            client = http_pool.get()
            res = await client.get(url, timeout=10.0, follow_redirects=True)
            if res.status_code != 200:
                return []
            content = res.text
        except Exception as e:
            logger.warning("Failed to fetch iCal: %s", e)
            return []

        events: List[Dict[str, Any]] = []
        raw_events = re.findall(r"BEGIN:VEVENT(.*?)END:VEVENT", content, re.DOTALL)
        now_ts = time.time()

        for ev_str in raw_events:
            summary_m = re.search(r"SUMMARY:(.*?)(?:\r?\n[A-Z]|\r?\nEND)", ev_str, re.DOTALL)
            summary = summary_m.group(1).strip() if summary_m else "Подія"
            summary = re.sub(r"\r?\n\s+", "", summary)

            dtstart_m = re.search(r"DTSTART(?:;[^:]+)?:(\d{8}(?:T\d{6}Z?)?)", ev_str)
            if not dtstart_m:
                continue
            raw_start = dtstart_m.group(1)

            try:
                if len(raw_start) == 8:
                    dt = datetime.datetime.strptime(raw_start, "%Y%m%d").replace(tzinfo=datetime.timezone.utc)
                elif raw_start.endswith("Z"):
                    dt = datetime.datetime.strptime(raw_start, "%Y%m%dT%H%M%SZ").replace(tzinfo=datetime.timezone.utc)
                else:
                    dt = datetime.datetime.strptime(raw_start, "%Y%m%dT%H%M%S").replace(tzinfo=datetime.timezone.utc)
            except Exception:
                continue

            start_ts = dt.timestamp()
            if start_ts < (now_ts - 1800):
                continue

            kyiv_tz = datetime.timezone(datetime.timedelta(hours=2))
            kyiv_dt = dt.astimezone(kyiv_tz)
            time_str = kyiv_dt.strftime("%H:%M")
            date_str = kyiv_dt.strftime("%d.%m")
            diff_sec = start_ts - now_ts
            min_until = int(diff_sec // 60)

            events.append({
                "id": f"ical_{int(start_ts)}",
                "summary": summary[:32],
                "time": time_str,
                "date": date_str,
                "start_ts": start_ts,
                "min_until": max(0, min_until),
                "is_soon": (0 <= diff_sec <= 900),
                "source": "google",
            })

        events.sort(key=lambda x: x["start_ts"])
        return events

    async def _fetch_google_next_event(self) -> Optional[Dict[str, Any]]:
        """Отримує найближчу подію з iCal або Google Calendar API."""
        if not self.is_linked():
            return None

        if self.ical_url:
            ical_evs = await self._fetch_ical_events()
            return ical_evs[0] if ical_evs else None

        now = time.time()
        if not self._access_token or now >= self._token_expires_at:
            ok = await self._refresh_access_token()
            if not ok:
                return None

        now_iso = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now))
        params = {
            "timeMin": now_iso,
            "maxResults": "1",
            "singleEvents": "true",
            "orderBy": "startTime",
        }
        headers = {"Authorization": f"Bearer {self._access_token}"}

        try:
            client = http_pool.get()
            res = await client.get(self.CALENDAR_API, params=params, headers=headers, timeout=6.0)
            if res.status_code != 200:
                return None
            items = res.json().get("items", [])
            if not items:
                return None

                item = items[0]
                summary = item.get("summary", "Подія")
                start_raw = item.get("start", {}).get("dateTime") or item.get("start", {}).get("date")

                start_ts = now
                time_display = "--:--"
                date_display = ""
                if start_raw:
                    try:
                        dt = datetime.datetime.fromisoformat(str(start_raw).replace("Z", "+00:00"))
                        start_ts = dt.timestamp()
                        time_display = dt.strftime("%H:%M")
                        date_display = dt.strftime("%d.%m")
                    except Exception:
                        if "T" in str(start_raw):
                            time_display = str(start_raw).split("T")[1][:5]

                diff_sec = start_ts - now
                min_until = int(diff_sec // 60)

                return {
                    "id": item.get("id", f"g_{int(start_ts)}"),
                    "summary": summary[:32],
                    "time": time_display,
                    "date": date_display,
                    "start_ts": start_ts,
                    "is_today": True,
                    "is_soon": (0 <= diff_sec <= 900),  # <= 15 хвилин
                    "min_until": max(0, min_until),
                    "source": "google",
                }
        except Exception as e:
            logger.warning("Google Calendar fetch error: %s", e)
            return None

    async def get_next_event(self) -> Optional[Dict[str, Any]]:
        """
        Уніфікований пошук найближчої події:
        Порівнює події з Google Calendar та локальної SQLite бази, повертаючи найбільш актуальну.
        Кешується на 90 секунд.
        """
        now = time.time()
        if self._cached_event and (now - self._last_poll_time < 90.0):
            # Перераховуємо динамічні прапорці diff_sec та is_soon
            diff_sec = self._cached_event.get("start_ts", now) - now
            self._cached_event["is_soon"] = (0 <= diff_sec <= 900)
            self._cached_event["min_until"] = max(0, int(diff_sec // 60))
            return self._cached_event

        google_ev = await self._fetch_google_next_event() if self.is_linked() else None
        db_ev = db_service.get_next_calendar_event()

        chosen = None
        if google_ev and db_ev:
            chosen = google_ev if google_ev["start_ts"] <= db_ev["start_ts"] else db_ev
        elif google_ev:
            chosen = google_ev
        elif db_ev:
            chosen = db_ev

        self._cached_event = chosen
        self._last_poll_time = now
        return chosen

    async def create_event(
        self,
        summary: str,
        start_dt: datetime.datetime,
        end_dt: Optional[datetime.datetime] = None,
        description: str = "",
        source: str = "manual",
    ) -> Dict[str, Any]:
        """
        Створює подію:
        1. Зберігає у локальну SQLite базу (для миттєвої синхронізації з лампою).
        2. Якщо Google Calendar підключено — синхронізує її в офіційний Google Calendar.
        """
        if end_dt is None:
            end_dt = start_dt + datetime.timedelta(hours=1)

        start_ts = start_dt.timestamp()
        end_ts = end_dt.timestamp()
        time_str = start_dt.strftime("%H:%M")
        date_str = start_dt.strftime("%Y-%m-%d")
        event_id = f"ev_{int(start_ts)}_{secrets.token_hex(4)}"

        # 1. Запис у Google Calendar (якщо прив'язано)
        if self.is_linked():
            now = time.time()
            if not self._access_token or now >= self._token_expires_at:
                await self._refresh_access_token()

            if self._access_token:
                body = {
                    "summary": summary,
                    "description": description or "Додано через SmartLamp Workspace",
                    "start": {"dateTime": start_dt.isoformat()},
                    "end": {"dateTime": end_dt.isoformat()},
                }
                headers = {"Authorization": f"Bearer {self._access_token}", "Content-Type": "application/json"}
                try:
                    async with httpx.AsyncClient(timeout=8.0) as client:
                        res = await client.post(self.CALENDAR_API, json=body, headers=headers)
                        if res.status_code in (200, 201):
                            g_data = res.json()
                            event_id = g_data.get("id", event_id)
                            source = "google"
                            logger.info("Event successfully created in Google Calendar: %s", summary)
                except Exception as e:
                    logger.warning("Failed to push event to Google Calendar: %s", e)

        # 2. Збереження у локальну SQLite базу
        saved = db_service.save_calendar_event(
            event_id=event_id,
            summary=summary,
            start_ts=start_ts,
            end_ts=end_ts,
            time_str=time_str,
            date_str=date_str,
            source=source,
        )

        # Очищуємо кеш найближчої події для миттєвого відображення на лампі
        self._cached_event = None
        return saved

    async def delete_event(self, event_id: str) -> bool:
        """Видаляє подію з бази та Google Calendar."""
        db_service.delete_calendar_event(event_id)
        if self.is_linked() and self._access_token and not event_id.startswith("ev_"):
            try:
                headers = {"Authorization": f"Bearer {self._access_token}"}
                url = f"{self.CALENDAR_API}/{event_id}"
                async with httpx.AsyncClient(timeout=8.0) as client:
                    await client.delete(url, headers=headers)
            except Exception as e:
                logger.warning("Failed to delete event from Google Calendar: %s", e)

        self._cached_event = None
        return True

    async def get_upcoming_events(self, max_results: int = 15) -> List[Dict[str, Any]]:
        """Повертає об'єднаний список майбутніх подій із сортуванням за часом."""
        events: List[Dict[str, Any]] = []

        # 1. Події з iCal або Google Calendar
        if self.ical_url:
            ical_evs = await self._fetch_ical_events()
            events.extend(ical_evs[:max_results])
        elif self.is_linked():
            now = time.time()
            if not self._access_token or now >= self._token_expires_at:
                await self._refresh_access_token()

            if self._access_token:
                now_iso = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now))
                params = {
                    "timeMin": now_iso,
                    "maxResults": str(min(30, max_results)),
                    "singleEvents": "true",
                    "orderBy": "startTime",
                }
                headers = {"Authorization": f"Bearer {self._access_token}"}
                try:
                    async with httpx.AsyncClient(timeout=8.0) as client:
                        res = await client.get(self.CALENDAR_API, params=params, headers=headers)
                        if res.status_code == 200:
                            for item in res.json().get("items", []):
                                summary = item.get("summary", "Без назви")
                                start_raw = item.get("start", {}).get("dateTime") or item.get("start", {}).get("date")
                                time_display = "--:--"
                                date_display = ""
                                start_ts = now
                                if start_raw:
                                    try:
                                        dt = datetime.datetime.fromisoformat(str(start_raw).replace("Z", "+00:00"))
                                        start_ts = dt.timestamp()
                                        time_display = dt.strftime("%H:%M")
                                        date_display = dt.strftime("%d.%m")
                                    except Exception:
                                        pass
                                events.append({
                                    "id": item.get("id", ""),
                                    "summary": summary,
                                    "date": date_display,
                                    "time": time_display,
                                    "start_ts": start_ts,
                                    "source": "google",
                                })
                except Exception as e:
                    logger.warning("Error fetching upcoming Google events: %s", e)

        # 2. Події з локальної SQLite бази (додані з Telegram / окремого бота)
        db_events = db_service.list_upcoming_calendar_events(limit=max_results)
        for ev in db_events:
            events.append({
                "id": ev["id"],
                "summary": ev["summary"],
                "date": ev.get("date_str", ""),
                "time": ev.get("time_str", "--:--"),
                "start_ts": ev["start_ts"],
                "source": ev.get("source", "manual"),
            })

        # Дедуплікація за однаковою назвою та початковим часом (в межах 60 секунд)
        seen = set()
        deduped = []
        # Сортуємо хронологічно
        events.sort(key=lambda x: x.get("start_ts", 0))
        for ev in events:
            key = f"{ev['summary'].lower().strip()}_{int(ev.get('start_ts', 0) // 60)}"
            if key not in seen:
                seen.add(key)
                deduped.append(ev)

        return deduped[:max_results]


google_calendar_service = GoogleCalendarService()
