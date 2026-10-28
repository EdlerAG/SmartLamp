import os
import time
import orjson
import asyncio
from typing import Optional, Dict, Any, List

from app.core.config import settings
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service, MODES_UA
from app.services.weather_ai_service import weather_ai_service
from app.services.audio_service import audio_service
from app.services.ota_service import ota_service
from app.services.media_service import media_service
from app.services.spotify_service import spotify_service
from app.services.infographic_service import infographic_service
from app.services.nastya_routine_service import nastya_routine_service
from app.services.maintenance_service import maintenance_service
from app.services.google_calendar_service import google_calendar_service

ADMIN_KB = {
    "keyboard": [
        [{"text": "🎨 Світло & Свічка"}, {"text": "🖼 Заставка & Гіфки"}, {"text": "🎵 Spotify Плеєр"}],
        [{"text": "🔊 Гучність & Звук"}, {"text": "📊 Інфографіка & Стан"}, {"text": "📅 Розклад Насті"}],
        [{"text": "🩺 Здоров'я Лампи"}, {"text": "📦 Бекап БД"}, {"text": "⏹ Скинути екран / Стоп Гіф"}],
    ],
    "resize_keyboard": True,
}

GIRL_KB = {
    "keyboard": [
        [{"text": "🎨 Світло & Свічка"}, {"text": "🖼 Заставка & Гіфки"}, {"text": "🎵 Spotify Плеєр"}],
        [{"text": "📚 Фокус Pomodoro"}, {"text": "🌅 Ранковий Гід"}, {"text": "🔊 Гучність & Звук"}],
        [{"text": "📊 Інфографіка & Стан"}, {"text": "⏹ Скинути екран / Стоп Гіф"}],
    ],
    "resize_keyboard": True,
}


