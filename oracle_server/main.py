import asyncio
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.responses import ORJSONResponse
from app.core.config import settings
from app.core.http_client import http_pool
from app.api.router import api_router
from app.services.state_service import state_service
from app.services.blackout_service import blackout_service
from app.services.bot_service import bot_service


@asynccontextmanager
async def lifespan(app: FastAPI):
    watchdog_task = asyncio.create_task(blackout_service.watchdog_loop())
    bot_task = asyncio.create_task(bot_service.polling_loop())
    yield
    watchdog_task.cancel()
    bot_task.cancel()
    state_service.save_snapshot()
    await http_pool.close()


app = FastAPI(
    title=settings.PROJECT_NAME,
    version=settings.VERSION,
    default_response_class=ORJSONResponse,
    lifespan=lifespan,
)

app.include_router(api_router)


@app.get("/", tags=["System"])
async def root_info():
    return {
        "service": settings.PROJECT_NAME,
        "version": settings.VERSION,
        "box_online": state_service.is_online(),
        "docs_url": "/docs",
        "health_url": "/health",
    }


@app.get("/health", tags=["System"])
async def health_check():
    return {
        "status": "ok",
        "version": settings.VERSION,
        "online": state_service.is_online(),
        "queued_commands": len(state_service.command_queue),
    }