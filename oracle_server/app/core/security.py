import secrets
from typing import Optional
from fastapi import Header, HTTPException, status
from app.core.config import settings


async def verify_device_key(x_device_key: Optional[str] = Header(None)) -> str:
    if not x_device_key or not secrets.compare_digest(x_device_key, settings.API_KEY):
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Forbidden: Invalid or missing X-Device-Key",
        )
    return x_device_key