class TelegramBotService:
    __slots__ = ("last_update_id", "synced", "webhook_cleared")

    def __init__(self) -> None:
        self.last_update_id: int = 0
        self.synced: bool = False
        self.webhook_cleared: bool = False

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
                    {"text": f"{'✅ ' if not auto_b and b_idx == 2 else ''}✨ 65% Комфорт", "callback_data": "brt:2"},
                    {"text": f"{'✅ ' if not auto_b and b_idx == 3 else ''}☀️ 100% Макс", "callback_data": "brt:3"},
                ],
                [{"text": "🎨 До списку режимів...", "callback_data": "menu:modes"}],
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
                [{"text": "🎨 До списку режимів...", "callback_data": "menu:modes"}],
            ]
        }

    @staticmethod
    def _media_control_inline_kb(media_name: str) -> Dict[str, Any]:
        return {
            "inline_keyboard": [
                [
                    {"text": "♾ Крутиться постійно", "callback_data": "med_timer:0"},
                    {"text": "⏹ Зупинити показ", "callback_data": "med_stop"},
                ],
                [
                    {"text": "⏱ На 15 хв", "callback_data": "med_timer:15"},
                    {"text": "⏱ На 1 год", "callback_data": "med_timer:60"},
                    {"text": "⏱ На 3 год", "callback_data": "med_timer:180"},
                ],
                [
                    {"text": "📌 Зробити заставкою (LittleFS)", "callback_data": f"med_saver:{media_name}"},
                    {"text": "🎭 Галерея", "callback_data": "menu:gallery"},
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
        beep_lbl = f"🔘 Кліки: {'ВКЛ' if t_beep else 'ВИКЛ'}"
        return {
            "inline_keyboard": [
                [
                    {"text": mute_lbl, "callback_data": "vol:mute"},
                    {"text": f"{'✅ ' if not muted and vol <= 5 else ''}🌙 Тихо (20%)", "callback_data": "vol:4"},
                ],
                [
                    {"text": f"{'✅ ' if not muted and 10 <= vol <= 14 else ''}☕ Комфорт (55%)", "callback_data": "vol:12"},
                    {"text": f"{'✅ ' if not muted and vol >= 19 else ''}🔊 Гучно (95%)", "callback_data": "vol:20"},
                ],
                [
                    {"text": "🔉 -10%", "callback_data": "vol:down"},
                    {"text": "🔊 +10%", "callback_data": "vol:up"},
                    {"text": beep_lbl, "callback_data": "vol:beep"},
                ],
            ]
        }

    @staticmethod
    def _reactions_quick_inline_kb() -> Dict[str, Any]:
        return {
            "inline_keyboard": [
                [
                    {"text": "🔥 Вогник", "callback_data": "react:2"},
                    {"text": "⚡ Блискавка", "callback_data": "react:3"},
                    {"text": "🐱 Мем-Кіт", "callback_data": "react:4"},
                ],
                [
                    {"text": "👽 Пороблено", "callback_data": "react:5"},
                    {"text": "👑 Корона", "callback_data": "react:6"},
                ],
            ]
        }

    @staticmethod
    def _voice_config_inline_kb() -> Dict[str, Any]:
        prov = audio_service.get_configured_tts_provider()
        edge_v = audio_service.get_configured_edge_voice()
        return {
            "inline_keyboard": [
                [
                    {"text": f"{'✅ ' if prov == 'edge' and 'Ostap' in edge_v else ''}👨 Ostap (Чоловічий)", "callback_data": "voice_sel:uk-UA-OstapNeural"},
                    {"text": f"{'✅ ' if prov == 'edge' and 'Polina' in edge_v else ''}👩 Polina (Жіночий)", "callback_data": "voice_sel:uk-UA-PolinaNeural"},
                ],
                [
                    {"text": f"{'✅ ' if prov in ('elevenlabs', 'fish_audio') else ''}🎙 Клонований Голос Ярослава (API)", "callback_data": "voice_sel:clone"},
                ],
                [
                    {"text": "🗣 Озвучити тестове вітання", "callback_data": "voice:test"},
                ],
            ]
        }

    @staticmethod
    def _gallery_inline_kb() -> Dict[str, Any]:
        media_list = db_service.list_custom_media()
        visual_media = [m for m in media_list if m.get("media_type") != "sound" and not m.get("name", "").startswith("voice_")]
        if not visual_media:
            return {"inline_keyboard": [[{"text": "📭 Галерея порожня. Надішли гіфку!", "callback_data": "noop"}]]}

        rows = []
        for i in range(0, min(len(visual_media), 10), 2):
            m1 = visual_media[i]
            r = [{"text": f"🎬 {m1['name'][:14]}", "callback_data": f"play_media:{m1['name']}"}]
            if i + 1 < len(visual_media):
                m2 = visual_media[i + 1]
                r.append({"text": f"🎬 {m2['name'][:14]}", "callback_data": f"play_media:{m2['name']}"})
            rows.append(r)

        rows.append([
            {"text": "⏹ Зупинити показ", "callback_data": "med_stop"},
            {"text": "🖼 Постійна Заставка", "callback_data": "menu:saver_help"},
        ])
        return {"inline_keyboard": rows}

    @staticmethod
    def _ai_personas_inline_kb() -> Dict[str, Any]:
        p_name = state_service.ai_persona_name
        return {
            "inline_keyboard": [
                [
                    {"text": f"{'✅ ' if 'Дух' in p_name else ''}✨ Затишний Дух", "callback_data": "persona:cozy"},
                    {"text": f"{'✅ ' if 'Саркастич' in p_name else ''}😼 Саркастичний Кіт", "callback_data": "persona:cat"},
                ],
                [
                    {"text": f"{'✅ ' if 'Синоптик' in p_name else ''}🧭 Синоптик Braun", "callback_data": "persona:braun"},
                    {"text": f"{'✅ ' if 'Кібер' in p_name else ''}🤖 Кіберпанк AI", "callback_data": "persona:cyber"},
                ],
                [
                    {"text": "🔊 Озвучити зведення зараз", "callback_data": "ai:speak"},
                    {"text": "📊 Оновити інфографіку", "callback_data": "menu:infographic"},
                ],
            ]
        }

    @staticmethod
    def _build_status_text() -> str:
        d = state_service.latest_telemetry
        is_on = state_service.is_online()
        sec = state_service.seconds_since_seen()
        status_line = "🟢 В МЕРЕЖІ (ONLINE)" if is_on else f"🔴 ВІДСУТНІЙ ЗВ'ЯЗОК ({sec or '?'} с тому)"

        t_val = d.get("t", 22.0)
        h_val = d.get("h", 50.0)
        p_val = d.get("p", 750.0)
        p_diff = d.get("p_diff", 0.0)
        trend_sym = "→" if abs(p_diff) < 0.7 else ("↑" if p_diff > 0 else "↓")
        l_pct = d.get("l", 50)
        m_idx = d.get("m", 0)
        m_name = MODES_UA[m_idx % len(MODES_UA)]
        b_idx = d.get("b_idx", 2)
        auto_b = d.get("auto_b", True)
        b_pct = d.get("b_pct", 65)
        vol = d.get("vol", 12)
        muted = d.get("muted", False)
        t_beep = d.get("t_beep", True)
        rssi = d.get("rssi", -60)
        sleep_m = d.get("sleep_m", 0)
        media_m = d.get("media_m", 0)

        vol_str = "🔇 MUTE" if muted or vol == 0 else f"{int(vol * 100 / 21)}% ({vol}/21)"
        sleep_line = f"\n⏳ Таймер сну: {sleep_m} хв" if sleep_m > 0 else ""
        media_line = f"\n⏱ Таймер медіа: {media_m} хв" if media_m > 0 else ""

        return (
            f"🪄 MAGICBOX SOVEREIGN ATELIER (v4.5)\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"📡 Стан: {status_line}\n"
            f"🌡 Температура: {t_val:.1f}°C\n"
            f"💧 Вологість: {h_val:.0f}%\n"
            f"🧭 Барометр: {p_val:.0f} mmHg ({trend_sym} {p_diff:+.1f})\n"
            f"☀️ Освітленість: {l_pct}%\n"
            f"🎨 Світло: {m_name}\n"
            f"🔆 Яскравість: {'АВТО' if auto_b else f'РУЧН ({b_pct}%)'}\n"
            f"🔊 Звук: {vol_str} (Кліки: {'ВКЛ' if t_beep else 'ВИКЛ'})\n"
            f"📶 Wi-Fi Сигнал: {rssi} dBm"
            f"{sleep_line}{media_line}\n\n"
            f"🧠 Дух: {state_service.ai_persona_name}"
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
                "Для синхронізації вінілового плеєра 136×136, підсвітки AmbiLight та відображення треків — "
                "авторизуй свій акаунт через офіційний OAuth2:"
            )
            kb = {
                "inline_keyboard": [
                    [{"text": "🔗 Авторизувати Spotify", "url": auth_url if auth_url else "http://127.0.0.1:8000"}],
                    [{"text": "🔄 Перевірити підключення", "callback_data": "spotify:refresh"}],
                ]
            }
            return text, kb

        status_str = "▶️ ВІДТВОРЮЄТЬСЯ" if playing else "⏸ ПАУЗА"
        track_info = f"<b>{title}</b>\n👤 {artist}\n⏱ {time_str}" if title else "<i>Очікування відтворення...</i>"
        text = (
            f"🎵 <b>SPOTIFY VINYL DECK (v4.5)</b>\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"Статус: {status_str}\n\n"
            f"{track_info}\n\n"
            f"💡 Платівка автоматично обертається на екрані (Вікно 3) та живить стрічку SK6812 (Режим 11)!"
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
            ]
        }
        return text, kb

    def _trigger_reaction_with_custom_media(self, react_code: int, sender: str, screen_text: str) -> str:
        state_service.enqueue_raw({"react": react_code, "sender": sender[:28], "msg": screen_text[:90]})
        custom_info = ""
        media_item = db_service.get_media_by_slot(react_code)
        if media_item and media_item.get("media_type") != "sound":
            try:
                media_service.trigger_saved_media(media_item["name"], settings.PUBLIC_SERVER_URL)
                custom_info = f" + кастомна анімація '{media_item['name']}'"
            except Exception:
                pass
        sound_url = audio_service.get_reaction_sound_url(react_code, settings.PUBLIC_SERVER_URL)
        if sound_url:
            state_service.enqueue_raw({"play_url": sound_url})
        return custom_info

    async def _handle_callback(self, cb: Dict[str, Any]) -> None:
        cb_id = cb.get("id", "")
        data = cb.get("data", "")
        chat_id = str((cb.get("message") or {}).get("chat", {}).get("id", ""))
        msg_id = (cb.get("message") or {}).get("message_id")

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
                "text": f"⏳ Таймер сну «Захід сонця» встановлено на {dur} хв!" if dur > 0 else "⏳ Таймер сну скасовано.",
                "reply_markup": self._modes_inline_kb(),
            })

        # Керування гучністю та MUTE
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

        # Керування медіа-показом та таймерами
        elif data.startswith("med_timer:"):
            t_min = int(data.split(":")[1])
            media_service.set_timer(t_min)
            msg_t = f"⏱ Таймер показу: {t_min} хв!" if t_min > 0 else "♾ Анімація крутиться постійно до скасування!"
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": msg_t})

        elif data == "med_stop":
            media_service.stop_media()
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "⏹ Показ зупинено! Екран повернуто"})

        elif data.startswith("med_saver:"):
            s_name = data.split(":", 1)[1]
            try:
                media_service.trigger_saved_media(s_name, settings.PUBLIC_SERVER_URL, set_as_saver=True)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"📌 '{s_name}' записано як постійну заставку LittleFS!"})
            except Exception as e:
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Помилка: {e}"})

        # Швидкі реакції з інлайн-меню
        elif data.startswith("react:"):
            r_code = int(data.split(":")[1])
            r_names = {2: "🔥 Вогник", 3: "⚡ Блискавка", 4: "🐱 Мем-Кіт", 5: "👽 Пороблено", 6: "👑 Корона"}
            self._trigger_reaction_with_custom_media(r_code, "Telegram", r_names.get(r_code, "VIBE"))
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"{r_names.get(r_code, 'Реакція')} запущена!"})

        elif data.startswith("sp_act:"):
            action = data.split(":")[1]
            if action == "play_pause":
                cur_p = spotify_service.get_cached_state().get("playing", False)
                await spotify_service.player_action("pause" if cur_p else "play")
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

        elif data.startswith("voice_sel:"):
            choice = data.split(":", 1)[1]
            if choice == "clone":
                db_service.set_setting("tts_provider", "elevenlabs")
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Увімкнено Клонований Голос (ElevenLabs)!"})
            else:
                db_service.set_setting("tts_provider", "edge")
                db_service.set_setting("tts_edge_voice", choice)
                name_ua = "Ostap (Чоловічий)" if "Ostap" in choice else "Polina (Жіночий)"
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Голос Edge-TTS: {name_ua}"})
            await self._api("editMessageReplyMarkup", {"chat_id": chat_id, "message_id": msg_id, "reply_markup": self._voice_config_inline_kb()})

        elif data == "voice:test":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Озвучую тестове вітання на шкатулці..."})
            await audio_service.speak_text("Привіт! Шкатулка MagicBox v4.5 працює бездоганно. Звук чистий та об'ємний!", settings.PUBLIC_SERVER_URL)

        elif data.startswith("play_voice:"):
            v_name = data.split(":", 1)[1]
            ok = await audio_service.play_voice_preset(v_name, settings.PUBLIC_SERVER_URL)
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Відтворюю голос на шкатулці!" if ok else "Запис не знайдено"})

        elif data.startswith("persona:"):
            p_type = data.split(":")[1]
            presets = {
                "cozy": ("Затишний Дух MagicBox", "Ти теплий, домашній, турботливий дух дерев'яної шкатулки з вогником. Говори лагідно, затишно і з любов'ю."),
                "cat": ("Саркастичний Кіт", "Ти хитрий, розумний кіт, що живе всередині шкатулки. Твій тон іронічний, дотепний, ти любиш жартувати про сон і їжу."),
                "braun": ("Синоптик Braun", "Ти точний німецький прилад хорології у стилі Дітера Рамса. Говори лаконічно, чітко, професійно."),
                "cyber": ("Кіберпанк AI", "Ти ШІ з неонового 2077 року. Говори стильно, технічно та впевнено."),
            }
            if p_type in presets:
                p_name, p_prompt = presets[p_type]
                state_service.save_ai_persona(p_name, p_prompt)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Персона: {p_name}"})
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": f"🧠 АКТИВНА ПЕРСОНА ШІ: {p_name}\n\nПромпт: «{p_prompt}»",
                    "reply_markup": self._ai_personas_inline_kb(),
                })

        elif data == "menu:infographic":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "Генерую свіжу інфографіку..."})
            png_bytes = infographic_service.generate_status_infographic()
            await self._send_photo_bytes(
                chat_id,
                png_bytes,
                caption=self._build_status_text(),
                reply_markup=self._ai_personas_inline_kb(),
            )

        elif data == "menu:gallery":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🎭 ГАЛЕРЕЯ КАСТОМНИХ ГІФОК ТА НАЛІПОК:\nНатисни на будь-яку, щоб вивести її на круглий дисплей:",
                "reply_markup": self._gallery_inline_kb(),
            })

    async def _handle_message(self, msg: Dict[str, Any]) -> None:
        chat_id = str((msg.get("chat") or {}).get("id", ""))
        sender = (msg.get("from") or {}).get("first_name", "Telegram")
        text = (msg.get("text") or msg.get("caption") or "").strip()

        if text.lower() == "/iam_admin":
            if not state_service.admin_id or chat_id == state_service.admin_id:
                state_service.save_auth_ids(admin_id=chat_id)
                await self._api("sendMessage", {"chat_id": chat_id, "text": "👑 Акаунт Адміністратора підтверджено у v4.5!", "reply_markup": ADMIN_KB})
            else:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Адміністратор вже прив'язаний."})
            return

        if text.lower() in ("/iam_nastia", "/iam_girl"):
            if not state_service.girl_id or chat_id in (state_service.girl_id, state_service.admin_id):
                state_service.save_auth_ids(girl_id=chat_id)
                await self._api("sendMessage", {"chat_id": chat_id, "text": "🌸 Акаунт власниці шкатулки прив'язано!", "reply_markup": GIRL_KB})
            else:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Акаунт власниці вже захищено."})
            return

        is_admin = (chat_id == state_service.admin_id)
        is_girl = (chat_id == state_service.girl_id)

        if not is_admin and not is_girl:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Приватна шкатулка MagicBox v4.5."})
            return

        active_kb = ADMIN_KB if is_admin else GIRL_KB
        online_note = "" if state_service.is_online() else "\n🌙 (Шкатулка офлайн — команда в черзі й миттєво спрацює при появі світла!)"

        # 1. Прийом НАЛІПОК (Stickers)
        if "sticker" in msg:
            st = msg["sticker"]
            is_video_or_tgs = bool(st.get("is_animated") or st.get("is_video"))
            thumb_obj = st.get("thumbnail") or st.get("thumb") or {}
            file_id = thumb_obj.get("file_id") if is_video_or_tgs else st.get("file_id")
            if file_id:
                raw_bytes = await self._download_tg_file(file_id)
                if raw_bytes:
                    save_tag = text or f"stk_{st.get('file_unique_id', 'custom')[:6]}"
                    meta = await media_service.process_and_enqueue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": (
                            f"✨ Наліпку «{meta['name']}» запущено на екрані шкатулки!{online_note}\n\n"
                            f"Керування показом (крутиться постійно або за таймером):"
                        ),
                        "reply_markup": self._media_control_inline_kb(meta["name"]),
                    })
                    return

        # 2. Прийом СПРАВЖНІХ GIF-анімацій (animation)
        if "animation" in msg:
            anim = msg["animation"]
            mime = str(anim.get("mime_type") or "")
            file_id = anim.get("file_id") if ("gif" in mime or int(anim.get("file_size", 0)) < 2500000) else None
            if not file_id:
                thumb_obj = anim.get("thumbnail") or anim.get("thumb") or {}
                file_id = thumb_obj.get("file_id") or anim.get("file_id")
            if file_id:
                raw_bytes = await self._download_tg_file(file_id)
                if raw_bytes:
                    save_tag = text or f"gif_{anim.get('file_unique_id', 'anim')[:6]}"
                    meta = await media_service.process_and_enqueue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": (
                            f"🎞 Живу анімацію «{meta['name']}» ({meta['frames']} к. @ {meta['fps']} FPS, AmbiLight RGB {meta['ambi_rgb']}) запущено на екрані!{online_note}\n\n"
                            f"Обери тривалість показу або залиш постійно:"
                        ),
                        "reply_markup": self._media_control_inline_kb(meta["name"]),
                    })
                    return

        # 3. Прийом голосових повідомлень (Voice)
        if "voice" in msg:
            if is_girl:
                # Голосові повідомлення від Насті пересилаються Ярославу в Telegram, а не грають на самій лампі
                if state_service.admin_id:
                    await self._api("sendVoice", {
                        "chat_id": state_service.admin_id,
                        "voice": msg["voice"].get("file_id"),
                        "caption": "🎙 <b>Голосове повідомлення від Насті</b>",
                        "parse_mode": "HTML",
                    })
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "🎙 Твоє голосове повідомлення надіслано Ярославу в Telegram!",
                    "reply_markup": GIRL_KB,
                })
                return

            voice_id = msg["voice"].get("file_id")
            if voice_id:
                raw_voice = await self._download_tg_file(voice_id)
                if raw_voice:
                    if text.startswith("/save_voice ") or (text and not text.startswith("/")):
                        p_name = text.replace("/save_voice", "").strip() or f"v_{int(time.time())}"
                        await audio_service.save_voice_preset(p_name, raw_voice, base_url=settings.PUBLIC_SERVER_URL)
                        await self._api("sendMessage", {
                            "chat_id": chat_id,
                            "text": f"🎙 Голосову замітку «{p_name}» збережено у банк звуків шкатулки!",
                            "reply_markup": {"inline_keyboard": [[{"text": "🔊 Відтворити зараз на шкатулці", "callback_data": f"play_voice:{p_name}"}]]},
                        })
                    else:
                        await audio_service.save_voice_stream(raw_voice, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                        await self._api("sendMessage", {
                            "chat_id": chat_id,
                            "text": f"🎙 Голосове повідомлення передано в динамік шкатулки!{online_note}",
                            "reply_markup": active_kb,
                        })
                    return

        # 4. Прийом ФОТОГРАФІЙ
        if "photo" in msg and msg["photo"]:
            best_photo = msg["photo"][-1]
            raw_bytes = await self._download_tg_file(best_photo["file_id"])
            if raw_bytes:
                save_tag = text or f"pic_{best_photo.get('file_unique_id', 'img')[:6]}"
                meta = await media_service.process_and_enqueue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"🖼 Фото оброблено (240×240 RGB565 TrueColor Bayer, Ambi {meta['ambi_rgb']}) та виведено на дисплей!{online_note}",
                    "reply_markup": self._media_control_inline_kb(meta["name"]),
                })
            return

        # 5. Прийом документів (.bin прошивки, .gif або .mp3 звуки)
        if "document" in msg or "audio" in msg:
            doc = msg.get("document") or msg.get("audio") or {}
            fname = str(doc.get("file_name") or "file").lower()
            if fname.endswith(".bin"):
                if not is_admin:
                    await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Тільки адміністратор може оновлювати прошивку."})
                    return
                raw_bin = await self._download_tg_file(doc["file_id"])
                if raw_bin:
                    res = ota_service.save_binary_bytes(raw_bin, state_service.resolve_public_url(settings.PUBLIC_SERVER_URL))
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🚀 Прошивку `{fname}` ({res['size']} байт, SHA256: `{res['sha256'][:10]}...`) поставлено в чергу OTA!{online_note}",
                        "reply_markup": active_kb,
                    })
                return
            elif fname.endswith((".gif", ".webp", ".png", ".jpg", ".jpeg")):
                raw_img = await self._download_tg_file(doc["file_id"])
                if raw_img:
                    save_tag = text or fname.rsplit(".", 1)[0]
                    meta = await media_service.process_and_enqueue(raw_img, settings.PUBLIC_SERVER_URL, save_name=save_tag)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎞 Анімацію «{meta['name']}» запущено на екрані шкатулки!{online_note}",
                        "reply_markup": self._media_control_inline_kb(meta["name"]),
                    })
                return
            elif fname.endswith(".mp3") or "audio" in msg:
                raw_mp3 = await self._download_tg_file(doc["file_id"])
                if raw_mp3:
                    sound_tag = text or fname[:-4]
                    await audio_service.save_and_play_mp3_bytes(raw_mp3, name=sound_tag, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎵 Звук `{sound_tag}` збережено та відправлено на динамік шкатулки!{online_note}",
                        "reply_markup": active_kb,
                    })
                return

        if not text:
            return

        if text in ("/start", "/help", "/menu"):
            await self._api("sendMessage", {"chat_id": chat_id, "text": self._build_status_text(), "reply_markup": active_kb})
            return

        # 6. Обробка текстових кнопок нової клавіатури v4.5
        if text.startswith("/status") or text.startswith("/blackout") or "Інфографіка" in text or "Стан" in text:
            png_bytes = infographic_service.generate_status_infographic()
            await self._send_photo_bytes(
                chat_id,
                png_bytes,
                caption=self._build_status_text(),
                reply_markup=self._ai_personas_inline_kb(),
            )
            return

        if text.startswith("/volume") or "Гучність" in text or "Звук" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🔊 <b>КЕРУВАННЯ ГУЧНІСТЮ ТА ЗВУКОВИМИ РЕЖИМАМИ</b>\nОберіть комфортний рівень або вимкніть звук повністю:",
                "parse_mode": "HTML",
                "reply_markup": self._volume_inline_kb(),
            })
            return

        if text.startswith("/stop") or "Скинути екран" in text or "Стоп Гіф" in text:
            media_service.stop_media()
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "⏹ Показ медіа зупинено, екран шкатулки повернуто до хорологічного годинника та погоди!",
                "reply_markup": active_kb,
            })
            return

        if "Швидка Реакція" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🎭 <b>ШВИДКІ МЕМ-РЕАКЦІЇ:</b>\nТоркніться реакції для миттєвого показу на екрані з фірмовим процедурним звуком:",
                "parse_mode": "HTML",
                "reply_markup": self._reactions_quick_inline_kb(),
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

        if text.startswith("/voice"):
            prov = audio_service.get_configured_tts_provider()
            v_name = audio_service.get_configured_edge_voice()
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": (
                    f"🎙 <b>НАЛАШТУВАННЯ ГОЛОСУ ОЗВУЧКИ (v4.5)</b>\n"
                    f"━━━━━━━━━━━━━━━━━━\n"
                    f"Поточний провайдер: <code>{prov}</code>\n"
                    f"Активний голос: <code>{v_name}</code>\n\n"
                    f"💡 Щоб зберегти власну голосову замітку для шкатулки: надішли голосове повідомлення із текстом <code>/save_voice [назва]</code>."
                ),
                "parse_mode": "HTML",
                "reply_markup": self._voice_config_inline_kb(),
            })
            return

        if text.startswith("/modes") or "Світло" in text or "Свічка" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ:", "reply_markup": self._modes_inline_kb()})
            return

        if text.startswith("/gallery") or "Заставка" in text or "Гіфки" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🎭 ГАЛЕРЕЯ КАСТОМНИХ ГІФОК ТА НАЛІПОК:\nОбери анімацію або просто надішли будь-яку гіфку/стікер сюди:",
                "reply_markup": self._gallery_inline_kb(),
            })
            return

        if text.startswith("/weather") or text.startswith("/ai") or "Погода" in text:
            note_arg = text.split(" ", 1)[1].strip() if (text.startswith("/ai ") and " " in text) else ""
            res = await weather_ai_service.generate_smart_briefing(custom_note=note_arg)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"🌤 МЕТЕОСТАНЦІЯ & {res['persona'].upper()}\n━━━━━━━━━━━━━━━━━━\n{res['ai_forecast']}{online_note}",
                "reply_markup": self._ai_personas_inline_kb(),
            })
            return

        if text.startswith("/say "):
            phrase = text[5:].strip()
            await audio_service.speak_text(phrase, settings.PUBLIC_SERVER_URL)
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"🔊 Озвучую на шкатулці:\n«{phrase}»{online_note}", "reply_markup": active_kb})
            return

        if text.startswith("/pomodoro") or "Фокус" in text:
            st = nastya_routine_service.start_pomodoro(25)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "📚 <b>РЕЖИМ ФОКУСУ ТА НАВЧАННЯ (POMODORO)</b>\n━━━━━━━━━━━━━━━━━━\nШкатулка перейшла на спокійне бурштинове світло на 25 хвилин. Не відволікайся, продуктивного фокусу! ✨",
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            return

        if text.startswith("/morning") or "Ранковий Гід" in text or "Ранок" in text:
            brief = await nastya_routine_service.generate_morning_brief_for_nastya()
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": brief,
                "parse_mode": "HTML",
                "reply_markup": active_kb,
            })
            return

        if (text.startswith("/health") or "Здоров'я" in text) and is_admin:
            rep = maintenance_service.get_health_report()
            box = rep["box"]
            srv = rep["server"]["resources"]
            t_msg = (
                f"🩺 <b>ЗДОРОВ'Я СИСТЕМИ MAGICBOX</b>\n"
                f"━━━━━━━━━━━━━━━━━━\n"
                f"📡 Шкатулка: <b>{'🟢 ОНЛАЙН' if box['is_online'] else '🔴 ОФЛАЙН'}</b> (RSSI: {box['wifi_rssi_dbm']} dBm)\n"
                f"🌡 Кімната Насті: {box['room_temperature']}°C, {box['room_humidity']}%, {box['room_pressure_mm']} мм\n"
                f"🔋 Вільна PSRAM: {box['psram_free_bytes'] // (1024*1024)} MB | Heap: {box['heap_free_bytes'] // 1024} KB\n"
                f"🖥 Сервер RAM RSS: {srv.get('ram_rss_mb', 0):.1f} MB | Диск: {srv.get('storage_used_mb', 0):.1f} MB\n"
                f"🎵 Spotify: {'Підключено ✅' if rep['integrations']['spotify_linked'] else 'Не підключено ❌'}\n"
                f"📅 Google Календар: {'Підключено ✅' if rep['integrations']['google_calendar_linked'] else 'Не підключено ❌'}\n"
                f"🔌 Блекаутів за 7 днів: {rep['power_analytics']['recent_outages_count']} (загалом {rep['power_analytics']['total_outage_time_min']} хв)"
            )
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": t_msg,
                "parse_mode": "HTML",
                "reply_markup": ADMIN_KB,
            })
            return

        if (text.startswith("/backup") or "Бекап" in text) and is_admin:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⏳ Створюю SQLite бекап бази даних..."})
            ok = await maintenance_service.send_weekly_backup_to_admin()
            if not ok:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "❌ Помилка створення чи відправки бекапу."})
            return

        if (text.startswith("/schedule") or "Розклад Насті" in text or "Розклад" in text) and is_admin:
            if not google_calendar_service.is_linked():
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "📅 Google Календар Насті ще не підключено у Web Studio.",
                    "reply_markup": ADMIN_KB,
                })
                return

            events = await google_calendar_service.get_upcoming_events(max_results=10)
            if not events:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": "📅 У календарі Насті на найближчий час немає запланованих подій.",
                    "reply_markup": ADMIN_KB,
                })
                return

            lines = ["📅 <b>Найближчий розклад Насті:</b>\n━━━━━━━━━━━━━━━━━━"]
            for ev in events:
                d_str = f" ({ev['date']})" if ev.get('date') else ""
                lines.append(f"• <b>{ev['time']}{d_str}</b> — {ev['summary']}")

            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "\n".join(lines),
                "parse_mode": "HTML",
                "reply_markup": ADMIN_KB,
            })
            return

        # 7. Звичайні текстові повідомлення від Насті чи Ярослава
        if is_girl:
            # Повідомлення від Насті НЕ повинно запускати мем кота!
            # Воно надсилається Ярославу в Telegram і з'являється у картці Inbox на шкатулці
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
                "text": f"📨 Повідомлення доставлено на екран шкатулки та надіслано Ярославу!{online_note}",
                "reply_markup": GIRL_KB,
            })
        else:
            state_service.enqueue_raw({"sender": sender[:28], "msg": text[:110]})
            if state_service.girl_id:
                await self._api("sendMessage", {
                    "chat_id": state_service.girl_id,
                    "text": f"✨ <b>Повідомлення на шкатулці від {sender}:</b>\n«{text}»",
                    "parse_mode": "HTML",
                    "reply_markup": GIRL_KB,
                })
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"📩 Повідомлення виведено на екран шкатулки!{online_note}",
                "reply_markup": ADMIN_KB,
            })

    async def polling_loop(self) -> None:
        if not settings.SERVER_MANAGES_BOT or len(settings.TG_BOT_TOKEN) < 15:
            return

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
