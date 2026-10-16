from typing import Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.state_service import state_service, MODES_UA
from app.services.weather_service import weather_service
from app.services.audio_service import audio_service


class AiService:
    """Інтелектуальний аналізатор клімату та настрою (Gemini Flash REST + Розумний локальний рушій)."""

    __slots__ = ()
    GEMINI_MODELS = ("gemini-3.8-flash", "gemini-3.5-flash", "gemini-2.5-flash")

    async def _ask_gemini(self, w: Dict[str, Any], custom_note: str) -> str:
        if not settings.GEMINI_API_KEY:
            return ""

        prompt = (
            "Ти — теплий, дотепний дух-охоронець розумної лампи-шкатулки MagicBox, яку Ярослав створив для Насті. "
            f"Дані в кімнаті: температура {w['indoor_temp']:.1f}°C, вологість {w['indoor_hum']:.0f}%, "
            f"тиск барометра {w['pressure_mm']:.0f} мм (зміна {w['pressure_diff']:+.1f} мм). "
            f"На вулиці зараз: {w['weather_desc']}, {w['outdoor_temp']:.1f}°C, вітер {w['wind_kmh']} км/год, "
            f"ймовірність опадів у найближчі години: {w['rain_prob_6h']}%. "
            f"Висновок барометра: {w['baro_verdict']}. "
            f"{('Додаткове побажання: ' + custom_note) if custom_note else ''} "
            "Напиши українською мовою короткий (2-3 речення), затишний і персоналізований прогноз-пораду "
            "(як одягтися або який настрій створити вдома)."
        )
        payload = {"contents": [{"parts": [{"text": prompt}]}]}
        headers = {"x-goog-api-key": settings.GEMINI_API_KEY, "Content-Type": "application/json"}

        for model_name in self.GEMINI_MODELS:
            url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_name}:generateContent"
            try:
                resp = await http_pool.get().post(url, json=payload, headers=headers, timeout=8.0)
                if resp.status_code == 200:
                    data = resp.json()
                    text = data["candidates"][0]["content"]["parts"][0]["text"].strip()
                    if text:
                        return text
            except Exception:
                continue
        return ""

    async def generate_smart_briefing(self, custom_note: str = "", speak: bool = False) -> Dict[str, Any]:
        w = await weather_service.get_combined_weather()
        in_t = w["indoor_temp"]
        out_t = w["outdoor_temp"]
        rain_p = w["rain_prob_6h"]
        p_mm = w["pressure_mm"]

        if rain_p >= 50 or w["weather_code"] >= 51 or p_mm < 743:
            rec_mode = 3  # 🔥 Камін з іскрами (дощ / циклон)
        elif in_t < 19.5 or out_t < 5.0:
            rec_mode = 2  # 🍯 Бурштинова свічка (прохолодно — зігріваємо)
        elif in_t > 25.0 or out_t > 26.0:
            rec_mode = 7  # 🌊 Океанський бриз (спекотно — освіжаємо)
        else:
            rec_mode = 5  # 🌸 Сакура Неон (комфортна погода)

        ai_full_text = await self._ask_gemini(w, custom_note)
        if not ai_full_text:
            rain_tip = "Візьми парасольку ☔!" if rain_p >= 45 else "Опадів не очікується ✨."
            ai_full_text = (
                f"{w['weather_desc']}, на вулиці {out_t:+.1f}°C (вітер {w['wind_kmh']} км/год). "
                f"У кімнаті затишні {in_t:.1f}°C та {w['indoor_hum']:.0f}% вологості. "
                f"{w['baro_verdict']}. {rain_tip}\n"
                f"💡 Вмикаю на шкатулці режим: {MODES_UA[rec_mode]}!"
            )

        screen_msg = custom_note[:58] if custom_note else f"Out:{out_t:+.0f}C Rain:{rain_p}% | In:{in_t:.1f}C {p_mm:.0f}mm"

        cmd_payload: Dict[str, Any] = {
            "set_mode": rec_mode,
            "sender": "AI Weather",
            "msg": screen_msg,
        }

        if speak:
            tts_res = await audio_service.speak_text(ai_full_text, enqueue=False)
            if tts_res.get("ok"):
                cmd_payload["play_url"] = tts_res["audio_url"]

        state_service.enqueue_raw(cmd_payload)

        return {
            "ok": True,
            "recommended_mode": rec_mode,
            "mode_name": MODES_UA[rec_mode],
            "screen_text": screen_msg,
            "ai_forecast": ai_full_text,
            "weather": w,
        }


ai_service = AiService()