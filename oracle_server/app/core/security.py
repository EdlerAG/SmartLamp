# Файл: oracle_server/app/core/security.py
import secrets
from typing import Optional
from fastapi import Header, Cookie, HTTPException, status
from app.core.config import settings


async def verify_access(
    x_device_key: Optional[str] = Header(None),
    mb_session: Optional[str] = Cookie(None),
) -> str:
    """Єдиний захист для ESP32-S3 (X-Device-Key) та Web-Термінала (Cookie або заголовок)."""
    token = x_device_key or mb_session or ""
    if token and (
        secrets.compare_digest(token, settings.API_KEY)
        or secrets.compare_digest(token, settings.DASHBOARD_PASSWORD)
    ):
        return token

    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Unauthorized: Invalid Device Key or Dashboard Password",
    )