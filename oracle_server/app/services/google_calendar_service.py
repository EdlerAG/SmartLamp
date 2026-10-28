# Файл: oracle_server/app/services/google_calendar_service.py
import time
import urllib.parse
from typing import Dict, Any, Optional
import httpx

from app.core.config import settings
from app.core.logger import logger
from app.services.database_service import db_service
from app.services.state_service import state_service


class GoogleCalendarService:
    """
    Zero-Knowledge особистий Google Календар для Насті:
    - OAuth2 Code Flow (авторизація з правами readonly до подій).
    - Отримує виключно найближчу 1 подію для виведення на картку Smart Hub лампи.
    - Повна ізоляція: список усіх подій календаря не зберігається і недоступний в адмін-панелі.
    """

    __slots__ = ("_access_token", "_token_expires_at", "_cached_event", "_last_poll_time")

    AUTH_ENDPOINT = "https://accounts.google.com/o/oauth2/v2/auth"
    TOKEN_ENDPOINT = "https://oauth2.googleapis.com/token"
    CALENDAR_API = "https://www.googleapis.com/calendar/v3/calendars/primary/events"
    SCOPES = "https://www.googleapis.com/auth/calendar.events.readonly"

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
        if not settings.GOOGLE_CLIENT_ID:
            return ""
        params = {
            "client_id": settings.GOOGLE_CLIENT_ID,
            "redirect_uri": self._get_redirect_uri(base_url),
            "response_type": "code",
            "scope": self.SCOPES,
            "access_type": "offline",
            "prompt": "consent",
        }
        return f"{self.AUTH_ENDPOINT}?{urllib.parse.urlencode(params)}"

    def is_linked(self) -> bool:
        tokens = db_service.get_setting("google_calendar_tokens", {})
        return bool(tokens.get("refresh_token") and settings.GOOGLE_CLIENT_ID and settings.GOOGLE_CLIENT_SECRET)

    async def handle_callback(self, code: str, base_url: str = "") -> bool:
        if not settings.GOOGLE_CLIENT_ID or not settings.GOOGLE_CLIENT_SECRET:
            logger.warning("Google Calendar client credentials not set")
            return False

        redirect_uri = self._get_redirect_uri(base_url)
        data = {
            "code": code,
            "client_id": settings.GOOGLE_CLIENT_ID,
            "client_secret": settings.GOOGLE_CLIENT_SECRET,
            "redirect_uri": redirect_uri,
            "grant_type": "authorization_code",
        }

        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                res = await client.post(self.TOKEN_ENDPOINT, data=data)
                if res.status_code != 200:
                    logger.error("Google token exchange error %d: %s", res.status_code, res.text)
                    return False
                payload = res.json()
                self._access_token = payload.get("access_token")
                self._token_expires_at = time.time() + float(payload.get("expires_in", 3600)) - 60

                saved = db_service.get_setting("google_calendar_tokens", {})
                saved["refresh_token"] = payload.get("refresh_token", saved.get("refresh_token", ""))
                db_service.set_setting("google_calendar_tokens", saved)
                logger.info("Google Calendar successfully linked for Nastya!")
                return True
        except Exception as e:
            logger.error("Google callback exception: %s", e)
            return False

    def disconnect(self) -> None:
        db_service.set_setting("google_calendar_tokens", {})
        self._access_token = None
        self._cached_event = None
        logger.info("Google Calendar disconnected")

    async def _refresh_access_token(self) -> bool:
        tokens = db_service.get_setting("google_calendar_tokens", {})
        refresh_token = tokens.get("refresh_token")
        if not refresh_token:
            return False

        data = {
            "client_id": settings.GOOGLE_CLIENT_ID,
            "client_secret": settings.GOOGLE_CLIENT_SECRET,
            "refresh_token": refresh_token,
            "grant_type": "refresh_token",
        }

        try:
            async with httpx.AsyncClient(timeout=8.0) as client:
                res = await client.post(self.TOKEN_ENDPOINT, data=data)
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

    async def get_next_event(self) -> Optional[Dict[str, Any]]:
        """
        Отримує виключно 1 найближчу подію для Smart Hub картки на лампі.
        Кешується на 5 хвилин для мінімізації навантаження та повної приватності.
        """
        if not self.is_linked():
            return None

        now = time.time()
        if self._cached_event and (now - self._last_poll_time < 300.0):
            return self._cached_event

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
            async with httpx.AsyncClient(timeout=6.0) as client:
                res = await client.get(self.CALENDAR_API, params=params, headers=headers)
                if res.status_code != 200:
                    return None
                data = res.json()
                items = data.get("items", [])
                if not items:
                    self._cached_event = None
                    self._last_poll_time = now
                    return None

                item = items[0]
                summary = item.get("summary", "Подія")
                start_raw = item.get("start", {}).get("dateTime") or item.get("start", {}).get("date")

                start_ts = now
                time_display = "--:--"
                if start_raw and "T" in start_raw:
                    try:
                        # Парсимо час "2026-10-28T14:30:00+02:00"
                        time_part = start_raw.split("T")[1][:5]
                        time_display = time_part
                    except Exception:
                        pass

                min_until = max(0, int((start_ts - now) // 60))

                self._cached_event = {
                    "summary": summary[:32],
                    "time": time_display,
                    "is_today": True,
                    "is_soon": 0 <= min_until <= 15,
                }
                self._last_poll_time = now
                return self._cached_event
        except Exception as e:
            logger.warning("Google Calendar fetch error: %s", e)
            return None


google_calendar_service = GoogleCalendarService()
