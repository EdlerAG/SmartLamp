import time
import datetime
import orjson
import asyncio
import re
from typing import Optional, Dict, Any, Tuple

from app.core.logger import logger
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.database_service import db_service
from app.services.state_service import state_service, MODES_UA
from app.services.weather_ai_service import weather_ai_service
from app.services.audio_service import audio_service
from app.services.media_service import media_service
from app.services.spotify_service import spotify_service
from app.services.infographic_service import infographic_service
from app.services.nastya_routine_service import nastya_routine_service
from app.services.maintenance_service import maintenance_service
from app.services.google_calendar_service import google_calendar_service
from app.services.server_updater import server_updater

ADMIN_KB = {
    "keyboard": [
        [{"text": "🎛 Головний пульт"}, {"text": "💡 Освітлення"}, {"text": "🎵 Spotify"}],
        [{"text": "⏱ Фокус Pomodoro"}, {"text": "📊 Клімат кімнати"}, {"text": "📅 Календар & Розклад"}],
        [{"text": "🌙 Режим DND"}, {"text": "🔊 Гучність & Звук"}, {"text": "🖼 Екран & Заставка"}],
        [{"text": "🩺 Діагностика лампи"}, {"text": "🔄 Оновити сервер"}, {"text": "📌 Інструкція"}],
    ],
    "resize_keyboard": True,
}

GIRL_KB = {
    "keyboard": [
        [{"text": "🎛 Головний пульт"}, {"text": "💡 Освітлення"}, {"text": "🎵 Spotify"}],
        [{"text": "⏱ Фокус Pomodoro"}, {"text": "📊 Клімат кімнати"}, {"text": "📅 Календар & Розклад"}],
        [{"text": "🌙 Режим DND"}, {"text": "🔊 Гучність & Звук"}, {"text": "🖼 Екран & Заставка"}],
        [{"text": "🌤 Прогноз погоди"}, {"text": "📌 Інструкція"}],
    ],
    "resize_keyboard": True,
}

GUIDE_TEXT = """🪄 <b>ІНСТРУКЦІЯ SMARTLAMP SOVEREIGN v5.5</b>
━━━━━━━━━━━━━━━━━━
Твоя розумна шкатулка SmartLamp — це автономний центр затишку, світла, музики та мікроклімату кімнати:

💡 <b>СВІТЛО ТА РЕЖИМИ</b>
• 12 затишних атмосферних режимів (Теплий бурштин, Захід сонця, Жива свічка, Північне сяйво, Океанський бриз та ін.)
• Розумна авто-яскравість (підлаштовується під освітленість у кімнаті) або ручний вибір.
• Таймер автоматичного вимкнення світла перед сном (15, 30, 60 хв).

📊 <b>МІКРОКЛІМАТ ТА БЛЕКАУТИ</b>
• Високоточні сенсори SHT31 та BMP280: температура, вологість та атмосферний тиск із трендом.
• <b>«Чорна скринька» телеметрії</b>: якщо вимикається світло або Wi-Fi, шкатулка автономно записує показання в пам'ять, а при появі зв'язку виливає повний графік на сервер без прогалин!
• Графік за 24 години генерується в 1 клік кнопкою «📈 Графік».

🎵 <b>SPOTIFY VINYL HUB</b>
• Вініловий плеєр обертається на круглому дисплеї з живою обкладинкою треку.
• Підсвітка <b>AmbiLight</b> автоматично підлаштовує колір під колір обкладинки поточної пісні!

⏱ <b>ФОКУС ТА НАВЧАННЯ (POMODORO)</b>
• Режими фокусу 15, 25, 45 або 60 хвилин.
• На круглому дисплеї з'являється елегантний хорологічний циферблат відліку, лампа приглушує зайві звуки та вмикає м'яке робоче світло.

📅 <b>КАЛЕНДАР ТА РОЗКЛАД</b>
• Щоб додати подію на екран лампи, просто напиши в чат:
  <code>14:30 Семінар</code> або <code>Завтра 11:00 Іспит</code>.
• Найближча подія автоматично відображається на дисплеї шкатулки!

🖼 <b>АНІМАЦІЇ, ЗАСТАВКИ ТА ФОТО</b>
• Надішли будь-яку GIF-анімацію або фотографію в цей чат — і вона миттєво відтвориться на круглому дисплеї!
• Можна встановити її постійною заставкою або за таймером.

🎙 <b>ГОЛОСОВІ ПОВІДОМЛЕННЯ</b>
• Надішли голосове повідомлення в цей чат — і лампа відтворить його через вбудований динамік!

🌙 <b>РЕЖИМ DND (ТИХА НІЧ)</b>
• Повна тиша: динамік стає на MUTE, кліки вимикаються, світло переходить у нічник.

📱 <b>SMART STUDIO (MINI APP)</b>
• Натискай кнопку <b>«📱 Web Studio»</b> у головному пульті або біля меню, щоб відкрити інтерактивну панель керування прямо в Telegram!
━━━━━━━━━━━━━━━━━━
<i>Натисни «🎛 Головний пульт» нижче, щоб відкрити панель керування.</i>"""


