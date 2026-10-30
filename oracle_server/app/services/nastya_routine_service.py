# Файл: oracle_server/app/services/nastya_routine_service.py
# Зворотна сумісність: сервіс перейменовано на routine_service
from app.services.routine_service import (
    RoutineService,
    routine_service,
    NastyaRoutineService,
    nastya_routine_service,
)

__all__ = [
    "RoutineService",
    "routine_service",
    "NastyaRoutineService",
    "nastya_routine_service",
]
