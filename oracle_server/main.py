# Файл: oracle_server/main.py
import os

os.environ["OMP_NUM_THREADS"] = "1"
os.environ["OPENBLAS_NUM_THREADS"] = "1"
os.environ["MALLOC_TRIM_THRESHOLD_"] = "65536"
os.environ["MALLOC_ARENA_MAX"] = "2"

import asyncio
from contextlib import asynccontextmanager
from pathlib import Path
from fastapi import FastAPI
from fastapi.middleware.gzip import GZipMiddleware
from fastapi.staticfiles import StaticFiles
from starlette.responses import FileResponse

from app.core.config import settings, validate_security_keys
from app.core.logger import logger
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.api.router import api_router
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.bot_service import bot_service


async def _supervised_bot_loop() -> None:
    """Супервізор фонового циклу Telegram-бота з експоненційним backoff при збоях."""
    backoff = 3.0
    while True:
        try:
            await bot_service.polling_loop()
            backoff = 3.0
        except asyncio.CancelledError:
            break
        except Exception as e:
            logger.error("Telegram bot task crashed: %s. Restarting in %.1fs...", e, backoff)
            await asyncio.sleep(backoff)
            backoff = min(backoff * 2.0, 60.0)


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Валідація ключів безпеки при старті
    validate_security_keys()
    db_service.cleanup_expired_sessions()
    memory_guard.recalculate_disk_usage()
    memory_guard.trim_ram()

    # Запуск супервізора Telegram бота
    bot_task = asyncio.create_task(_supervised_bot_loop(), name="tg_bot_supervisor")

    yield

    bot_task.cancel()
    blackout_service.shutdown()
    db_service.close()
    await http_pool.close()
    memory_guard.trim_ram()


app = FastAPI(
    title=settings.PROJECT_NAME,
    version=settings.VERSION,
    lifespan=lifespan,
)

app.add_middleware(GZipMiddleware, minimum_size=1000)
app.include_router(api_router)

# Підключення статичних ресурсів (CSS, JS, PWA icons, SW)
STATIC_DIR = Path(__file__).resolve().parent / "app" / "web" / "static"
if STATIC_DIR.exists():
    app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")

    @app.get("/manifest.json", include_in_schema=False)
    async def get_manifest():
        return FileResponse(
            STATIC_DIR / "manifest.json",
            media_type="application/manifest+json",
            headers={"Cache-Control": "public, max-age=86400"},
        )

    @app.get("/sw.js", include_in_schema=False)
    async def get_sw():
        return FileResponse(
            STATIC_DIR / "sw.js",
            media_type="application/javascript",
            headers={
                "Cache-Control": "no-cache, no-store, must-revalidate",
                "Service-Worker-Allowed": "/",
            },
        )

    @app.get("/favicon.ico", include_in_schema=False)
    async def get_favicon():
        return FileResponse(
            STATIC_DIR / "icon-192.svg",
            media_type="image/svg+xml",
            headers={"Cache-Control": "public, max-age=604800"},
        )


@app.get("/health", tags=["System"])
async def health_check():
    return {
        "status": "ok",
        "version": settings.VERSION,
        "online": state_service.is_online(),
        "queued_commands": len(state_service.command_queue),
        **memory_guard.get_telemetry(),
    }