class TelegramBotService:
    __slots__ = ("last_update_id", "synced", "webhook_cleared", "admin_preview_girl")

    def __init__(self) -> None:
        self.last_update_id: int = 0
        self.synced: bool = False
        self.webhook_cleared: bool = False
        self.admin_preview_girl: bool = False

    async def _api(self, method: str, payload: Dict[str, Any], timeout: float = 8.0) -> Optional[Dict[str, Any]]:
        if not settings.TG_BOT_TOKEN:
            return None
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/{method}"
        try:
            resp = await http_pool.get().post(url, json=payload, timeout=timeout)
            if resp.status_code == 200:
                return resp.json()
        except Exception:
            pass
        return None

    async def _send_photo_bytes(self, chat_id: str, photo_bytes: bytes, caption: str, reply_markup: Optional[Dict[str, Any]] = None) -> bool:
        if not settings.TG_BOT_TOKEN:
            return False
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/sendPhoto"
        files = {"photo": ("infographic.png", photo_bytes, "image/png")}
        data: Dict[str, Any] = {"chat_id": chat_id, "caption": caption}
        if reply_markup:
            data["reply_markup"] = orjson.dumps(reply_markup).decode("utf-8")
        try:
            resp = await http_pool.get().post(url, data=data, files=files, timeout=20.0)
            return resp.status_code == 200
        except Exception:
            return False

    async def _download_tg_file(self, file_id: str) -> Optional[bytes]:
        f_info = await self._api("getFile", {"file_id": file_id})
        f_path = (f_info.get("result") or {}).get("file_path") if isinstance(f_info, dict) else None
        if not f_path:
            return None
        dl_url = f"https://api.telegram.org/file/bot{settings.TG_BOT_TOKEN}/{f_path}"
        try:
            resp = await http_pool.get().get(dl_url, timeout=25.0)
            return resp.content if resp.status_code == 200 else None
        except Exception:
            return None

    async def _pin_message(self, chat_id: str, message_id: int) -> bool:
        """Автоматично закріплює вітальне вікно-інструкцію в шапці чату."""
        res = await self._api("pinChatMessage", {
            "chat_id": chat_id,
            "message_id": message_id,
            "disable_notification": True,
        })
        return bool(res and res.get("ok"))

    @staticmethod
    def _web_button(text: str = "📱 Відкрити Web Studio") -> Dict[str, Any]:
        """Формує нативну кнопку Mini App (web_app) для HTTPS або звичайний URL лінк з Cache-Buster."""
        base_url = state_service.resolve_public_url(settings.PUBLIC_SERVER_URL)
        clean_base = base_url.rstrip("/")
        app_url = f"{clean_base}/?v=5.1.0"
        if app_url.startswith("https://"):
            return {"text": text, "web_app": {"url": app_url}}
        return {"text": text, "url": app_url}

    def _hub_main_view(self, chat_id: str, is_admin: bool, is_girl: bool) -> Tuple[str, Dict[str, Any]]:
        """Головний пульт «кнопка в кнопці»: компактне багатошарове меню."""
        text = self._build_status_text()
        buttons = [
            [
                {"text": "💡 Світло & Ефекти", "callback_data": "hub:light"},
                {"text": "📊 Клімат & Погода", "callback_data": "hub:climate"},
            ],
            [
                {"text": "🎵 Музика & Звук", "callback_data": "hub:audio"},
                {"text": "⏱ Фокус & Сон", "callback_data": "hub:routine"},
            ],
            [
                {"text": "🖼 Екран & Заставка", "callback_data": "hub:screen"},
                self._web_button("📱 Web Studio"),
            ],
        ]
        if is_admin:
            buttons.append([
                {"text": "🛠 Адмін-Центр", "callback_data": "hub:admin"},
                {"text": "🔄 Оновити статус", "callback_data": "hub:refresh"},
            ])
        else:
            buttons.append([
                {"text": "🔄 Оновити статус", "callback_data": "hub:refresh"},
                {"text": "📌 Інструкція", "callback_data": "hub:guide"},
            ])
        return text, {"inline_keyboard": buttons}

    @staticmethod
    def _modes_inline_kb() -> Dict[str, Any]:
        cur_m = state_service.latest_telemetry.get("m", 0)
        rows = []
        for i in range(0, len(MODES_UA), 2):
            row = [{"text": f"{'✅ ' if cur_m == i else ''}{MODES_UA[i]}", "callback_data": f"mode:{i}"}]
            if i + 1 < len(MODES_UA):
                row.append({"text": f"{'✅ ' if cur_m == i + 1 else ''}{MODES_UA[i + 1]}", "callback_data": f"mode:{i + 1}"})
            rows.append(row)
        rows.append([
            {"text": "🔆 Яскравість", "callback_data": "menu:brt"},
            {"text": "⏳ Таймер сну", "callback_data": "menu:sleep"},
        ])
        rows.append([
            {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
        ])
        return {"inline_keyboard": rows}

    @staticmethod
    def _brt_inline_kb() -> Dict[str, Any]:
        d = state_service.latest_telemetry
        auto_b = d.get("auto_b", True)
        b_idx = d.get("b_idx", 2)
        return {
            "inline_keyboard": [
                [{"text": f"{'✅ ' if auto_b else ''}🤖 Автоматична (Фоторезистор)", "callback_data": "brt:auto"}],
                [
                    {"text": f"{'✅ ' if not auto_b and b_idx == 0 else ''}🌙 15% Нічник", "callback_data": "brt:0"},
                    {"text": f"{'✅ ' if not auto_b and b_idx == 1 else ''}☕ 35% М'яко", "callback_data": "brt:1"},
                ],
                [
                    {"text": f"{'✅ ' if not auto_b and b_idx == 2 else ''}✨ 65% Робота", "callback_data": "brt:2"},
                    {"text": f"{'✅ ' if not auto_b and b_idx == 3 else ''}☀️ 100% Максимум", "callback_data": "brt:3"},
                ],
                [
                    {"text": "💡 До вибору режимів", "callback_data": "hub:light"},
                    {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
                ],
            ]
        }

    @staticmethod
    def _sleep_inline_kb() -> Dict[str, Any]:
        return {
            "inline_keyboard": [
                [
                    {"text": "⏳ 15 хв", "callback_data": "sleep:15"},
                    {"text": "⏳ 30 хв", "callback_data": "sleep:30"},
                    {"text": "⏳ 45 хв", "callback_data": "sleep:45"},
                ],
                [
                    {"text": "⏳ 60 хв", "callback_data": "sleep:60"},
                    {"text": "⏳ 90 хв", "callback_data": "sleep:90"},
                    {"text": "❌ Скасувати", "callback_data": "sleep:0"},
                ],
                [
                    {"text": "💡 До вибору режимів", "callback_data": "hub:light"},
                    {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
                ],
            ]
        }

    @staticmethod
    def _volume_inline_kb() -> Dict[str, Any]:
        d = state_service.latest_telemetry
        vol = d.get("vol", 12)
        muted = d.get("muted", False)
        t_beep = d.get("t_beep", True)
        mute_lbl = "🔊 Увімкнути звук" if muted else "🔇 Без звуку (Mute)"
        beep_lbl = f"🔘 Кліки кнопок: {'ВКЛ' if t_beep else 'ВИКЛ'}"
        return {
            "inline_keyboard": [
                [
                    {"text": mute_lbl, "callback_data": "vol:mute"},
                    {"text": f"{'✅ ' if not muted and vol <= 5 else ''}🌙 Тихо (20%)", "callback_data": "vol:4"},
                ],
                [
                    {"text": f"{'✅ ' if not muted and 10 <= vol <= 14 else ''}☕ Комфорт (55%)", "callback_data": "vol:12"},
                    {"text": f"{'✅ ' if not muted and vol >= 19 else ''}🔊 Гучно (90%)", "callback_data": "vol:20"},
                ],
                [
                    {"text": "🔉 -10%", "callback_data": "vol:down"},
                    {"text": "🔊 +10%", "callback_data": "vol:up"},
                    {"text": beep_lbl, "callback_data": "vol:beep"},
                ],
                [
                    {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
                ],
            ]
        }

    @staticmethod
    def _pomodoro_inline_kb() -> Dict[str, Any]:
        st = nastya_routine_service.get_pomodoro_status()
        active = st.get("active", False)
        mode = st.get("mode", "idle")
        rem_min = st.get("remaining_min", 0)

        rows = []
        if active:
            status_txt = f"Активний ({mode.upper()}): залишилось {rem_min} хв"
            rows.append([{"text": f"🛑 Зупинити ({status_txt})", "callback_data": "pomo:stop"}])
        else:
            rows.append([
                {"text": "⏱ 15 хв (Короткий)", "callback_data": "pomo:15"},
                {"text": "⏱ 25 хв (Класичний)", "callback_data": "pomo:25"},
            ])
            rows.append([
                {"text": "⏱ 45 хв (Глибокий)", "callback_data": "pomo:45"},
                {"text": "⏱ 60 хв (Сесія)", "callback_data": "pomo:60"},
            ])
        if spotify_service.is_linked():
            rows.append([{"text": "🎧 Фокус-музика у Spotify (Play/Pause)", "callback_data": "pomo:spotify_lofi"}])
        rows.append([{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}])
        return {"inline_keyboard": rows}

    @staticmethod
    def _dnd_inline_kb() -> Dict[str, Any]:
        d = state_service.latest_telemetry
        dnd = d.get("dnd", False)
        qh = nastya_routine_service.get_quiet_hours()
        toggle_txt = "☀️ Вимкнути режим DND" if dnd else "🌙 Увімкнути режим DND (Тиша + нічник)"
        qh_txt = f"Тихі години: {qh['start']} - {qh['end']} ({'ВКЛ' if qh['enabled'] else 'ВИКЛ'})"
        return {
            "inline_keyboard": [
                [{"text": toggle_txt, "callback_data": "dnd:toggle"}],
                [{"text": f"⏰ {qh_txt}", "callback_data": "dnd:quiet_info"}],
                [{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
            ]
        }

    @staticmethod
    async def _calendar_view(base_url: str = "") -> Tuple[str, Dict[str, Any]]:
        is_linked = google_calendar_service.is_linked()
        link_type = google_calendar_service.get_link_type()
        if link_type == "ical":
            link_status = "Google Календар: <b>Підключено (iCal) ✅</b>"
        elif link_type == "oauth":
            link_status = "Google Календар: <b>Підключено (OAuth) ✅</b>"
        else:
            link_status = "Google Календар: <b>Не підключено ❌</b>"

        next_ev = await google_calendar_service.get_next_event()
        events = await google_calendar_service.get_upcoming_events(max_results=8)

        next_txt = "<i>Немає запланованих подій на найближчий час</i>"
        if next_ev:
            soon_badge = " 🔥 <b>(Скоро!)</b>" if next_ev.get("is_soon") else ""
            min_txt = f" (через {next_ev['min_until']} хв)" if next_ev.get("min_until", 0) > 0 else ""
            next_txt = f"• <b>{next_ev['time']}</b> — {next_ev['summary']}{min_txt}{soon_badge}"

        ev_lines = []
        for ev in events:
            d_str = f" ({ev['date']})" if ev.get("date") else ""
            src_sym = "🇬" if ev.get("source") == "google" else "📝"
            ev_lines.append(f"{src_sym} <b>{ev['time']}{d_str}</b> — {ev['summary']}")

        ev_block = "\n".join(ev_lines) if ev_lines else "<i>Список подій порожній</i>"

        text = (
            f"📅 <b>КАЛЕНДАР ТА РОЗКЛАД</b>\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"{link_status}\n\n"
            f"📌 <b>Найближча подія на екрані лампи:</b>\n{next_txt}\n\n"
            f"📋 <b>Майбутні події:</b>\n{ev_block}\n\n"
            f"💡 <i>Щоб додати подію, напишіть у чат повідомлення у форматі:\n<code>15:30 Зустріч</code> або <code>Завтра 11:00 Семінар</code></i>"
        )

        buttons = []
        for ev in events[:4]:
            buttons.append([{"text": f"🗑 {ev['time']} {ev['summary'][:18]}", "callback_data": f"cal_act:del_{ev['id']}"}])
        buttons.append([{"text": "🔄 Оновити розклад", "callback_data": "cal_act:refresh"}])
        if not is_linked:
            auth_url = google_calendar_service.get_authorize_url(base_url)
            if auth_url:
                buttons.append([{"text": "🔗 Підключити Google Календар", "url": auth_url}])
            else:
                buttons.append([{"text": "⚙️ Налаштувати Google ID/Secret", "callback_data": "google:setup_info"}])
        else:
            buttons.append([{"text": "🔌 Відключити Google Календар", "callback_data": "cal_act:disconnect"}])
        buttons.append([{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}])

        return text, {"inline_keyboard": buttons}

    @staticmethod
    def _build_status_text() -> str:
        d = state_service.latest_telemetry
        is_on = state_service.is_online()
        sec = state_service.seconds_since_seen()
        status_line = "🟢 В мережі" if is_on else f"🔴 Офлайн ({sec or '?'} с тому)"

        t_val = d.get("t", 22.0)
        h_val = d.get("h", 50.0)
        p_val = d.get("p", 750.0)
        p_diff = d.get("p_diff", 0.0)
        trend_sym = "→ стабільно" if abs(p_diff) < 0.7 else ("↑ зростає" if p_diff > 0 else "↓ падає")

        # Оцінка мікроклімату
        hum_eval = "Комфортно"
        if h_val < 40:
            hum_eval = "Сухе повітря"
        elif h_val > 65:
            hum_eval = "Висока вологість"

        l_pct = d.get("l", 50)
        m_idx = d.get("m", 0)
        m_name = MODES_UA[m_idx % len(MODES_UA)]
        auto_b = d.get("auto_b", True)
        b_pct = d.get("b_pct", 65)
        vol = d.get("vol", 12)
        muted = d.get("muted", False)
        t_beep = d.get("t_beep", True)
        rssi = d.get("rssi", -60)
        dnd = d.get("dnd", False)

        pomo_st = nastya_routine_service.get_pomodoro_status()
        pomo_line = f"⏱ Фокус Pomodoro: <b>{pomo_st['remaining_min']} хв ({pomo_st['mode'].upper()})</b>" if pomo_st["active"] else "⏱ Фокус Pomodoro: <b>Вимкнено</b>"

        vol_str = "🔇 MUTE" if muted or vol == 0 else f"{int(vol * 100 / 21)}% ({vol}/21)"
        brt_str = "Авто" if auto_b else f"Ручна ({b_pct}%)"

        sp_st = spotify_service.get_cached_state()
        sp_line = f"🎵 Spotify: {sp_st['title']} — {sp_st['artist']}" if sp_st.get("playing") else "🎵 Spotify: Пауза / Не активний"

        return (
            f"🏠 <b>SMARTLAMP · СТАТУС ТА МІКРОКЛІМАТ</b>\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"📡 Зв'язок: {status_line} (Wi-Fi: {rssi} dBm)\n"
            f"🌡 Температура: <b>{t_val:.1f}°C</b>\n"
            f"💧 Вологість: <b>{h_val:.0f}%</b> ({hum_eval})\n"
            f"🧭 Атмосферний тиск: <b>{p_val:.0f} мм рт. ст.</b> ({trend_sym})\n"
            f"☀️ Освітленість у кімнаті: <b>{l_pct}%</b>\n\n"
            f"💡 Освітлення: <b>{m_name}</b> ({brt_str})\n"
            f"{pomo_line}\n"
            f"🌙 Режим DND: <b>{'Увімкнено 🌙' if dnd else 'Вимкнено'}</b>\n"
            f"🔊 Гучність: <b>{vol_str}</b> (Кліки: {'ВКЛ' if t_beep else 'ВИКЛ'})\n"
            f"{sp_line}"
        )

    @staticmethod
    def _build_spotify_view(base_url: str = "") -> Tuple[str, Dict[str, Any]]:
        st = spotify_service.get_cached_state()
        playing = st.get("playing", False)
        title = st.get("title", "")
        artist = st.get("artist", "")
        prog_ms = st.get("progress_ms", 0)
        dur_ms = st.get("duration_ms", 0)

        c_sec = prog_ms // 1000
        d_sec = dur_ms // 1000
        time_str = f"{c_sec // 60:02d}:{c_sec % 60:02d} / {d_sec // 60:02d}:{d_sec % 60:02d}"

        if not spotify_service.is_linked():
            auth_url = spotify_service.get_authorize_url(base_url)
            text = (
                "🎵 <b>SPOTIFY VINYL HUB · НЕ ПІДКЛЮЧЕНО</b>\n\n"
                "Для синхронізації вінілового плеєра на дисплеї лампи, динамічної підсвітки AmbiLight "
                "та відображення назв треків — підключіть Spotify акаунт через OAuth2:"
            )
            buttons = []
            if auth_url:
                buttons.append([{"text": "🔗 Підключити Spotify", "url": auth_url}])
            else:
                text += "\n\n⚠️ <i>Ключі Spotify ще не налаштовані. Натисніть кнопку нижче для швидкого налаштування.</i>"
                buttons.append([{"text": "⚙️ Як налаштувати Spotify (інструкція)", "callback_data": "spotify:setup_info"}])
            buttons.append([
                {"text": "🔄 Перевірити статус", "callback_data": "spotify:refresh"},
                {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
            ])
            return text, {"inline_keyboard": buttons}

        status_str = "▶️ Грає" if playing else "⏸ Пауза"
        track_info = f"<b>{title}</b>\n👤 {artist}\n⏱ {time_str}" if title else "<i>Очікування початку треку...</i>"
        text = (
            f"🎵 <b>SPOTIFY ПЛЕЄР</b>\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"Статус: {status_str}\n\n"
            f"{track_info}\n\n"
            f"💡 Вінілова платівка обертається на екрані лампи (Вікно 3) та живить AmbiLight підсвітку."
        )

        kb = {
            "inline_keyboard": [
                [
                    {"text": "⏮ Попередній", "callback_data": "sp_act:previous"},
                    {"text": "⏸ Пауза" if playing else "▶️ Грати", "callback_data": "sp_act:play_pause"},
                    {"text": "⏭ Наступний", "callback_data": "sp_act:next"},
                ],
                [
                    {"text": "🎨 Увімкнути Spotify AmbiLight", "callback_data": "mode:11"},
                    {"text": "🔄 Оновити", "callback_data": "spotify:refresh"},
                ],
                [
                    {"text": "🔌 Відключити Spotify", "callback_data": "sp_act:disconnect"},
                    {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
                ],
            ]
        }
        return text, kb

    @staticmethod
    def _gallery_inline_kb() -> Dict[str, Any]:
        media_list = db_service.list_custom_media()
        visual_media = [m for m in media_list if m.get("media_type") != "sound" and not m.get("name", "").startswith("voice_")]
        if not visual_media:
            return {
                "inline_keyboard": [
                    [{"text": "📭 Галерея порожня. Надішліть гіфку!", "callback_data": "noop"}],
                    [{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
                ]
            }

        rows = []
        for i in range(0, min(len(visual_media), 10), 2):
            m1 = visual_media[i]
            r = [{"text": f"🎬 {m1['name'][:14]}", "callback_data": f"play_media:{m1['name']}"}]
            if i + 1 < len(visual_media):
                m2 = visual_media[i + 1]
                r.append({"text": f"🎬 {m2['name'][:14]}", "callback_data": f"play_media:{m2['name']}"})
            rows.append(r)

        rows.append([
            {"text": "⏹ Зупинити показ (Годинник)", "callback_data": "med_stop"},
            {"text": "⬅️ Головне меню", "callback_data": "menu:hub"},
        ])
        return {"inline_keyboard": rows}

    @staticmethod
    def _media_control_inline_kb(media_name: str) -> Dict[str, Any]:
        return {
            "inline_keyboard": [
                [
                    {"text": "♾ Постійний показ", "callback_data": "med_timer:0"},
                    {"text": "⏹ Зупинити показ", "callback_data": "med_stop"},
                ],
                [
                    {"text": "⏱ 15 хв", "callback_data": "med_timer:15"},
                    {"text": "⏱ 1 год", "callback_data": "med_timer:60"},
                    {"text": "⏱ 3 год", "callback_data": "med_timer:180"},
                ],
                [
                    {"text": "📌 Встановити заставкою", "callback_data": f"med_saver:{media_name}"},
                    {"text": "🎭 До галереї", "callback_data": "menu:gallery"},
                ],
            ]
        }

    async def _handle_callback(self, cb: Dict[str, Any]) -> None:
        cb_id = cb.get("id", "")
        data = cb.get("data", "")
        chat_id = str((cb.get("message") or {}).get("chat", {}).get("id", ""))
        msg_id = (cb.get("message") or {}).get("message_id")

        ALLOWED_ADMINS = {state_service.admin_id, "1047202957"}
        ALLOWED_GIRLS = {state_service.girl_id, "1294097414", "1334785533"}

        if chat_id not in ALLOWED_ADMINS and chat_id not in ALLOWED_GIRLS:
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "⛔ Доступ обмежено."})
            return

        is_admin = (chat_id in ALLOWED_ADMINS)
        is_girl = (chat_id in ALLOWED_GIRLS)
        if self.admin_preview_girl and is_admin:
            is_admin = False
            is_girl = True

        if data.startswith("mode:"):
            m = int(data.split(":")[1])
            state_service.enqueue_raw({"set_mode": m})
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Режим: {MODES_UA[m % len(MODES_UA)]}"})
            await self._api("editMessageReplyMarkup", {"chat_id": chat_id, "message_id": msg_id, "reply_markup": self._modes_inline_kb()})

        elif data.startswith("brt:"):
            arg = data.split(":")[1]
            if arg == "auto":
                state_service.enqueue_raw({"set_brt": "auto"})
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Авто-яскравість увімкнена"})
            else:
                idx = int(arg)
                state_service.enqueue_raw({"set_brt": str(idx)})
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Яскравість: {idx}"})
            await self._api("editMessageReplyMarkup", {"chat_id": chat_id, "message_id": msg_id, "reply_markup": self._brt_inline_kb()})

        elif data.startswith("sleep:"):
            dur = int(data.split(":")[1])
            state_service.enqueue_raw({"sleep": dur})
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Таймер сну: {dur} хв" if dur > 0 else "Таймер сну вимкнено"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": f"⏳ Таймер сну встановлено на {dur} хв." if dur > 0 else "⏳ Таймер сну скасовано.",
                "reply_markup": self._modes_inline_kb(),
            })

        elif data.startswith("vol:"):
            v_arg = data.split(":")[1]
            d = state_service.latest_telemetry
            cur_vol = d.get("vol", 12)
            cur_muted = d.get("muted", False)
            cur_beep = d.get("t_beep", True)

            if v_arg == "mute":
                new_m = not cur_muted
                state_service.enqueue_raw({"muted": new_m})
                ans = "🔇 Звук вимкнено (Mute)" if new_m else "🔊 Звук увімкнено"
            elif v_arg == "beep":
                new_b = not cur_beep
                state_service.enqueue_raw({"touch_beep": new_b})
                ans = f"🔘 Кліки кнопок: {'ВКЛ' if new_b else 'ВИКЛ'}"
            elif v_arg == "up":
                new_v = min(21, cur_vol + 2)
                state_service.enqueue_raw({"volume": new_v, "muted": False})
                ans = f"🔊 Гучність: {int(new_v * 100 / 21)}%"
            elif v_arg == "down":
                new_v = max(0, cur_vol - 2)
                state_service.enqueue_raw({"volume": new_v})
                ans = f"🔉 Гучність: {int(new_v * 100 / 21)}%"
            else:
                new_v = int(v_arg)
                state_service.enqueue_raw({"volume": new_v, "muted": False})
                ans = f"🔊 Гучність: {int(new_v * 100 / 21)}%"

            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": ans})
            await self._api("editMessageReplyMarkup", {"chat_id": chat_id, "message_id": msg_id, "reply_markup": self._volume_inline_kb()})

        elif data.startswith("pomo:"):
            act = data.split(":", 1)[1]
            if act == "stop":
                nastya_routine_service.stop_pomodoro()
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🛑 Pomodoro зупинено"})
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": "🛑 <b>Фокус Pomodoro зупинено</b>. Лампа повернулась до звичайного режиму.",
                    "parse_mode": "HTML",
                    "reply_markup": self._pomodoro_inline_kb(),
                })
            elif act == "spotify_lofi":
                cur_p = spotify_service.get_cached_state().get("playing", False)
                await spotify_service.player_action("pause" if cur_p else "play")
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🎧 Spotify фокус-трек перемкнуто"})
                await self._api("editMessageReplyMarkup", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "reply_markup": self._pomodoro_inline_kb(),
                })
            else:
                try:
                    dur = int(act)
                except ValueError:
                    dur = 25
                nastya_routine_service.start_pomodoro(dur)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"⏳ Фокус {dur} хв запущено!"})
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": (
                        f"⏱ <b>ФОКУС POMODORO · АКТИВНО</b>\n"
                        f"━━━━━━━━━━━━━━━━━━\n"
                        f"Тривалість: <b>{dur} хвилин</b>\n"
                        f"На лампі увімкнено хорологічний циферблат відліку та спокійне бурштинове світло."
                    ),
                    "parse_mode": "HTML",
                    "reply_markup": self._pomodoro_inline_kb(),
                })

        elif data == "dnd:toggle":
            d = state_service.latest_telemetry
            cur_dnd = d.get("dnd", False)
            new_dnd = not cur_dnd
            state_service.enqueue_raw({
                "dnd": new_dnd,
                "muted": new_dnd,
                "set_brt": "0" if new_dnd else "auto",
            })
            ans = "🌙 Режим DND увімкнено (Нічник + MUTE)" if new_dnd else "☀️ Режим DND вимкнено"
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": ans})
            await self._api("editMessageReplyMarkup", {"chat_id": chat_id, "message_id": msg_id, "reply_markup": self._dnd_inline_kb()})

        elif data == "dnd:quiet_info":
            qh = nastya_routine_service.get_quiet_hours()
            await self._api("answerCallbackQuery", {
                "callback_query_id": cb_id,
                "text": f"Тихі години: щодня з {qh['start']} до {qh['end']}. Звук вимикається автоматично.",
                "show_alert": True,
            })

        elif data.startswith("cal_act:"):
            sub = data.split(":", 1)[1]
            if sub == "refresh":
                google_calendar_service._cached_event = None
                cal_t, cal_k = await self._calendar_view(settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Розклад оновлено!"})
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": cal_t, "parse_mode": "HTML", "reply_markup": cal_k})
            elif sub.startswith("del_"):
                ev_id = sub[4:]
                await google_calendar_service.delete_event(ev_id)
                google_calendar_service._cached_event = None
                cal_t, cal_k = await self._calendar_view(settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🗑 Подію видалено!"})
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": cal_t, "parse_mode": "HTML", "reply_markup": cal_k})
            elif sub == "disconnect":
                google_calendar_service.disconnect()
                cal_t, cal_k = await self._calendar_view(settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Google Календар відключено"})
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": cal_t, "parse_mode": "HTML", "reply_markup": cal_k})

        elif data.startswith("sp_act:"):
            action = data.split(":")[1]
            if action == "disconnect":
                spotify_service.disconnect()
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Spotify відключено"})
                sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": sp_t, "parse_mode": "HTML", "reply_markup": sp_k})
            elif action == "play_pause":
                cur_p = spotify_service.get_cached_state().get("playing", False)
                await spotify_service.player_action("pause" if cur_p else "play")
                await asyncio.sleep(0.4)
                sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Spotify: play/pause"})
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": sp_t, "parse_mode": "HTML", "reply_markup": sp_k})
            else:
                await spotify_service.player_action(action)
                await asyncio.sleep(0.4)
                sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Spotify: {action}"})
                await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": sp_t, "parse_mode": "HTML", "reply_markup": sp_k})

        elif data == "spotify:refresh":
            await asyncio.sleep(0.3)
            sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Дані Spotify оновлено"})
            await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": sp_t, "parse_mode": "HTML", "reply_markup": sp_k})

        elif data.startswith("play_media:"):
            name = data.split(":", 1)[1]
            try:
                media_service.trigger_saved_media(name, settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Запущено '{name}'!"})
            except Exception as e:
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Помилка: {e}"})

        elif data.startswith("med_timer:"):
            t_min = int(data.split(":")[1])
            media_service.set_timer(t_min)
            msg_t = f"⏱ Таймер показу: {t_min} хв!" if t_min > 0 else "♾ Анімація крутиться постійно!"
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": msg_t})

        elif data == "med_stop":
            media_service.stop_media()
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "⏹ Показ зупинено! Екран повернуто"})

        elif data.startswith("med_saver:"):
            s_name = data.split(":", 1)[1]
            try:
                media_service.trigger_saved_media(s_name, settings.PUBLIC_SERVER_URL, set_as_saver=True)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"📌 '{s_name}' встановлено заставкою!"})
            except Exception as e:
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Помилка: {e}"})

        elif data == "menu:gallery":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🖼 <b>ГАЛЕРЕЯ АНІМАЦІЙ ТА ЗАСТАВОК:</b>\nОберіть елемент для виведення на круглий дисплей:",
                "parse_mode": "HTML",
                "reply_markup": self._gallery_inline_kb(),
            })

        elif data == "menu:modes":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "💡 <b>ВИБІР РЕЖИМУ ОСВІТЛЕННЯ:</b>",
                "parse_mode": "HTML",
                "reply_markup": self._modes_inline_kb(),
            })

        elif data == "menu:brt":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🔆 <b>РЕГУЛЮВАННЯ ЯСКРАВОСТІ СВІТЛА:</b>",
                "parse_mode": "HTML",
                "reply_markup": self._brt_inline_kb(),
            })

        elif data == "menu:sleep":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "⏳ <b>ТАЙМЕР СНУ:</b>\nОберіть тривалість автоматичного вимкнення світла:",
                "parse_mode": "HTML",
                "reply_markup": self._sleep_inline_kb(),
            })

        elif data == "menu:infographic_get":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Генерую графік..."})
            png_bytes = infographic_service.generate_status_infographic()
            await self._send_photo_bytes(chat_id, png_bytes, caption="📊 Мікроклімат за останні 24 години.")

        elif data == "menu:hub":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            hub_t, hub_k = self._hub_main_view(chat_id, is_admin, is_girl)
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": hub_k,
            })

        elif data == "hub:light":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "💡 <b>КЕРУВАННЯ ОСВІТЛЕННЯМ</b>\nОберіть режим атмосферного освітлення або налаштуйте яскравість:",
                "parse_mode": "HTML",
                "reply_markup": self._modes_inline_kb(),
            })

        elif data == "hub:climate":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            d = state_service.latest_telemetry
            t_val = d.get("t", 22.0)
            h_val = d.get("h", 50.0)
            p_val = d.get("p", 750.0)
            p_diff = d.get("p_diff", 0.0)
            trend_sym = "→ стабільно" if abs(p_diff) < 0.7 else ("↑ зростає" if p_diff > 0 else "↓ падає")
            clim_t = (
                f"📊 <b>МІКРОКЛІМАТ КІМНАТИ ТА ПОГОДА</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"🌡 Температура: <b>{t_val:.1f}°C</b>\n"
                f"💧 Вологість: <b>{h_val:.0f}%</b>\n"
                f"🧭 Атмосферний тиск: <b>{p_val:.0f} мм рт. ст.</b> ({trend_sym})\n\n"
                f"💾 <b>Чорна скринька</b>: сенсори записують історію навіть під час відключень світла!"
            )
            clim_k = {
                "inline_keyboard": [
                    [
                        {"text": "📈 Графік за 24 години", "callback_data": "menu:infographic_get"},
                        {"text": "🌤 Прогноз для Києва", "callback_data": "hub:weather"},
                    ],
                    [{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
                ]
            }
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": clim_t,
                "parse_mode": "HTML",
                "reply_markup": clim_k,
            })

        elif data == "hub:weather":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            res = await weather_ai_service.generate_smart_briefing()
            w = res["weather"]
            rain_tip = "Варто взяти парасольку ☂️" if w.get("rain_prob_6h", 0) >= 45 else "Опадів найближчим часом не очікується"
            w_msg = (
                f"🌤 <b>ПОГОДА ТА ПРОГНОЗ ДЛЯ КИЄВА</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"• На вулиці: <b>{w.get('weather_desc', 'Ясно')}</b>, {w.get('outdoor_temp', 15.0):+.1f}°C\n"
                f"• Вітер: {w.get('wind_kmh', 5.0)} км/год, ймовірність опадів: {w.get('rain_prob_6h', 0)}%\n"
                f"• Барометр: {w.get('pressure_mm', 750.0):.0f} мм рт. ст. ({w.get('baro_verdict', 'стабільно')})\n\n"
                f"💡 Рекомендація: {rain_tip}."
            )
            w_k = {
                "inline_keyboard": [
                    [{"text": "⬅️ До клімату", "callback_data": "hub:climate"}, {"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
                ]
            }
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": w_msg,
                "parse_mode": "HTML",
                "reply_markup": w_k,
            })

        elif data == "hub:audio":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": sp_t,
                "parse_mode": "HTML",
                "reply_markup": sp_k,
            })

        elif data == "hub:routine":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            routine_t = (
                "⏱ <b>ФОКУС POMODORO ТА РЕЖИМ СНУ</b>\n"
                "━━━━━━━━━━━━━━━━━━\n"
                "Оберіть дію або налаштуйте тихі години:"
            )
            routine_k = {
                "inline_keyboard": [
                    [{"text": "⏱ Сесія Pomodoro (Фокус)", "callback_data": "hub:pomo_menu"}],
                    [{"text": "🌙 Режим DND (Не турбувати)", "callback_data": "hub:dnd_menu"}],
                    [{"text": "⏳ Таймер сну (Вимкнення світла)", "callback_data": "menu:sleep"}],
                    [{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
                ]
            }
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": routine_t,
                "parse_mode": "HTML",
                "reply_markup": routine_k,
            })

        elif data == "hub:pomo_menu":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "⏱ <b>ФОКУС ТА НАВЧАННЯ (POMODORO)</b>\nОберіть тривалість сесії концентрації:",
                "parse_mode": "HTML",
                "reply_markup": self._pomodoro_inline_kb(),
            })

        elif data == "hub:dnd_menu":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🌙 <b>РЕЖИМ DND (ТИХА НІЧ)</b>\nПовна тиша та спокійне мінімальне світло.",
                "parse_mode": "HTML",
                "reply_markup": self._dnd_inline_kb(),
            })

        elif data == "hub:screen":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🖼 <b>ГАЛЕРЕЯ АНІМАЦІЙ ТА ЗАСТАВОК:</b>\nОберіть елемент для круглого дисплея:",
                "parse_mode": "HTML",
                "reply_markup": self._gallery_inline_kb(),
            })

        elif data == "hub:refresh":
            hub_t, hub_k = self._hub_main_view(chat_id, is_admin, is_girl)
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Статус оновлено!"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": hub_k,
            })

        elif data == "hub:guide":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": GUIDE_TEXT,
                "parse_mode": "HTML",
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}]]},
            })

        elif data == "hub:admin":
            if not is_admin:
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "⛔ Лише для адміністратора"})
                return
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            role_st = "Дівчина (Тест)" if self.admin_preview_girl else "Адміністратор"
            admin_t = (
                "🛠 <b>АДМІНІСТРАТИВНИЙ ЦЕНТР SMARTLAMP</b>\n"
                "━━━━━━━━━━━━━━━━━━\n"
                "Керування системою, оновлення коду з GitHub та діагностика:"
            )
            admin_k = {
                "inline_keyboard": [
                    [{"text": "🩺 Діагностика лампи", "callback_data": "admin:health"}, {"text": "🔄 Оновити сервер", "callback_data": "admin:update"}],
                    [{"text": "💾 Бекап SQLite", "callback_data": "admin:backup"}, {"text": f"🎭 Перемкнути вигляд ({role_st})", "callback_data": "admin:toggle_view"}],
                    [{"text": "🎁 Скинути для Насті", "callback_data": "admin:sandbox_reset"}],
                    [{"text": "⬅️ Головне меню", "callback_data": "menu:hub"}],
                ]
            }
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": admin_t,
                "parse_mode": "HTML",
                "reply_markup": admin_k,
            })

        elif data == "admin:health":
            rep = maintenance_service.get_health_report()
            box = rep["box"]
            srv = rep["server"]["resources"]
            t_msg = (
                f"🩺 <b>ДІАГНОСТИКА СИСТЕМИ SMARTLAMP</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"📡 Лампа: <b>{'🟢 Онлайн' if box['is_online'] else '🔴 Офлайн'}</b> (RSSI: {box['wifi_rssi_dbm']} dBm)\n"
                f"🌡 Кімната: {box['room_temperature']}°C, {box['room_humidity']}%, {box['room_pressure_mm']} мм\n"
                f"🔋 Вільна PSRAM: {box['psram_free_bytes'] // (1024*1024)} MB | Heap: {box['heap_free_bytes'] // 1024} KB\n"
                f"🖥 Сервер RAM: {srv.get('ram_rss_mb', 0):.1f} MB | Диск: {srv.get('storage_used_mb', 0):.1f} MB\n"
                f"🎵 Spotify: {'Підключено ✅' if rep['integrations']['spotify_linked'] else 'Не підключено ❌'}\n"
                f"📅 Google Календар: {'Підключено ✅' if rep['integrations']['google_calendar_linked'] else 'Не підключено ❌'}"
            )
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": t_msg,
                "parse_mode": "HTML",
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ До адмінки", "callback_data": "hub:admin"}, {"text": "⬅️ Головне меню", "callback_data": "menu:hub"}]]},
            })

        elif data == "admin:update":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Перевірка оновлень..."})
            st = server_updater.check_for_updates()
            if not st.get("update_available"):
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": f"✅ <b>Сервер вже має найновішу версію!</b>\n• Поточний коміт: <code>{st.get('current_commit')}</code>\n• Опис: <i>{st.get('current_message')}</i>",
                    "parse_mode": "HTML",
                    "reply_markup": {"inline_keyboard": [[{"text": "⬅️ До адмінки", "callback_data": "hub:admin"}]]},
                })
            else:
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": f"🚀 <b>Застосовую оновлення...</b>\nНовий коміт: <code>{st.get('remote_commit')}</code>",
                    "parse_mode": "HTML",
                })
                res = await server_updater.perform_server_update()
                if not res.get("ok"):
                    await self._api("editMessageText", {
                        "chat_id": chat_id,
                        "message_id": msg_id,
                        "text": f"❌ Помилка оновлення: {res.get('error')}",
                        "reply_markup": {"inline_keyboard": [[{"text": "⬅️ До адмінки", "callback_data": "hub:admin"}]]},
                    })

        elif data == "admin:backup":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Створюю бекап..."})
            ok = await maintenance_service.send_weekly_backup_to_admin()
            ans_t = "✅ Резервну копію успішно відправлено адміністратору!" if ok else "❌ Помилка створення бекапу."
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": ans_t,
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ До адмінки", "callback_data": "hub:admin"}]]},
            })

        elif data == "admin:toggle_view":
            self.admin_preview_girl = not self.admin_preview_girl
            ans = "🎭 Увімкнено вигляд для дівчини (Насті)" if self.admin_preview_girl else "👑 Повернуто вигляд адміністратора"
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": ans, "show_alert": True})
            hub_t, hub_k = self._hub_main_view(chat_id, not self.admin_preview_girl, True)
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": hub_k,
            })

        elif data == "admin:sandbox_reset":
            spotify_service.disconnect()
            google_calendar_service.disconnect()
            purged = db_service.purge_sandbox_test_data()
            state_service.save_auth_ids(girl_id="1294097414")
            db_service.log_event("admin", "sandbox", "reset_to_nastya", {"purged": purged, "girl_id": "1294097414"})
            rep_msg = (
                "🎁 <b>ПЕРЕДПОДАРУНКОВЕ СКИДАННЯ ВИКОНАНО!</b>\n"
                "━━━━━━━━━━━━━━━━━━\n"
                "• Тестовий Spotify та Google Calendar відключено 🔌\n"
                f"• Очищено: події календаря ({purged.get('calendar_events', 0)}), "
                f"клімат ({purged.get('climate_history', 0)}), "
                f"блекаути ({purged.get('power_outages', 0)})\n"
                "• Збережено: кастомні .mbx гіфки, звуки та налаштування AI\n"
                "• TG ID власниці закріплено за Настею: <b>1294097414</b>"
            )
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🎁 Скинуто для Насті!", "show_alert": True})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": rep_msg,
                "parse_mode": "HTML",
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ До адмінки", "callback_data": "hub:admin"}]]},
            })

        elif data == "spotify:setup_info":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            info_t = (
                "⚙️ <b>НАЛАШТУВАННЯ SPOTIFY DEVELOPER</b>\n"
                "━━━━━━━━━━━━━━━━━━\n"
                "1. Відкрийте <a href=\"https://developer.spotify.com/dashboard\">developer.spotify.com</a>\n"
                "2. Створіть додаток «SmartLamp»\n"
                "3. У Redirect URIs додайте:\n"
                f"   <code>{state_service.resolve_public_url(settings.PUBLIC_SERVER_URL)}/api/v5/spotify/callback</code>\n"
                "4. Скопіюйте <b>Client ID</b> та <b>Client Secret</b>\n"
                "5. Надішліть боту команду:\n"
                "   <code>/set_spotify &lt;CLIENT_ID&gt; &lt;CLIENT_SECRET&gt;</code>\n"
                "   або введіть їх через Web Studio!"
            )
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": info_t,
                "parse_mode": "HTML",
                "disable_web_page_preview": True,
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ Назад", "callback_data": "hub:audio"}]]},
            })

        elif data == "google:setup_info":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            info_t = (
                "⚙️ <b>ПІДКЛЮЧЕННЯ КАЛЕНДАРЯ НАСТІ</b>\n"
                "━━━━━━━━━━━━━━━━━━\n"
                "<b>Спосіб 1 (Швидкий iCal — 10 секунд без Google Cloud)</b>:\n"
                "1. Відкрийте <a href=\"https://calendar.google.com\">Google Calendar</a> на ПК або телефоні\n"
                "2. Натисніть ⚙️ Налаштування біля потрібного календаря\n"
                "3. Скопіюйте <b>«Таємна адреса у форматі iCal»</b> (.ics)\n"
                "4. Просто надішліть це посилання у цей чат або через команду:\n"
                "   <code>/set_calendar https://calendar.google.com/.../basic.ics</code>\n\n"
                "<b>Спосіб 2 (Офіційний Google OAuth)</b>:\n"
                "1. Створіть проєкт у Google Cloud Console\n"
                "2. Увімкніть Google Calendar API та налаштуйте OAuth Consent Screen\n"
                "3. Створіть OAuth Client ID (Web Application) з Redirect URI:\n"
                f"   <code>{state_service.resolve_public_url(settings.PUBLIC_SERVER_URL)}/api/v5/google/callback</code>\n"
                "4. Надішліть боту команду:\n"
                "   <code>/set_google &lt;CLIENT_ID&gt; &lt;CLIENT_SECRET&gt;</code>"
            )
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": info_t,
                "parse_mode": "HTML",
                "disable_web_page_preview": True,
                "reply_markup": {"inline_keyboard": [[{"text": "⬅️ Назад", "callback_data": "hub:routine"}]]},
            })

    async def _handle_message(self, msg: Dict[str, Any]) -> None:
        chat_id = str((msg.get("chat") or {}).get("id", ""))
        sender = (msg.get("from") or {}).get("first_name", "Telegram")
        text = (msg.get("text") or msg.get("caption") or "").strip()

        # Автоматичне розпізнавання ролей без перехоплення звичайних повідомлень
        if chat_id == "1294097414" and state_service.girl_id != "1294097414":
            state_service.save_auth_ids(girl_id="1294097414")
        if chat_id == "1334785533" and not state_service.girl_id:
            state_service.save_auth_ids(girl_id="1334785533")
        if chat_id == "1047202957" and not state_service.admin_id:
            state_service.save_auth_ids(admin_id="1047202957")

        ALLOWED_ADMINS = {state_service.admin_id, "1047202957"}
        ALLOWED_GIRLS = {state_service.girl_id, "1294097414", "1334785533"}

        # Авторизація через Deep-Link
        if text.startswith("/start"):
            args = text.split(maxsplit=1)
            token = args[1] if len(args) > 1 else ""
            if token.startswith("box_link_") or token.startswith("link_") or chat_id == "1294097414":
                state_service.save_auth_ids(girl_id=chat_id)
                db_service.log_event("user", "auth", "bind_girl_deeplink", {"chat_id": chat_id, "token": token})

        # Авторизація команд ролей
        if text.lower() == "/iam_admin":
            if not state_service.admin_id or chat_id in ALLOWED_ADMINS:
                state_service.save_auth_ids(admin_id=chat_id)
                await self._api("sendMessage", {"chat_id": chat_id, "text": "👑 Акаунт адміністратора підтверджено.", "reply_markup": ADMIN_KB})
            else:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Адміністратор вже зареєстрований."})
            return

        if text.lower() in ("/iam_nastia", "/iam_girl"):
            state_service.save_auth_ids(girl_id=chat_id)
            db_service.log_event("user", "auth", "bind_girl", {"chat_id": chat_id})
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "✅ Вітаю, Настю! Пульт керування SmartLamp активовано.",
                "reply_markup": GIRL_KB,
            })
            return

        is_admin = (chat_id in ALLOWED_ADMINS)
        is_girl = (chat_id in ALLOWED_GIRLS)

        # Адміністративне керування передподарунковим скиданням та ID Насті
        if is_admin:
            if text.startswith("/set_girl"):
                parts = text.split()
                new_id = parts[1] if len(parts) > 1 else "1294097414"
                state_service.save_auth_ids(girl_id=new_id)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"👑 TG ID власниці (Насті) встановлено: <code>{new_id}</code>",
                    "parse_mode": "HTML",
                    "reply_markup": ADMIN_KB,
                })
                return

            if text.startswith("/sandbox_reset") or "Скинути для Насті" in text:
                spotify_service.disconnect()
                google_calendar_service.disconnect()
                purged = db_service.purge_sandbox_test_data()
                state_service.save_auth_ids(girl_id="1294097414")
                db_service.log_event("admin", "sandbox", "reset_to_nastya", {"purged": purged, "girl_id": "1294097414"})
                rep_msg = (
                    "🎁 <b>ПЕРЕДПОДАРУНКОВЕ СКИДАННЯ ВИКОНАНО!</b>\n"
                    "━━━━━━━━━━━━━━━━━━\n"
                    "• Тестовий Spotify та Google Calendar відключено 🔌\n"
                    f"• Очищено: події календаря ({purged.get('calendar_events', 0)}), "
                    f"клімат ({purged.get('climate_history', 0)}), "
                    f"блекаути ({purged.get('power_outages', 0)})\n"
                    "• Збережено: кастомні .mbx гіфки, звуки та налаштування AI\n"
                    "• TG ID власниці закріплено за Настею: <b>1294097414</b>"
                )
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": rep_msg,
                    "parse_mode": "HTML",
                    "reply_markup": ADMIN_KB,
                })
                return

            if text.startswith("/server_update") or text.startswith("/update_server") or "Оновити сервер" in text:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "🔍 <b>Перевірка оновлень на GitHub...</b>",
                    "parse_mode": "HTML",
                })
                st = server_updater.check_for_updates()
                if not st.get("update_available"):
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"✅ <b>Сервер вже має найновішу версію!</b>\n• Поточний коміт: <code>{st.get('current_commit')}</code>\n• Опис: <i>{st.get('current_message')}</i>",
                        "parse_mode": "HTML",
                        "reply_markup": ADMIN_KB,
                    })
                    return
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": (
                        f"🚀 <b>Знайдено оновлення!</b> (відставання: {st.get('commits_behind')} комітів)\n"
                        f"• Новий коміт: <code>{st.get('remote_commit')}</code>\n"
                        f"• Опис: <i>{st.get('remote_message')}</i>\n\n"
                        f"⏳ Створюю бекап бази та застосовую оновлення..."
                    ),
                    "parse_mode": "HTML",
                })
                res = await server_updater.perform_server_update()
                if not res.get("ok"):
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"❌ Помилка оновлення: {res.get('error')}",
                        "reply_markup": ADMIN_KB,
                    })
                return

        if not is_admin and not is_girl:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Доступ обмежено. Приватна система SmartLamp."})
            return

        active_kb = ADMIN_KB if is_admin else GIRL_KB
        online_note = "" if state_service.is_online() else "\n🌙 (Лампа офлайн — команду збережено, спрацює при появі живлення)"

        # 1. Прийом GIF-анімацій
        if "animation" in msg or ("document" in msg and str(msg["document"].get("file_name", "")).lower().endswith(".gif")):
            anim = msg.get("animation") or msg.get("document")
            file_id = anim.get("file_id")
            if file_id:
                raw_bytes = await self._download_tg_file(file_id)
                if raw_bytes:
                    save_tag = text or f"gif_{int(time.time())}"
                    meta = await media_service.process_and_enqueue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎞 Анімацію «{meta['name']}» запущено на екрані лампи!{online_note}",
                        "reply_markup": self._media_control_inline_kb(meta["name"]),
                    })
                    return

        # 2. Прийом фотографій
        if "photo" in msg and msg["photo"]:
            best_photo = msg["photo"][-1]
            raw_bytes = await self._download_tg_file(best_photo["file_id"])
            if raw_bytes:
                save_tag = text or f"pic_{int(time.time())}"
                meta = await media_service.process_and_enqueue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"🖼 Зображення виведено на дисплей лампи!{online_note}",
                    "reply_markup": self._media_control_inline_kb(meta["name"]),
                })
                return

        # 3. Прийом голосових повідомлень (Voice)
        if "voice" in msg:
            if is_girl:
                if state_service.admin_id:
                    await self._api("sendVoice", {
                        "chat_id": state_service.admin_id,
                        "voice": msg["voice"].get("file_id"),
                        "caption": "🎙 <b>Голосове повідомлення від Насті</b>",
                        "parse_mode": "HTML",
                    })
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "🎙 Голосове повідомлення передано Ярославу в Telegram.",
                    "reply_markup": GIRL_KB,
                })
                return
            else:
                voice_id = msg["voice"].get("file_id")
                if voice_id:
                    raw_voice = await self._download_tg_file(voice_id)
                    if raw_voice:
                        await audio_service.save_voice_stream(raw_voice, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                        await self._api("sendMessage", {
                            "chat_id": chat_id,
                            "text": f"🎙 Голосове повідомлення відправлено на динамік лампи.{online_note}",
                            "reply_markup": active_kb,
                        })
                return

        if not text:
            return

        # Навігаційне меню та пульт
        if text.startswith("/start"):
            guide_res = await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": GUIDE_TEXT,
                "parse_mode": "HTML",
            })
            if guide_res and guide_res.get("ok"):
                mid = guide_res["result"]["message_id"]
                await self._pin_message(chat_id, mid)

            hub_t, hub_k = self._hub_main_view(chat_id, is_admin, is_girl)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            return

        if text in ("📌 Інструкція", "/guide", "/help"):
            guide_res = await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": GUIDE_TEXT,
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            if guide_res and guide_res.get("ok"):
                mid = guide_res["result"]["message_id"]
                await self._pin_message(chat_id, mid)
            return

        if text in ("🎛 Головний пульт", "/menu", "/hub"):
            hub_t, hub_k = self._hub_main_view(chat_id, is_admin, is_girl)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            return

        if text.startswith("/toggle_view") or text == "🎭 Перемкнути режим":
            if chat_id not in ALLOWED_ADMINS:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Лише для адміністратора."})
                return
            self.admin_preview_girl = not self.admin_preview_girl
            role_text = "Дівчина (Настя)" if self.admin_preview_girl else "Адміністратор (Ярослав)"
            active_kb = GIRL_KB if self.admin_preview_girl else ADMIN_KB
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"🎭 <b>Режим інтерфейсу перемкнуто на:</b> {role_text}",
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            hub_t, hub_k = self._hub_main_view(chat_id, not self.admin_preview_girl, True)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": hub_t,
                "parse_mode": "HTML",
                "reply_markup": hub_k,
            })
            return

        if text.startswith("/set_spotify"):
            parts = text.split()
            if len(parts) >= 3:
                cid, csec = parts[1].strip(), parts[2].strip()
                spotify_service.save_credentials(cid, csec)
                auth_u = spotify_service.get_authorize_url(settings.PUBLIC_SERVER_URL)
                reply_k = {"inline_keyboard": [[{"text": "🔗 Авторизувати Spotify", "url": auth_u}]]} if auth_u else None
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "✅ <b>Ключі Spotify успішно збережено!</b>\nНатисніть кнопку нижче, щоб прив'язати акаунт:",
                    "parse_mode": "HTML",
                    "reply_markup": reply_k,
                })
            else:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "Формат команди:\n<code>/set_spotify &lt;CLIENT_ID&gt; &lt;CLIENT_SECRET&gt;</code>",
                    "parse_mode": "HTML",
                })
            return

        if text.startswith("/set_google"):
            parts = text.split()
            if len(parts) >= 3:
                cid, csec = parts[1].strip(), parts[2].strip()
                google_calendar_service.save_credentials(cid, csec)
                auth_u = google_calendar_service.get_authorize_url(settings.PUBLIC_SERVER_URL)
                reply_k = {"inline_keyboard": [[{"text": "🔗 Авторизувати Google", "url": auth_u}]]} if auth_u else None
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "✅ <b>Ключі Google Calendar збережено!</b>\nНатисніть кнопку нижче для авторизації:",
                    "parse_mode": "HTML",
                    "reply_markup": reply_k,
                })
            else:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "Формат команди:\n<code>/set_google &lt;CLIENT_ID&gt; &lt;CLIENT_SECRET&gt;</code>",
                    "parse_mode": "HTML",
                })
            return

        if text.startswith("/set_calendar") or ("calendar.google.com/calendar/ical" in text) or (text.strip().startswith("https://") and ".ics" in text):
            raw_url = text.split(maxsplit=1)[1] if text.startswith("/set_calendar") and len(text.split(maxsplit=1)) > 1 else text
            import re
            m = re.search(r'https?://[^\s]+', raw_url)
            if m:
                target_url = m.group(0).strip()
                google_calendar_service.save_ical_url(target_url)
                events = await google_calendar_service.get_upcoming_events(days=7)
                count = len(events)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": (
                        f"✅ <b>Google Календар підключено через iCal!</b> 📅\n"
                        f"• Знайдено подій на найближчі 7 днів: <b>{count}</b>\n"
                        f"• Лампа синхронізуватиметься автоматично кожні 15 хв."
                    ),
                    "parse_mode": "HTML",
                    "reply_markup": active_kb,
                })
            else:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "❌ Будь ласка, надішліть посилання на iCal у форматі:\n<code>/set_calendar https://calendar.google.com/calendar/ical/.../basic.ics</code>",
                    "parse_mode": "HTML",
                })
            return

        # 4. Обробка текстових команд
        if text.startswith("/status") or text.startswith("/climate") or "Клімат" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": self._build_status_text(),
                "parse_mode": "HTML",
                "reply_markup": {
                    "inline_keyboard": [
                        [{"text": "📈 Графік за 24 години", "callback_data": "menu:infographic_get"}],
                    ]
                },
            })
            return

        if text.startswith("/infographic") or text == "menu:infographic_get":
            png_bytes = infographic_service.generate_status_infographic()
            await self._send_photo_bytes(chat_id, png_bytes, caption="📊 Мікроклімат за останні 24 години.")
            return

        if text.startswith("/modes") or text.startswith("/light") or "Освітлення" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "💡 <b>КЕРУВАННЯ ОСВІТЛЕННЯМ:</b>\nОберіть бажану колірну схему чи свічку:",
                "parse_mode": "HTML",
                "reply_markup": self._modes_inline_kb(),
            })
            return

        if text.startswith("/pomodoro") or text.startswith("/focus") or "Pomodoro" in text or "Фокус" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": (
                    "⏱ <b>ФОКУС ТА НАВЧАННЯ (POMODORO)</b>\n"
                    "━━━━━━━━━━━━━━━━━━\n"
                    "Оберіть тривалість сесії концентрації:\n"
                    "Лампа перейде на хорологічний циферблат відліку та тепле спокійне світло."
                ),
                "parse_mode": "HTML",
                "reply_markup": self._pomodoro_inline_kb(),
            })
            return

        if text.startswith("/dnd") or "Режим DND" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": (
                    "🌙 <b>РЕЖИМ «НЕ ТУРБУВАТИ» (DND)</b>\n"
                    "━━━━━━━━━━━━━━━━━━\n"
                    "У цьому режимі звук лампи повністю вимикається (MUTE), а світло переходить у м'який мінімум. "
                    "Усі вхідні повідомлення зберігаються без звукових сигналів."
                ),
                "parse_mode": "HTML",
                "reply_markup": self._dnd_inline_kb(),
            })
            return

        if text.startswith("/calendar") or text.startswith("/schedule") or "Календар" in text or "Розклад" in text:
            cal_t, cal_k = await self._calendar_view(settings.PUBLIC_SERVER_URL)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": cal_t,
                "parse_mode": "HTML",
                "reply_markup": cal_k,
            })
            return

        if text.startswith("/spotify") or "Spotify" in text:
            sp_t, sp_k = self._build_spotify_view(settings.PUBLIC_SERVER_URL)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": sp_t,
                "parse_mode": "HTML",
                "reply_markup": sp_k,
            })
            return

        if text.startswith("/volume") or "Гучність" in text or "Звук" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🔊 <b>КЕРУВАННЯ ГУЧНІСТЮ ТА ЗВУКОМ</b>\nОберіть рівень або вимкніть звук:",
                "parse_mode": "HTML",
                "reply_markup": self._volume_inline_kb(),
            })
            return

        if text.startswith("/gallery") or "Заставка" in text or "Екран" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🖼 <b>ГАЛЕРЕЯ АНІМАЦІЙ ТА ЗАСТАВОК:</b>\nОберіть анімацію для виведення на круглий екран лампи:",
                "parse_mode": "HTML",
                "reply_markup": self._gallery_inline_kb(),
            })
            return

        if text.startswith("/weather") or "Погода" in text:
            res = await weather_ai_service.generate_smart_briefing()
            w = res["weather"]
            rain_tip = "Варто взяти парасольку ☂️" if w.get("rain_prob_6h", 0) >= 45 else "Опадів найближчим часом не очікується"
            w_msg = (
                f"🌤 <b>ПОГОДА ТА ПРОГНОЗ ДЛЯ КИЄВА</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"• На вулиці: <b>{w.get('weather_desc', 'Ясно')}</b>, {w.get('outdoor_temp', 15.0):+.1f}°C\n"
                f"• Вітер: {w.get('wind_kmh', 5.0)} км/год, ймовірність опадів: {w.get('rain_prob_6h', 0)}%\n"
                f"• Барометр: {w.get('pressure_mm', 750.0):.0f} мм рт. ст. ({w.get('baro_verdict', 'стабільно')})\n\n"
                f"💡 Рекомендація: {rain_tip}."
            )
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": w_msg,
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            return

        if (text.startswith("/health") or "Діагностика" in text) and is_admin:
            rep = maintenance_service.get_health_report()
            box = rep["box"]
            srv = rep["server"]["resources"]
            t_msg = (
                f"🩺 <b>ДІАГНОСТИКА СИСТЕМИ SMARTLAMP</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"📡 Лампа: <b>{'🟢 Онлайн' if box['is_online'] else '🔴 Офлайн'}</b> (RSSI: {box['wifi_rssi_dbm']} dBm)\n"
                f"🌡 Кімната: {box['room_temperature']}°C, {box['room_humidity']}%, {box['room_pressure_mm']} мм\n"
                f"🔋 Вільна PSRAM: {box['psram_free_bytes'] // (1024*1024)} MB | Heap: {box['heap_free_bytes'] // 1024} KB\n"
                f"🖥 Сервер RAM: {srv.get('ram_rss_mb', 0):.1f} MB | Диск: {srv.get('storage_used_mb', 0):.1f} MB\n"
                f"🎵 Spotify: {'Підключено ✅' if rep['integrations']['spotify_linked'] else 'Не підключено ❌'}\n"
                f"📅 Google Календар: {'Підключено ✅' if rep['integrations']['google_calendar_linked'] else 'Не підключено ❌'}"
            )
            await self._api("sendMessage", {"chat_id": chat_id, "text": t_msg, "parse_mode": "HTML", "reply_markup": ADMIN_KB})
            return

        if (text.startswith("/backup") or "Бекап" in text) and is_admin:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⏳ Створюю резервну копію SQLite бази даних..."})
            ok = await maintenance_service.send_weekly_backup_to_admin()
            if not ok:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "❌ Помилка створення бекапу."})
            return

        # 5. Швидке додавання подій календаря з тексту (наприклад: "14:30 Семінар" або "Завтра 10:00 Іспит")
        cal_match = re.match(r"^(?:завтра\s+)?(\d{1,2}:\d{2})\s+(.+)$", text.strip(), re.IGNORECASE)
        if cal_match:
            t_str = cal_match.group(1)
            summary_txt = cal_match.group(2).strip()
            now_dt = nastya_routine_service.get_kyiv_now()

            # Перевіряємо чи є слово 'завтра'
            is_tomorrow = "завтра" in text.lower()
            if is_tomorrow:
                summary_txt = re.sub(r"(?i)\bзавтра\b", "", summary_txt).strip()
                target_dt = now_dt + datetime.timedelta(days=1)
            else:
                target_dt = now_dt

            h, m = map(int, t_str.split(":"))
            event_dt = datetime.datetime(target_dt.year, target_dt.month, target_dt.day, h, m, tzinfo=now_dt.tzinfo)

            await google_calendar_service.create_event(
                summary=summary_txt,
                start_dt=event_dt,
                source="tg_bot",
            )
            cal_t, cal_k = await self._calendar_view(settings.PUBLIC_SERVER_URL)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"✅ <b>Подію додано до розкладу:</b>\n• <b>{t_str} ({event_dt.strftime('%d.%m')})</b> — {summary_txt}\n\nПодія автоматично синхронізована з екраном лампи!",
                "parse_mode": "HTML",
                "reply_markup": cal_k,
            })
            return

        # 6. Звичайні текстові повідомлення
        if is_girl:
            state_service.enqueue_raw({"sender": "Настя", "msg": text[:110]})
            if state_service.admin_id:
                await self._api("sendMessage", {
                    "chat_id": state_service.admin_id,
                    "text": f"💬 <b>Повідомлення від Насті:</b>\n«{text}»",
                    "parse_mode": "HTML",
                    "reply_markup": ADMIN_KB,
                })
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"📩 Повідомлення виведено на екран лампи та передано Ярославу.{online_note}",
                "reply_markup": GIRL_KB,
            })
        else:
            state_service.enqueue_raw({"sender": sender[:28], "msg": text[:110]})
            if state_service.girl_id:
                await self._api("sendMessage", {
                    "chat_id": state_service.girl_id,
                    "text": f"💬 <b>Повідомлення від {sender}:</b>\n«{text}»",
                    "parse_mode": "HTML",
                    "reply_markup": GIRL_KB,
                })
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"📩 Повідомлення виведено на екран лампи.{online_note}",
                "reply_markup": ADMIN_KB,
            })

    async def polling_loop(self) -> None:
        if not settings.SERVER_MANAGES_BOT or len(settings.TG_BOT_TOKEN) < 15:
            logger.warning(
                "⚠️ Telegram bot polling disabled (SERVER_MANAGES_BOT=%s, token_len=%d)",
                settings.SERVER_MANAGES_BOT,
                len(settings.TG_BOT_TOKEN),
            )
            return

        logger.info("🤖 Telegram bot polling active for @%s (token: ...%s)", "SmartLampEdlerAGBot", settings.TG_BOT_TOKEN[-6:])

        try:
            if not self.webhook_cleared:
                await self._api("deleteWebhook", {"drop_pending_updates": False})
                self.webhook_cleared = True

            while True:
                offset = -1 if not self.synced else (self.last_update_id + 1)
                timeout_val = 0 if not self.synced else 25
                data = await self._api(
                    "getUpdates",
                    {"offset": offset, "limit": 8, "timeout": timeout_val},
                    timeout=32.0,
                )
                if data and data.get("ok"):
                    results = data.get("result", [])
                    if not self.synced:
                        if results:
                            self.last_update_id = results[-1]["update_id"]
                        self.synced = True
                        continue

                    for upd in results:
                        self.last_update_id = upd["update_id"]
                        if "callback_query" in upd:
                            await self._handle_callback(upd["callback_query"])
                        elif "message" in upd:
                            await self._handle_message(upd["message"])
                else:
                    await asyncio.sleep(3.0)
        except asyncio.CancelledError:
            return


bot_service = TelegramBotService()
