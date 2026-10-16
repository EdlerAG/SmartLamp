import asyncio
from typing import Optional, Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.services.state_service import state_service, MODES_UA
from app.services.ai_service import ai_service
from app.services.audio_service import audio_service
from app.services.ota_service import ota_service
from app.services.media_service import media_service

ADMIN_KB = {
    "keyboard": [
        [{"text": "🎨 Режими та Свічка"}, {"text": "🔆 Яскравість"}, {"text": "🌤 Погода & ШІ"}],
        [{"text": "🔥 Вогник"}, {"text": "⚡ Блискавка"}, {"text": "🐱 Мем-Кіт"}],
        [{"text": "👽 Пороблено"}, {"text": "👑 Корона"}, {"text": "📊 Стан / Прошивка"}],
    ],
    "resize_keyboard": True,
}

GIRL_KB = {
    "keyboard": [
        [{"text": "🔥 Вогник"}, {"text": "⚡ Блискавка"}, {"text": "🐱 Кусь Кота"}],
        [{"text": "👽 Пороблено"}, {"text": "👑 Корона"}, {"text": "🌤 Погода & ШІ"}],
        [{"text": "🎨 Режими та Свічка"}, {"text": "🔆 Яскравість"}, {"text": "📊 Стан"}],
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
        url = f"https://api.telegram.org/bot{settings.TG_BOT_TOKEN}/{method}"
        try:
            resp = await http_pool.get().post(url, json=payload, timeout=timeout)
            if resp.status_code == 200:
                return resp.json()
        except Exception:
            pass
        return None

    async def _download_tg_file(self, file_id: str) -> Optional[bytes]:
        f_info = await self._api("getFile", {"file_id": file_id})
        f_path = f_info.get("result", {}).get("file_path") if f_info else None
        if not f_path:
            return None
        dl_url = f"https://api.telegram.org/file/bot{settings.TG_BOT_TOKEN}/{f_path}"
        try:
            resp = await http_pool.get().get(dl_url, timeout=20.0)
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
                    {"text": "🕯 Сон 15 хв", "callback_data": "slp:15"},
                    {"text": "🕯 Сон 30 хв", "callback_data": "slp:30"},
                    {"text": "🕯 Сон 60 хв", "callback_data": "slp:60"},
                ],
                [{"text": "❌ Вимкнути таймер сну", "callback_data": "slp:0"}],
                [{"text": "🎨 До списку режимів...", "callback_data": "menu:modes"}],
            ]
        }

    def _build_status_text(self) -> str:
        online = state_service.is_online()
        d = state_service.latest_telemetry
        status_icon = "🟢 ОНЛАЙН" if online else "🌙 ОФЛАЙН (Блекаут / Черга активна)"
        if not d:
            return f"🪄 MagicBox Cloud Hub\nСтатус: {status_icon}\nОчікую синхронізації зі шкатулкою..."

        return (
            f"🪄 СТАН ШКАТУЛКИ ({status_icon})\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"💡 Режим: {d.get('m_name', '--')} ({d.get('b_pct', 0)}%)\n"
            f"⏳ Таймер сну: {d.get('sleep_m', 0)} хв | Кімната: {d.get('l', 0)}%\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"🌡 Температура: {d.get('t', 0):.1f} °C\n"
            f"💧 Вологість: {d.get('h', 0):.0f} %\n"
            f"🧭 Барометр: {d.get('p', 0):.0f} мм рт. ст.\n"
            f"⚙️ FW: {d.get('fw', '--')} | 📶 {d.get('rssi', 0)} dBm\n"
            f"📬 Команд у черзі: {len(state_service.command_queue)}\n"
            f"💬 Надішли текст, фото, наліпку, голосове або /say [текст]!"
        )

    async def _handle_callback(self, cb: Dict[str, Any]) -> None:
        cb_id = cb.get("id", "")
        data = cb.get("data", "")
        msg = cb.get("message", {})
        chat_id = str(msg.get("chat", {}).get("id", ""))
        msg_id = msg.get("message_id")

        if chat_id not in (state_service.admin_id, state_service.girl_id):
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "⛔ Доступ обмежено"})
            return

        if data.startswith("mode:"):
            m = int(data.split(":")[1])
            state_service.enqueue_raw({"set_mode": m})
            if state_service.latest_telemetry:
                state_service.latest_telemetry["m"] = m
                state_service.latest_telemetry["m_name"] = MODES_UA[m % len(MODES_UA)]
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Увімкнено: {MODES_UA[m % len(MODES_UA)]}"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": f"🎨 ВИБІР РЕЖИМУ СВІТЛА\nОбрано: {MODES_UA[m % len(MODES_UA)]}",
                "reply_markup": self._modes_inline_kb(),
            })
        elif data.startswith("brt:"):
            val = data.split(":")[1]
            state_service.enqueue_raw({"set_brt": val})
            if state_service.latest_telemetry:
                state_service.latest_telemetry["auto_b"] = (val == "auto")
                if val != "auto":
                    state_service.latest_telemetry["b_idx"] = int(val)
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🔆 Яскравість оновлено!"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ",
                "reply_markup": self._brt_inline_kb(),
            })
        elif data.startswith("slp:"):
            mins = int(data.split(":")[1])
            state_service.enqueue_raw({"sleep": mins})
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"🌙 Таймер: {mins} хв" if mins else "Таймер вимкнено"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": f"⏳ Таймер сну: {mins} хв" if mins else "⏳ Таймер сну вимкнено.",
                "reply_markup": self._sleep_inline_kb(),
            })
        elif data == "menu:brt":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ", "reply_markup": self._brt_inline_kb()})
        elif data == "menu:sleep":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»", "reply_markup": self._sleep_inline_kb()})
        elif data == "menu:modes":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {"chat_id": chat_id, "message_id": msg_id, "text": "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ", "reply_markup": self._modes_inline_kb()})

    async def _handle_message(self, msg: Dict[str, Any]) -> None:
        chat_id = str(msg.get("chat", {}).get("id", ""))
        sender = msg.get("from", {}).get("first_name", "Telegram")
        text = (msg.get("text") or msg.get("caption") or "").strip()

        if text.lower() == "/iam_admin":
            if not state_service.admin_id or chat_id == state_service.admin_id:
                state_service.admin_id = chat_id
                state_service.save_snapshot()
                await self._api("sendMessage", {"chat_id": chat_id, "text": "👑 Акаунт Ярослава (Admin) підтверджено на сервері Oracle!", "reply_markup": ADMIN_KB})
            else:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Адміністратор вже прив'язаний."})
            return

        if text.lower() in ("/iam_nastia", "/iam_girl"):
            if not state_service.girl_id or chat_id in (state_service.girl_id, state_service.admin_id):
                state_service.girl_id = chat_id
                state_service.save_snapshot()
                await self._api("sendMessage", {"chat_id": chat_id, "text": "🌸 Акаунт власниці шкатулки прив'язано!", "reply_markup": GIRL_KB})
            else:
                await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Акаунт власниці вже захищено."})
            return

        is_admin = (chat_id == state_service.admin_id)
        is_girl = (chat_id == state_service.girl_id)

        if not is_admin and not is_girl:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Приватна шкатулка MagicBox. Доступ лише для власників."})
            return

        active_kb = ADMIN_KB if is_admin else GIRL_KB
        online_note = "" if state_service.is_online() else "\n🌙 (Шкатулка зараз без Wi-Fi — команда в черзі й спрацює при появі світла!)"

        # 1a. Прийом НАЛІПОК (Stickers)
        if "sticker" in msg:
            st = msg["sticker"]
            thumb_obj = st.get("thumbnail") or st.get("thumb") or {}
            file_id = thumb_obj.get("file_id") if (st.get("is_animated") or st.get("is_video")) else st.get("file_id")
            if file_id:
                raw_bytes = await self._download_tg_file(file_id)
                if raw_bytes:
                    await media_service.convert_bytes_and_queue(raw_bytes, settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"✨ Наліпку відцентровано під 240×240 та відправлено на екран шкатулки!{online_note}",
                        "reply_markup": active_kb,
                    })
                    return

        # 1b. Прийом GIF-анімацій (animation)
        if "animation" in msg:
            anim = msg["animation"]
            thumb_obj = anim.get("thumbnail") or anim.get("thumb") or {}
            thumb_id = thumb_obj.get("file_id") or anim.get("file_id")
            if thumb_id:
                raw_bytes = await self._download_tg_file(thumb_id)
                if raw_bytes:
                    await media_service.convert_bytes_and_queue(raw_bytes, settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎞 GIF-анімацію адаптовано та відправлено на екран шкатулки!{online_note}",
                        "reply_markup": active_kb,
                    })
                    return

        # 1c. Прийом голосових повідомлень (voice — формат OGG Opus)
        if "voice" in msg:
            voice_id = msg["voice"].get("file_id")
            if voice_id:
                raw_voice = await self._download_tg_file(voice_id)
                if raw_voice:
                    audio_service.save_voice_ogg(raw_voice, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎙 Голосове повідомлення відправлено на динамік шкатулки!{online_note}",
                        "reply_markup": active_kb,
                    })
                    return

        # 2. Прийом ФОТОГРАФІЙ
        if "photo" in msg and msg["photo"]:
            best_photo = msg["photo"][-1]
            raw_bytes = await self._download_tg_file(best_photo["file_id"])
            if raw_bytes:
                await media_service.convert_bytes_and_queue(raw_bytes, settings.PUBLIC_SERVER_URL, save_name=text)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"🖼 Фото оброблено у формат 240×240 RGB565 та відправлено на екран шкатулки!{online_note}",
                    "reply_markup": active_kb,
                })
            return

        # 3. Прийом прошивки (.bin) або MP3-звуку в бібліотеку / на динамік
        if "document" in msg or "audio" in msg:
            doc = msg.get("document") or msg.get("audio") or {}
            fname = doc.get("file_name", "audio.mp3")
            if fname.endswith(".bin"):
                if not is_admin:
                    await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Тільки Ярослав може оновлювати прошивку."})
                    return
                raw_bin = await self._download_tg_file(doc["file_id"])
                if raw_bin:
                    res = ota_service.save_binary_bytes(raw_bin, state_service.resolve_public_url(settings.PUBLIC_SERVER_URL))
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🚀 Прошивку `{fname}` ({res['size']} байт) збережено на сервері та поставлено в чергу OTA!{online_note}",
                        "reply_markup": active_kb,
                    })
                return
            elif fname.lower().endswith(".mp3") or "audio" in msg:
                raw_mp3 = await self._download_tg_file(doc["file_id"])
                if raw_mp3:
                    sound_tag = text or fname[:-4]
                    audio_service.save_and_play_mp3_bytes(raw_mp3, name=sound_tag, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎵 Аудіо `{fname}` відправлено на динамік шкатулки!{online_note}",
                        "reply_markup": active_kb,
                    })
                return

        if not text:
            return

        if text in ("/start", "/help", "/menu"):
            await self._api("sendMessage", {"chat_id": chat_id, "text": self._build_status_text(), "reply_markup": active_kb})
            return

        if text.startswith("/status") or "Стан" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": self._build_status_text(), "reply_markup": self._modes_inline_kb()})
            return

        if text.startswith("/modes") or "Режими" in text or "Свічка" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "🎨 ВИБІР РЕЖИМУ СВІТЛА ТА СВІЧКИ:", "reply_markup": self._modes_inline_kb()})
            return

        if text.startswith("/brt") or "Яскравість" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "🔆 КЕРУВАННЯ ЯСКРАВІСТЮ:", "reply_markup": self._brt_inline_kb()})
            return

        if text.startswith("/sleep") or "Таймер" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⏳ ТАЙМЕР СНУ «ЗАХІД СОНЦЯ»:", "reply_markup": self._sleep_inline_kb()})
            return

        if text.startswith("/weather") or text.startswith("/ai") or "Погода" in text:
            res = await ai_service.generate_smart_briefing()
            await self._api(
                "sendMessage",
                {
                    "chat_id": chat_id,
                    "text": f"🌤 МЕТЕОСТАНЦІЯ & ШІ-ПРОГНОЗ\n━━━━━━━━━━━━━━━━━━\n{res['ai_forecast']}{online_note}",
                    "reply_markup": active_kb,
                },
            )
            return

        if text.startswith("/say "):
            phrase = text[5:].strip()
            await audio_service.speak_text(phrase, settings.PUBLIC_SERVER_URL)
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"🔊 Озвучую голосом на шкатулці:\n«{phrase}»{online_note}", "reply_markup": active_kb})
            return

        reactions_map = {
            "/fire": (2, "🔥 Вогник", "FIRE VIBE! TSE PIDPAL!"),
            "🔥 Вогник": (2, "🔥 Вогник", "FIRE VIBE! TSE PIDPAL!"),
            "/zap": (3, "⚡ Блискавка", "KUS-ZAP! BZZT!"),
            "⚡ Блискавка": (3, "⚡ Блискавка", "KUS-ZAP! BZZT!"),
            "/cat": (4, "🐱 Мем-Кіт", "MEOW! TYTS-KUS =^._.^="),
            "🐱 Мем-Кіт": (4, "🐱 Мем-Кіт", "MEOW! TYTS-KUS =^._.^="),
            "🐱 Кусь Кота": (4, "🐱 Мем-Кіт", "MEOW! TYTS-KUS =^._.^="),
            "/ufo": (5, "👽 Пороблено", "UFO BEAM! POROBLENO!"),
            "👽 Пороблено": (5, "👽 Пороблено", "UFO BEAM! POROBLENO!"),
            "/crown": (6, "👑 Корона", "QUEEN VIBE! KRASH!"),
            "👑 Корона": (6, "👑 Корона", "QUEEN VIBE! KRASH!"),
        }
        if text in reactions_map:
            r_code, r_title, r_screen_txt = reactions_map[text]
            cmd_data: Dict[str, Any] = {"react": r_code, "sender": sender[:24], "msg": r_screen_txt}
            sound_url = audio_service.get_reaction_sound_url(r_code, settings.PUBLIC_SERVER_URL)
            if sound_url:
                cmd_data["play_url"] = sound_url
            state_service.enqueue_raw(cmd_data)

            if is_girl and state_service.admin_id:
                await self._api("sendMessage", {"chat_id": state_service.admin_id, "text": f"{r_title} від Насті!", "reply_markup": ADMIN_KB})
            elif is_admin and state_service.girl_id:
                await self._api("sendMessage", {"chat_id": state_service.girl_id, "text": f"{r_title} від Ярослава на твоїй шкатулці!", "reply_markup": GIRL_KB})
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"{r_title} запущено на шкатулці!{online_note}", "reply_markup": active_kb})
            return

        if is_girl:
            if state_service.admin_id:
                await self._api("sendMessage", {"chat_id": state_service.admin_id, "text": f"💬 Повідомлення від Насті:\n«{text}»", "reply_markup": ADMIN_KB})
            state_service.enqueue_raw({"react": 4})
            await self._api("sendMessage", {"chat_id": chat_id, "text": "📨 Відповідь миттєво доставлена Ярославу!", "reply_markup": GIRL_KB})
        else:
            state_service.enqueue_raw({"sender": sender[:24], "msg": text[:60]})
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"📩 Виведено на екран шкатулки!{online_note}", "reply_markup": ADMIN_KB})
            if state_service.girl_id:
                await self._api("sendMessage", {"chat_id": state_service.girl_id, "text": f"✨ Нове повідомлення на шкатулці від {sender}:\n«{text}»", "reply_markup": GIRL_KB})

    async def polling_loop(self) -> None:
        if not settings.SERVER_MANAGES_BOT or len(settings.TG_BOT_TOKEN) < 15:
            return

        try:
            if not self.webhook_cleared:
                await self._api("deleteWebhook", {"drop_pending_updates": False})
                self.webhook_cleared = True

            while True:
                offset = -1 if not self.synced else (self.last_update_id + 1)
                timeout_val = 0 if not self.synced else 20
                data = await self._api(
                    "getUpdates",
                    {"offset": offset, "limit": 5, "timeout": timeout_val},
                    timeout=28.0,
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