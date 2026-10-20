# Файл: oracle_server/main.py
import os

# Жорстке обмеження потоків C-аллокаторів для 1GB RAM VPS (як у QuantCore V23)
os.environ["OMP_NUM_THREADS"] = "1"
os.environ["OPENBLAS_NUM_THREADS"] = "1"
os.environ["MALLOC_TRIM_THRESHOLD_"] = "65536"
os.environ["MALLOC_ARENA_MAX"] = "2"

import asyncio
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.responses import ORJSONResponse

from app.core.config import settings
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.api.router import api_router
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.bot_service import bot_service


async def _periodic_ram_trim_loop() -> None:
    while True:
        await asyncio.sleep(120.0)
        memory_guard.trim_ram()


@asynccontextmanager
async def lifespan(app: FastAPI):
    memory_guard.recalculate_disk_usage()
    memory_guard.trim_ram()

    watchdog_task = asyncio.create_task(blackout_service.watchdog_loop(), name="blackout_watchdog")
    bot_task = asyncio.create_task(bot_service.polling_loop(), name="tg_bot_polling")
    trim_task = asyncio.create_task(_periodic_ram_trim_loop(), name="ram_trimmer")

    yield

    for t in (watchdog_task, bot_task, trim_task):
        t.cancel()
    await http_pool.close()
    memory_guard.trim_ram()


app = FastAPI(
    title=settings.PROJECT_NAME,
    version=settings.VERSION,
    default_response_class=ORJSONResponse,
    lifespan=lifespan,
)

app.include_router(api_router)


@app.get("/health", tags=["System"])
async def health_check():
    return {
        "status": "ok",
        "version": settings.VERSION,
        "online": state_service.is_online(),
        "queued_commands": len(state_service.command_queue),
        "ram_rss_mb": memory_guard.get_rss_mb(),
    }