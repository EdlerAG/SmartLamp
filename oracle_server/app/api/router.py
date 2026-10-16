from fastapi import APIRouter
from app.api.endpoints_box import router as box_router
from app.api.endpoints_control import router as control_router

api_router = APIRouter()
api_router.include_router(box_router)
api_router.include_router(control_router)