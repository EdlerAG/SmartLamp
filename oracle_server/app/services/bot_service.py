import asyncio
from typing import Optional, Dict, Any
from app.core.config import settings
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service, MODES_UA
from app.services.weather_ai_service import weather_ai_service
from app.services.audio_service import audio_service
from app.services.ota_service import ota_service
from app.services.media_service import media_service

ADMIN_KB = {
    "keyboard": [
        [{"text": "🎨 Режими та Свічка"}, {"text": "🎭 Галерея Гіфок"}, {"text": "🌤 Погода & ШІ"}],
        [{"text": "🔥 Вогник"}, {"text": "⚡ Блискавка"}, {"text": "🐱 Мем-Кіт"}],
        [{"text": "👽 Пороблено"}, {"text": "👑 Корона"}, {"text": "📊 Стан / ШІ-Дух"}],
    ],
    "resize_keyboard": True,
}

GIRL_KB = {
    "keyboard": [
        [{"text": "🔥 Вогник"}, {"text": "⚡ Блискавка"}, {"text": "🐱 Кусь Кота"}],
        [{"text": "👽 Пороблено"}, {"text": "👑 Корона"}, {"text": "🎭 Галерея Гіфок"}],
        [{"text": "🎨 Режими та Свічка"}, {"text": "🌤 Погода & ШІ"}, {"text": "📊 Стан"}],
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
                    {"text": "🕯 Сон 15 хв", "callback_data": "slp:15"},
                    {"text": "🕯 Сон 30 хв", "callback_data": "slp:30"},
                    {"text": "🕯 Сон 60 хв", "callback_data": "slp:60"},
                ],
                [{"text": "❌ Вимкнути таймер сну", "callback_data": "slp:0"}],
                [{"text": "🎨 До списку режимів...", "callback_data": "menu:modes"}],
            ]
        }

    @staticmethod
    def _bind_slot_inline_kb(media_name: str) -> Dict[str, Any]:
        """Інлайн-меню для миттєвого закріплення надісланої гіфки/наліпки за кнопкою реакції."""
        return {
            "inline_keyboard": [
                [
                    {"text": "📌 На 🔥 Вогник", "callback_data": f"bind:{media_name}:2"},
                    {"text": "📌 На ⚡ Блискавку", "callback_data": f"bind:{media_name}:3"},
                ],
                [
                    {"text": "📌 На 🐱 Кота", "callback_data": f"bind:{media_name}:4"},
                    {"text": "📌 На 👽 НЛО", "callback_data": f"bind:{media_name}:5"},
                    {"text": "📌 На 👑 Корону", "callback_data": f"bind:{media_name}:6"},
                ],
                [{"text": "🎭 Відкрити галерею гіфок", "callback_data": "menu:gallery"}],
            ]
        }

    @staticmethod
    def _gallery_inline_kb() -> Dict[str, Any]:
        items = [m for m in db_service.list_custom_media() if m["media_type"] in ("gif", "sticker")][:12]
        slot_icons = {2: "🔥", 3: "⚡", 4: "🐱", 5: "👽", 6: "👑"}
        rows = []
        for i in range(0, len(items), 2):
            row = []
            for m in items[i:i + 2]:
                sl_ic = slot_icons.get(m["reaction_slot"], "🎞" if m["frames"] > 1 else "🖼")
                lbl = f"{sl_ic} {m['name']} ({m['frames']}к)"
                row.append({"text": lbl, "callback_data": f"play_mbx:{m['name']}"})
            rows.append(row)
        rows.append([{"text": "🎨 До режимів світла", "callback_data": "menu:modes"}])
        return {"inline_keyboard": rows}

    @staticmethod
    def _ai_personas_inline_kb() -> Dict[str, Any]:
        return {
            "inline_keyboard": [
                [
                    {"text": "🧚 Затишний Дух", "callback_data": "persona:spirit"},
                    {"text": "🐱 Кібер-Котик", "callback_data": "persona:cat"},
                ],
                [
                    {"text": "💘 Романтик", "callback_data": "persona:romantic"},
                    {"text": "🎩 Дворецький Джарвіс", "callback_data": "persona:jarvis"},
                ],
                [{"text": "🔊 Озвучити свіжий ШІ-брифінг", "callback_data": "ai:speak"}],
            ]
        }

    def _build_status_text(self) -> str:
        online = state_service.is_online()
        d = state_service.latest_telemetry
        ram_mb = memory_guard.get_rss_mb()
        status_icon = "🟢 ОНЛАЙН" if online else "🌙 ОФЛАЙН (Блекаут / Черга активна)"

        qc = state_service.get_quantcore_summary()
        qc_line = ""
        if qc and qc.get("online"):
            qc_line = f"\n📈 QuantCore V23: ${qc['total_equity']:,.2f} ({qc['unrealized_pnl']:+.2f}$) | {qc['top_leader']}"

        if not d:
            return (
                f"🪄 MagicBox Sovereign Cloud v5.0\n"
                f"Статус: {status_icon} | RAM сервера: {ram_mb:.1f} MB{qc_line}\n"
                f"Очікую підключення шкатулки..."
            )

        return (
            f"🪄 СТАН ШКАТУЛКИ ({status_icon})\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"💡 Режим: {d.get('m_name', '--')} ({d.get('b_pct', 0)}%)\n"
            f"⏳ Таймер сну: {d.get('sleep_m', 0)} хв | Світло в кімнаті: {d.get('l', 0)}%\n"
            f"━━━━━━━━━━━━━━━━━━\n"
            f"🌡 Температура: {d.get('t', 0):.1f} °C\n"
            f"💧 Вологість: {d.get('h', 0):.0f} %\n"
            f"🧭 Барометр: {d.get('p', 0):.0f} мм ({d.get('p_diff', 0):+.1f})\n"
            f"🧠 Дух ШІ: {state_service.ai_persona_name}\n"
            f"⚙️ FW: {d.get('fw', '--')} | 📶 {d.get('rssi', 0)} dBm | Server RAM: {ram_mb:.1f} MB"
            f"{qc_line}\n"
            f"💬 Надішли будь-який GIF, наліпку, фото, голосове або /persona [опис]!"
        )

    def _trigger_reaction_with_custom_media(self, r_code: int, sender: str, default_txt: str) -> str:
        """Запускає реакцію разом із прив'язаною кастомною гіфкою (.mbx) та кастомним звуком."""
        cmd_data: Dict[str, Any] = {"react": r_code, "sender": sender[:28], "msg": default_txt}
        host = state_service.resolve_public_url(settings.PUBLIC_SERVER_URL)

        # Перевіряємо, чи прив'язана кастомна гіфка/наліпка до цього слота реакції (2..6)
        custom_m = db_service.get_media_for_reaction_slot(r_code)
        custom_note = ""
        if custom_m and custom_m.get("file_path"):
            name = custom_m["name"]
            crc = custom_m["crc32_hex"]
            cmd_data.update({
                "media_name": name,
                "media_url": f"{host}/api/v5/media/{name}.mbx?c={crc}",
                "img_url": f"{host}/api/v1/media/frame.rgb565?v={crc}",
                "media_crc": crc,
                "media_frames": custom_m["frames"],
                "media_fps": custom_m["fps"],
                "ambi_rgb": [custom_m["ambi_r"], custom_m["ambi_g"], custom_m["ambi_b"]],
            })
            custom_note = f" + кастомна анімація «{name}» ({custom_m['frames']} к.)"

        sound_url = audio_service.get_reaction_sound_url(r_code, host)
        if sound_url:
            cmd_data["play_url"] = sound_url

        state_service.enqueue_raw(cmd_data)
        return custom_note

    async def _handle_callback(self, cb: Dict[str, Any]) -> None:
        cb_id = cb.get("id", "")
        data = cb.get("data", "")
        msg = cb.get("message", {})
        chat_id = str((msg.get("chat") or {}).get("id", ""))
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

        elif data.startswith("bind:"):
            _, m_name, slot_s = data.split(":")
            slot = int(slot_s)
            slot_names = {2: "🔥 Вогник", 3: "⚡ Блискавка", 4: "🐱 Мем-Кіт", 5: "👽 Пороблено", 6: "👑 Корона"}
            db_service.bind_media_to_reaction_slot(m_name, slot)
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Закріплено за {slot_names.get(slot)}!"})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": f"✅ Кастомну анімацію «{m_name}» тепер прив'язано до реакції {slot_names.get(slot)}!",
                "reply_markup": self._gallery_inline_kb(),
            })

        elif data.startswith("play_mbx:"):
            m_name = data.split(":", 1)[1]
            try:
                res = media_service.trigger_saved_media(m_name, settings.PUBLIC_SERVER_URL)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"▶️ Запущено: {m_name} ({res['frames']} к.)"})
            except Exception:
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "❌ Файл не знайдено"})

        elif data.startswith("persona:"):
            p_key = data.split(":")[1]
            presets = {
                "spirit": (
                    "Затишний Дух MagicBox",
                    "Ти — теплий, дотепний дух-охоронець розумної шкатулки MagicBox, яку створено для затишку. Пиши українською мовою, лаконічно (2-3 речення), ніжно і з легким гумором.",
                ),
                "cat": (
                    "Кібер-Котик Мурчик",
                    "Ти — харизматичний пухнастий кібер-кіт, який живе всередині круглої лампи MagicBox. Додавай котячі метафори (мур, кусь, лапки), коментуй погоду та температуру в кімнаті дотепно українською мовою (2-3 речення).",
                ),
                "romantic": (
                    "Романтичний Охоронець",
                    "Ти — ніжний і турботливий голос шкатулки. Пиши дуже тепло, компліментарно, створюючи атмосферу любові, казки та затишку українською мовою (2-3 речення).",
                ),
                "jarvis": (
                    "Дворецький J.A.R.V.I.S.",
                    "Ти — високотехнологічний британський ШІ-дворецький Джарвіс. Доповідай показники барометра, температури та погоди з тонкою аристократичною іронією українською мовою (2-3 речення).",
                ),
            }
            if p_key in presets:
                p_name, p_prompt = presets[p_key]
                state_service.save_ai_persona(p_name, p_prompt)
                await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": f"Обрано: {p_name}"})
                await self._api("editMessageText", {
                    "chat_id": chat_id,
                    "message_id": msg_id,
                    "text": f"🧠 АКТИВНА ПЕРСОНА ШІ: {p_name}\n\nПромпт: «{p_prompt}»\n\n💡 Можна також задати власний характер командою:\n/persona [твій опис характеру]",
                    "reply_markup": self._ai_personas_inline_kb(),
                })

        elif data == "ai:speak":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id, "text": "🔊 Генерую та озвучую..."})
            res = await weather_ai_service.generate_smart_briefing(speak=True)
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": f"🔊 Озвучено ({res['persona']}):\n\n{res['ai_forecast']}",
            })

        elif data == "menu:gallery":
            await self._api("answerCallbackQuery", {"callback_query_id": cb_id})
            await self._api("editMessageText", {
                "chat_id": chat_id,
                "message_id": msg_id,
                "text": "🎭 ГАЛЕРЕЯ КАСТОМНИХ ГІФОК ТА НАЛІПОК:\nНатисни на будь-яку, щоб миттєво вивести її на круглий екран з AmbiLight-підсвіткою:",
                "reply_markup": self._gallery_inline_kb(),
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
        chat_id = str((msg.get("chat") or {}).get("id", ""))
        sender = (msg.get("from") or {}).get("first_name", "Telegram")
        text = (msg.get("text") or msg.get("caption") or "").strip()

        if text.lower() == "/iam_admin":
            if not state_service.admin_id or chat_id == state_service.admin_id:
                state_service.save_auth_ids(admin_id=chat_id)
                await self._api("sendMessage", {"chat_id": chat_id, "text": "👑 Акаунт Адміністратора підтверджено у v5.0!", "reply_markup": ADMIN_KB})
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
            await self._api("sendMessage", {"chat_id": chat_id, "text": "⛔ Приватна шкатулка MagicBox v5.0."})
            return

        active_kb = ADMIN_KB if is_admin else GIRL_KB
        online_note = "" if state_service.is_online() else "\n🌙 (Шкатулка офлайн — команда в черзі й миттєво спрацює при появі світла!)"

        # 1. Прийом НАЛІПОК (Stickers: WebP / Animated)
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
                            f"✨ Наліпку «{meta['name']}» ({meta['frames']} к., AmbiLight RGB {meta['ambi_rgb']}) "
                            f"відправлено на круглий дисплей!{online_note}\n\n"
                            f"Бажаєш закріпити її за однією з 5 кнопок реакцій?"
                        ),
                        "reply_markup": self._bind_slot_inline_kb(meta["name"]),
                    })
                    return

        # 2. Прийом СПРАВЖНІХ GIF-анімацій (animation)
        if "animation" in msg:
            anim = msg["animation"]
            mime = str(anim.get("mime_type") or "")
            # Якщо це справжній GIF або невеликий файл — беремо оригінал, інакше прев'ю
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
                            f"🎞 Живу анімацію «{meta['name']}» ({meta['frames']} кадрів @ {meta['fps']} FPS, "
                            f"AmbiLight {meta['ambi_rgb']}) запущено на екрані!{online_note}\n\n"
                            f"Можеш закріпити цю гіфку на будь-яку реакцію:"
                        ),
                        "reply_markup": self._bind_slot_inline_kb(meta["name"]),
                    })
                    return

        # 3. Прийом голосових повідомлень
        if "voice" in msg:
            voice_id = msg["voice"].get("file_id")
            if voice_id:
                raw_voice = await self._download_tg_file(voice_id)
                if raw_voice:
                    await audio_service.save_voice_stream(raw_voice, sender=sender, base_url=settings.PUBLIC_SERVER_URL)
                    await self._api("sendMessage", {
                        "chat_id": chat_id,
                        "text": f"🎙 Голосове повідомлення передано в RAM-буфер динаміка!{online_note}",
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
                    "text": f"🖼 Фото оброблено у формат 240×240 RGB565 (AmbiLight {meta['ambi_rgb']}) та виведено на дисплей!{online_note}",
                    "reply_markup": self._bind_slot_inline_kb(meta["name"]),
                })
            return

        # 5. Прийом документів (.bin прошивки, .gif без стиснення або .mp3 звуки)
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
                        "text": f"🎞 Кастомну анімацію `{meta['name']}` ({meta['frames']} кадрів @ {meta['fps']} FPS) збережено та запущено!{online_note}",
                        "reply_markup": self._bind_slot_inline_kb(meta["name"]),
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

        if text.startswith("/status") or "Стан" in text:
            await self._api("sendMessage", {"chat_id": chat_id, "text": self._build_status_text(), "reply_markup": self._ai_personas_inline_kb()})
            return

        if text.startswith("/gallery") or "Галерея" in text or "Гіфок" in text:
            await self._api("sendMessage", {
                "chat_id": chat_id,
                "text": "🎭 ГАЛЕРЕЯ КАСТОМНИХ ГІФОК ТА НАЛІПОК:\nОбери анімацію для запуску або просто надішли нову гіфку/стікер у чат:",
                "reply_markup": self._gallery_inline_kb(),
            })
            return

        if text.startswith("/persona"):
            custom_p = text[8:].strip()
            if custom_p:
                state_service.save_ai_persona("Кастомний Дух", custom_p)
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"🧠 Новий характер ШІ збережено!\nПромпт: «{custom_p}»",
                    "reply_markup": active_kb,
                })
            else:
                await self._api("sendMessage", {
                    "chat_id": chat_id,
                    "text": f"🧠 Поточна персона: {state_service.ai_persona_name}\nОбери готову або напиши `/persona [твій промпт]`:",
                    "reply_markup": self._ai_personas_inline_kb(),
                })
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

        reactions_map = {
            "/fire": (2, "🔥 Вогник", "FIRE VIBE! ЦЕ ПІДПАЛ!"),
            "🔥 Вогник": (2, "🔥 Вогник", "FIRE VIBE! ЦЕ ПІДПАЛ!"),
            "/zap": (3, "⚡ Блискавка", "КУСЬ-ЗАП! BZZT!"),
            "⚡ Блискавка": (3, "⚡ Блискавка", "КУСЬ-ЗАП! BZZT!"),
            "/cat": (4, "🐱 Мем-Кіт", "НЯВ! ТИЦЬ-КУСЬ =^._.^="),
            "🐱 Мем-Кіт": (4, "🐱 Мем-Кіт", "НЯВ! ТИЦЬ-КУСЬ =^._.^="),
            "🐱 Кусь Кота": (4, "🐱 Мем-Кіт", "НЯВ! ТИЦЬ-КУСЬ =^._.^="),
            "/ufo": (5, "👽 Пороблено", "UFO BEAM! ПОРОБЛЕНО!"),
            "👽 Пороблено": (5, "👽 Пороблено", "UFO BEAM! ПОРОБЛЕНО!"),
            "/crown": (6, "👑 Корона", "QUEEN VIBE! КРАШ!"),
            "👑 Корона": (6, "👑 Корона", "QUEEN VIBE! КРАШ!"),
        }
        if text in reactions_map:
            r_code, r_title, r_screen_txt = reactions_map[text]
            custom_info = self._trigger_reaction_with_custom_media(r_code, sender, r_screen_txt)

            if is_girl and state_service.admin_id:
                await self._api("sendMessage", {"chat_id": state_service.admin_id, "text": f"{r_title} від Насті!", "reply_markup": ADMIN_KB})
            elif is_admin and state_service.girl_id:
                await self._api("sendMessage", {"chat_id": state_service.girl_id, "text": f"{r_title} від Ярослава на твоїй шкатулці!", "reply_markup": GIRL_KB})
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"{r_title}{custom_info} запущено на шкатулці!{online_note}", "reply_markup": active_kb})
            return

        if is_girl:
            if state_service.admin_id:
                await self._api("sendMessage", {"chat_id": state_service.admin_id, "text": f"💬 Повідомлення від Насті:\n«{text}»", "reply_markup": ADMIN_KB})
            self._trigger_reaction_with_custom_media(4, sender, text[:90])
            await self._api("sendMessage", {"chat_id": chat_id, "text": "📨 Відповідь миттєво доставлена!", "reply_markup": GIRL_KB})
        else:
            state_service.enqueue_raw({"sender": sender[:28], "msg": text[:110]})
            await self._api("sendMessage", {"chat_id": chat_id, "text": f"📩 Виведено рідною кирилицею на екран шкатулки!{online_note}", "reply_markup": ADMIN_KB})
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