import time
from typing import Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.state_service import state_service

WMO_CODES_UA = {
    0: "☀️ Ясне небо",
    1: "🌤 Переважно ясно",
    2: "⛅ Мінлива хмарність",
    3: "☁️ Похмуро",
    45: "🌫 Туман",
    48: "🌫 Паморозь",
    51: "🌦 Легка мряка",
    53: "🌦 Помірна мряка",
    61: "🌧 Невеликий дощ",
    63: "🌧 Помірний дощ",
    65: "🌧 Сильна злива",
    71: "🌨 Невеликий сніг",
    73: "🌨 Снігопад",
    80: "🌦 Зливові дощі",
    95: "⛈ Гроза",
}


class WeatherService:
    """Поєднує зовнішній метеопрогноз Open-Meteo з локальним барометром BMP280 шкатулки."""

    __slots__ = ("_cache", "_last_fetch")

    def __init__(self) -> None:
        self._cache: Dict[str, Any] = {}
        self._last_fetch: float = 0.0

    async def get_combined_weather(self) -> Dict[str, Any]:
        now = time.time()
        # Оновлюємо зовнішню погоду не частіше ніж раз на 30 хвилин (1800 с)
        if not self._cache or (now - self._last_fetch > 1800.0):
            url = (
                f"https://api.open-meteo.com/v1/forecast"
                f"?latitude={settings.WEATHER_LAT}&longitude={settings.WEATHER_LON}"
                f"&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m"
                f"&hourly=temperature_2m,precipitation_probability&forecast_days=1"
            )
            try:
                resp = await http_pool.get().get(url, timeout=6.0)
                if resp.status_code == 200:
                    raw = resp.json()
                    cur = raw.get("current", {})
                    hourly = raw.get("hourly", {})
                    probs = hourly.get("precipitation_probability", [0])[:6]
                    w_code = cur.get("weather_code", 0)
                    self._cache = {
                        "outdoor_temp": cur.get("temperature_2m", 15.0),
                        "outdoor_hum": cur.get("relative_humidity_2m", 60),
                        "wind_kmh": cur.get("wind_speed_10m", 5.0),
                        "weather_code": w_code,
                        "weather_desc": WMO_CODES_UA.get(w_code, "🌤 Стабільна погода"),
                        "rain_prob_6h": max(probs) if probs else 0,
                    }
                    self._last_fetch = now
            except Exception:
                if not self._cache:
                    self._cache = {
                        "outdoor_temp": 15.0,
                        "outdoor_hum": 55,
                        "wind_kmh": 5.0,
                        "weather_code": 1,
                        "weather_desc": "🌤 Дані оновлюються",
                        "rain_prob_6h": 10,
                    }

        # Додаємо локальний барометричний аналіз із датчиків SHT31 + BMP280 шкатулки
        d = state_service.latest_telemetry
        indoor_t = d.get("t", 22.0)
        indoor_h = d.get("h", 50.0)
        pressure = d.get("p", 750.0)
        p_diff = d.get("p_diff", 0.0)

        if p_diff <= -1.5 or (pressure < 743 and self._cache.get("rain_prob_6h", 0) > 40):
            baro_verdict = "🌧 Барометр стрімко падає — насувається циклон / опади"
        elif p_diff >= 1.5 or pressure > 758:
            baro_verdict = "☀️ Атмосферний тиск зростає — антициклон, буде ясно"
        else:
            baro_verdict = "🧭 Барометричний тиск стабільний — без різких змін"

        return {
            **self._cache,
            "indoor_temp": indoor_t,
            "indoor_hum": indoor_h,
            "pressure_mm": pressure,
            "pressure_diff": p_diff,
            "baro_verdict": baro_verdict,
        }


weather_service = WeatherService()