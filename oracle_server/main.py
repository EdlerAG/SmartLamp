# Файл: oracle_server/main.py
import os

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
from app.services.database_service import db_service
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.bot_service import bot_service


@asynccontextmanager
async def lifespan(app: FastAPI):
    memory_guard.recalculate_disk_usage()
    memory_guard.trim_ram()

    # Єдиний фоновий слухач — Telegram Long-Polling, який спить на сервері Telegram по 25 с з 0% CPU
    bot_task = asyncio.create_task(bot_service.polling_loop(), name="tg_bot_polling")

    yield

    bot_task.cancel()
    blackout_service.shutdown()
    db_service.close()
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
        **memory_guard.get_telemetry(),
    }