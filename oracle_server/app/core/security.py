# Файл: oracle_server/app/core/security.py
import secrets
import time
from typing import Optional, Dict, List
from fastapi import Header, Cookie, HTTPException, status, Request
from app.core.config import settings
from app.services.database_service import db_service
from app.core.logger import logger

# Rate limiter для невдалих спроб логіну (IP -> список timestamps)
_login_failures: Dict[str, List[float]] = {}


def check_login_rate_limit(client_ip: str) -> bool:
    now = time.time()
    cutoff = now - 900.0  # 15 хвилин
    attempts = [t for t in _login_failures.get(client_ip, []) if t > cutoff]
    _login_failures[client_ip] = attempts
    return len(attempts) < 5


def record_login_failure(client_ip: str) -> None:
    now = time.time()
    attempts = _login_failures.get(client_ip, [])
    attempts.append(now)
    _login_failures[client_ip] = attempts
    logger.warning("Failed login attempt from IP '%s' (total: %d/5)", client_ip, len(attempts))


def clear_login_failures(client_ip: str) -> None:
    _login_failures.pop(client_ip, None)


async def verify_access(
    request: Request,
    x_device_key: Optional[str] = Header(None),
    mb_session: Optional[str] = Cookie(None),
    authorization: Optional[str] = Header(None),
) -> str:
    """
    Універсальна перевірка доступу (Device, Session або Admin API-Key):
    - Для плати ESP32-S3: заголовок X-Device-Key
    - Для Web Studio: випадковий криптографічний токен у Cookie 'mb_session'
    - Для API / curl: заголовок 'Authorization: Bearer <token>' або прямий ключ
    """
    bearer_token = ""
    if authorization and authorization.startswith("Bearer "):
        bearer_token = authorization.split("Bearer ", 1)[1].strip()

    token = x_device_key or mb_session or bearer_token or ""
    if not token:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Unauthorized: Відсутній ключ авторизації або токен сесії",
        )

    # 1. Перевірка валідності збереженої сесії в БД (криптографічний хеш)
    role = db_service.validate_session(token)
    if role:
        return role

    # 2. Пряма перевірка пароля панелі адміністратора
    if secrets.compare_digest(token, settings.DASHBOARD_PASSWORD):
        return "admin"

    # 3. Якщо запит надійшов із заголовком Authorization: Bearer <API_KEY> (адміністративний API-доступ)
    if bearer_token and secrets.compare_digest(bearer_token, settings.API_KEY):
        return "admin"

    # 4. Пряма перевірка апаратного ключа плати
    if secrets.compare_digest(token, settings.API_KEY):
        return "device"

    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Unauthorized: Недійсний ключ доступу або сесію завершено",
    )


async def verify_admin_access(
    request: Request,
    x_device_key: Optional[str] = Header(None),
    mb_session: Optional[str] = Cookie(None),
    authorization: Optional[str] = Header(None),
) -> str:
    """Захист для операцій керування пристроєм та інтеграціями (доступно власниці або адміністратору)."""
    role = await verify_access(request, x_device_key, mb_session, authorization)
    if role not in ("admin", "owner"):
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Forbidden: Доступ дозволено лише власниці або адміністратору",
        )
    return role


async def verify_system_admin_access(
    request: Request,
    x_device_key: Optional[str] = Header(None),
    mb_session: Optional[str] = Cookie(None),
    authorization: Optional[str] = Header(None),
) -> str:
    """Суворий технічний захист (OTA оновлення прошивки, оновлення коду сервера, бекапи)."""
    role = await verify_access(request, x_device_key, mb_session, authorization)
    if role != "admin":
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Forbidden: Доступ дозволено виключно системному адміністратору",
        )
    return role
