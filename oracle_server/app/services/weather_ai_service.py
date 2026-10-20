import time
from typing import Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.state_service import state_service, MODES_UA
from app.services.audio_service import audio_service

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


class WeatherAiService:
    """
    Об'єднаний метео-аналізатор (Open-Meteo + BMP280) та Кастомний ШІ-Дух шкатулки.
    Підтримує гарячу зміну особистості (System Prompt) без перезапуску сервера.
    """

    __slots__ = ("_weather_cache", "_last_weather_fetch")

    def __init__(self) -> None:
        self._weather_cache: Dict[str, Any] = {}
        self._last_weather_fetch: float = 0.0

    async def get_combined_weather(self) -> Dict[str, Any]:
        now = time.time()
        if not self._weather_cache or (now - self._last_weather_fetch > 1800.0):
            url = (
                f"https://api.open-meteo.com/v1/forecast"
                f"?latitude={settings.WEATHER_LAT}&longitude={settings.WEATHER_LON}"
                f"&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m"
                f"&hourly=temperature_2m,precipitation_probability&forecast_days=1"
            )
            try:
                resp = await http_pool.get().get(url, timeout=5.0)
                if resp.status_code == 200:
                    raw = resp.json()
                    cur = raw.get("current", {})
                    hourly = raw.get("hourly", {})
                    probs = (hourly.get("precipitation_probability") or [0])[:6]
                    w_code = cur.get("weather_code", 0)
                    self._weather_cache = {
                        "outdoor_temp": cur.get("temperature_2m", 15.0),
                        "outdoor_hum": cur.get("relative_humidity_2m", 60),
                        "wind_kmh": cur.get("wind_speed_10m", 5.0),
                        "weather_code": w_code,
                        "weather_desc": WMO_CODES_UA.get(w_code, "🌤 Стабільна погода"),
                        "rain_prob_6h": max(probs) if probs else 0,
                    }
                    self._last_weather_fetch = now
            except Exception:
                if not self._weather_cache:
                    self._weather_cache = {
                        "outdoor_temp": 15.0,
                        "outdoor_hum": 55,
                        "wind_kmh": 5.0,
                        "weather_code": 1,
                        "weather_desc": "🌤 Дані оновлюються",
                        "rain_prob_6h": 10,
                    }

        d = state_service.latest_telemetry
        indoor_t = float(d.get("t", 22.0))
        indoor_h = float(d.get("h", 50.0))
        pressure = float(d.get("p", 750.0))
        p_diff = float(d.get("p_diff", 0.0))

        if p_diff <= -1.5 or (pressure < 743 and self._weather_cache.get("rain_prob_6h", 0) > 40):
            baro_verdict = "🌧 Барометр падає — насувається циклон / опади"
        elif p_diff >= 1.5 or pressure > 758:
            baro_verdict = "☀️ Тиск зростає — антициклон, буде ясно"
        else:
            baro_verdict = "🧭 Барометричний тиск стабільний"

        return {
            **self._weather_cache,
            "indoor_temp": indoor_t,
            "indoor_hum": indoor_h,
            "pressure_mm": pressure,
            "pressure_diff": p_diff,
            "baro_verdict": baro_verdict,
        }

    async def _generate_custom_ai_response(self, w: Dict[str, Any], custom_note: str) -> str:
        if not settings.GEMINI_API_KEY:
            return ""

        prompt = (
            f"[Системна роль / Персона: {state_service.ai_persona_name}]\n"
            f"{state_service.ai_system_prompt}\n\n"
            f"[Контекст датчиків MagicBox]:\n"
            f"• У кімнаті: {w['indoor_temp']:.1f}°C, вологість {w['indoor_hum']:.0f}%, "
            f"барометр {w['pressure_mm']:.0f} мм ({w['pressure_diff']:+.1f} мм, {w['baro_verdict']}).\n"
            f"• На вулиці: {w['weather_desc']}, {w['outdoor_temp']:+.1f}°C, вітер {w['wind_kmh']} км/год, "
            f"ризик дощу {w['rain_prob_6h']}%.\n"
            f"{('• Побажання / Контекст від користувача: ' + custom_note) if custom_note else ''}\n\n"
            "Напиши коротке (2-3 речення) живе повідомлення українською мовою у своєму стилі."
        )

        payload = {
            "contents": [{"parts": [{"text": prompt}]}],
            "generationConfig": {
                "temperature": state_service.ai_temperature,
                "maxOutputTokens": 220,
            },
        }
        headers = {"x-goog-api-key": settings.GEMINI_API_KEY, "Content-Type": "application/json"}

        for model_name in (settings.GEMINI_MODEL, "gemini-2.5-flash", "gemini-2.0-flash"):
            url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_name}:generateContent"
            try:
                resp = await http_pool.get().post(url, json=payload, headers=headers, timeout=7.5)
                if resp.status_code == 200:
                    data = resp.json()
                    text = data["candidates"][0]["content"]["parts"][0]["text"].strip()
                    if text:
                        return text
            except Exception:
                continue
        return ""

    async def generate_smart_briefing(self, custom_note: str = "", speak: bool = False) -> Dict[str, Any]:
        w = await self.get_combined_weather()
        in_t = w["indoor_temp"]
        out_t = w["outdoor_temp"]
        rain_p = w["rain_prob_6h"]
        p_mm = w["pressure_mm"]

        if rain_p >= 50 or w["weather_code"] >= 51 or p_mm < 743:
            rec_mode = 3  # 🔥 Камін з іскрами
        elif in_t < 19.5 or out_t < 5.0:
            rec_mode = 2  # 🍯 Бурштинова свічка
        elif in_t > 25.0 or out_t > 26.0:
            rec_mode = 7  # 🌊 Океанський бриз
        else:
            rec_mode = 5  # 🌸 Сакура Неон

        ai_text = await self._generate_custom_ai_response(w, custom_note)
        if not ai_text:
            rain_tip = "Візьми парасольку ☔!" if rain_p >= 45 else "Опадів не очікується ✨."
            ai_text = (
                f"{w['weather_desc']}, на вулиці {out_t:+.1f}°C (вітер {w['wind_kmh']} км/год). "
                f"У кімнаті {in_t:.1f}°C та {w['indoor_hum']:.0f}% вологості. "
                f"{w['baro_verdict']}. {rain_tip}\n"
                f"💡 Рекомендована атмосфера: {MODES_UA[rec_mode]}!"
            )

        screen_msg = custom_note[:80] if custom_note else f"Вул:{out_t:+.0f}°C Дощ:{rain_p}% | Дім:{in_t:.1f}°C {p_mm:.0f}мм"

        cmd_payload: Dict[str, Any] = {
            "set_mode": rec_mode,
            "sender": state_service.ai_persona_name[:28],
            "msg": screen_msg,
        }

        if speak:
            tts_res = await audio_service.speak_text(ai_text, enqueue=False)
            if tts_res.get("ok"):
                cmd_payload["play_url"] = tts_res["audio_url"]

        state_service.enqueue_raw(cmd_payload)

        return {
            "ok": True,
            "persona": state_service.ai_persona_name,
            "recommended_mode": rec_mode,
            "mode_name": MODES_UA[rec_mode],
            "screen_text": screen_msg,
            "ai_forecast": ai_text,
            "weather": w,
        }


weather_ai_service = WeatherAiService()