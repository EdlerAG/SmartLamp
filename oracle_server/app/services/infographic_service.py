import os
import time
import math
from typing import Dict, Any, List, Optional, Tuple
from datetime import datetime, timezone, timedelta
import numpy as np
from PIL import Image, ImageDraw, ImageFont

from app.core.config import settings
from app.services.database_service import db_service
from app.services.state_service import state_service, MODES_UA
from app.services.blackout_service import blackout_service


# Hallmark Color Palette (RGB Tuples)
C_BG_PRIMARY    = (8, 9, 12)       # #08090C Deep Obsidian
C_BG_SURFACE    = (18, 21, 30)     # #12151E Elevated Graphite
C_BG_CARD_INNER = (24, 29, 41)     # #181D29 Elevated Card
C_BORDER        = (34, 40, 52)     # #222834 Hairline
C_TEXT_PRIMARY  = (244, 243, 239)  # #F4F3EF Warm Alabaster
C_TEXT_MUTED    = (138, 148, 166)  # #8A94A6 Muted Ash
C_TEXT_DARK     = (90, 100, 120)   # #5A6478
C_ACCENT_WARM   = (224, 122, 95)   # #E07A5F Terracotta
C_ACCENT_COOL   = (78, 168, 222)   # #4EA8DE Calm Indigo / Sky
C_ACCENT_SUCCESS= (82, 183, 136)   # #52B788 Sage Green / Emerald
C_ACCENT_DANGER = (230, 57, 70)    # #E63946 Matte Coral
C_ACCENT_AMBER  = (245, 158, 11)   # #F59E0B Warm Amber


def _get_font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    """Підбирає системний векторний шрифт для бездоганного рендерингу на Windows або Linux VPS."""
    candidate_paths = [
        "C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf" if bold else "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    ]
    for path in candidate_paths:
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, size=size)
            except Exception:
                pass
    try:
        return ImageFont.load_default()
    except Exception:
        return None


class InfographicService:
    """
    Генератор візуальної інфографіки MagicBox v4.5 у системі дизайну Hallmark:
    - 800x500 px преміальний темний дашборд (Obsidian #08090C).
    - Картка мікроклімату кімнати + 24-годинні гладкі графіки з Min/Max маркерами.
    - Аналітика електропостачання та блекаутів за 7 днів з візуальною шкалою відключень.
    """

    __slots__ = ()

    @staticmethod
    def _format_duration_ua(sec: float) -> str:
        s = int(sec)
        h = s // 3600
        m = (s % 3600) // 60
        if h > 0:
            return f"{h} год {m} хв" if m > 0 else f"{h} год"
        return f"{m} хв"

    def generate_status_infographic(self) -> bytes:
        W, H = 800, 500
        img = Image.new("RGB", (W, H), C_BG_PRIMARY)
        draw = ImageDraw.Draw(img)

        f_title = _get_font(18, bold=True)
        f_h1    = _get_font(26, bold=True)
        f_h2    = _get_font(14, bold=True)
        f_body  = _get_font(12, bold=False)
        f_small = _get_font(11, bold=False)
        f_micro = _get_font(9, bold=False)

        # Отримуємо свіжу телеметрію
        telem = state_service.latest_telemetry
        is_online = state_service.is_online()
        bo_status = blackout_service.get_status()
        current_time = telem.get("time", "--:--")
        temp = telem.get("t", 22.0)
        hum = telem.get("h", 50.0)
        pres = telem.get("p", 750.0)
        mode_idx = telem.get("m", 0)
        mode_name = MODES_UA[mode_idx] if 0 <= mode_idx < len(MODES_UA) else "Теплий затишок"
        vol = telem.get("vol", 12)
        muted = telem.get("muted", False)
        rssi = telem.get("rssi", -60)

        # ====================================================================
        # 1. ВЕРХНІЙ ХЕДЕР ДАШБОРДУ (Y: 14..56)
        # ====================================================================
        draw.rounded_rectangle([18, 12, W - 18, 58], radius=10, fill=C_BG_SURFACE, outline=C_BORDER, width=1)

        # Статус-пігулка лампи
        status_color = C_ACCENT_SUCCESS if is_online else C_ACCENT_DANGER
        status_label = "ONLINE" if is_online else "OFFLINE"
        draw.ellipse([32, 28, 42, 38], fill=status_color)
        draw.text((48, 26), f"MAGICBOX · {status_label}", font=f_h2, fill=C_TEXT_PRIMARY)

        # Інформація справа в хедері: Час, Wi-Fi RSSI, Гучність, Режим світла
        vol_text = "🔇 MUTE" if muted or vol == 0 else f"🔊 {int(vol * 100 / 21)}%"
        header_right = f"Київ {current_time}  ·  Wi-Fi {rssi} dBm  ·  {vol_text}  ·  {mode_name}"
        draw.text((W - 32, 27), header_right, font=f_body, fill=C_TEXT_MUTED, anchor="ra")

        # ====================================================================
        # 2. ЛІВА КАРТКА: МІКРОКЛІМАТ ТА 24-ГОДИННИЙ ТРЕНД (X: 18..392, Y: 68..484)
        # ====================================================================
        draw.rounded_rectangle([18, 68, 392, 484], radius=12, fill=C_BG_SURFACE, outline=C_BORDER, width=1)

        draw.text((34, 82), "МІКРОКЛІМАТ & БАРОМЕТР", font=f_h2, fill=C_ACCENT_COOL)
        draw.text((376, 84), "24 ГОДИНИ", font=f_micro, fill=C_TEXT_DARK, anchor="ra")

        # Bento-комірки показників
        # Температура
        draw.rounded_rectangle([32, 106, 142, 168], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((42, 114), "ТЕМПЕРАТУРА", font=f_micro, fill=C_TEXT_MUTED)
        draw.text((42, 130), f"{temp:.1f}°C", font=f_title, fill=C_ACCENT_WARM)

        # Вологість
        draw.rounded_rectangle([150, 106, 260, 168], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((160, 114), "ВОЛОГІСТЬ", font=f_micro, fill=C_TEXT_MUTED)
        draw.text((160, 130), f"{hum:.0f}%", font=f_title, fill=C_ACCENT_COOL)

        # Тиск
        draw.rounded_rectangle([268, 106, 378, 168], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((278, 114), "ТИСК", font=f_micro, fill=C_TEXT_MUTED)
        draw.text((278, 130), f"{pres:.0f} мм", font=f_title, fill=C_ACCENT_AMBER)

        # 24-годинний плавний графік температури та тиску
        chart_x0, chart_y0 = 34, 186
        chart_x1, chart_y1 = 376, 460
        draw.rounded_rectangle([chart_x0, chart_y0, chart_x1, chart_y1], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)

        # Горизонтальні координатні сітки
        for gy in range(chart_y0 + 20, chart_y1 - 15, 45):
            draw.line([(chart_x0 + 10, gy), (chart_x1 - 10, gy)], fill=C_BORDER, width=1)

        # Отримуємо серію з SQLite
        series = db_service.get_climate_series(hours=24)
        if len(series) >= 2:
            t_vals = [s["t"] for s in series]
            min_t, max_t = min(t_vals), max(t_vals)
            t_span = max(max_t - min_t, 2.0)

            # Малюємо криву температури (Terracotta)
            n_pts = len(series)
            pts = []
            for idx, pt in enumerate(series):
                px = chart_x0 + 15 + int((idx / (n_pts - 1)) * (chart_x1 - chart_x0 - 30))
                norm_val = (pt["t"] - min_t) / t_span
                py = chart_y1 - 25 - int(norm_val * (chart_y1 - chart_y0 - 55))
                pts.append((px, py))

            for i in range(len(pts) - 1):
                draw.line([pts[i], pts[i + 1]], fill=C_ACCENT_WARM, width=2)

            # Точки Min/Max
            min_idx = t_vals.index(min_t)
            max_idx = t_vals.index(max_t)
            draw.ellipse([pts[min_idx][0] - 4, pts[min_idx][1] - 4, pts[min_idx][0] + 4, pts[min_idx][1] + 4], fill=C_ACCENT_COOL)
            draw.text((pts[min_idx][0], pts[min_idx][1] + 6), f"Min {min_t:.1f}°", font=f_micro, fill=C_ACCENT_COOL, anchor="ma")

            draw.ellipse([pts[max_idx][0] - 4, pts[max_idx][1] - 4, pts[max_idx][0] + 4, pts[max_idx][1] + 4], fill=C_ACCENT_WARM)
            draw.text((pts[max_idx][0], pts[max_idx][1] - 14), f"Max {max_t:.1f}°", font=f_micro, fill=C_ACCENT_WARM, anchor="ma")
        else:
            draw.text(((chart_x0 + chart_x1) // 2, (chart_y0 + chart_y1) // 2), "Збір кліматичної історії...", font=f_small, fill=C_TEXT_MUTED, anchor="mm")

        draw.text((chart_x0 + 12, chart_y0 + 8), "Графік температури (°C)", font=f_micro, fill=C_TEXT_MUTED)

        # ====================================================================
        # 3. ПРАВА КАРТКА: АНАЛІТИКА БЛЕКАУТІВ & ЖИВЛЕННЯ (X: 408..782, Y: 68..484)
        # ====================================================================
        draw.rounded_rectangle([408, 68, 782, 484], radius=12, fill=C_BG_SURFACE, outline=C_BORDER, width=1)

        draw.text((424, 82), "ЕНЕРГОМЕРЕЖА & БЛЕКАУТИ", font=f_h2, fill=C_ACCENT_AMBER)
        draw.text((766, 84), "СТАТИСТИКА 7 ДНІВ", font=f_micro, fill=C_TEXT_DARK, anchor="ra")

        # Розрахунок аналітики блекаутів за 7 днів
        now_ts = time.time()
        week_cutoff = now_ts - (7 * 86400)
        recent_outages = db_service.get_recent_outages(limit=30)
        week_outages = [o for o in recent_outages if o.get("disconnected_at", 0) >= week_cutoff]

        total_outage_sec = sum(o.get("duration_sec", 0) for o in week_outages)
        total_week_sec = 7 * 86400
        uptime_pct = max(0.0, min(100.0, ((total_week_sec - total_outage_sec) / total_week_sec) * 100.0))

        # KPI-колонки аналітики
        # Аптайм світла
        draw.rounded_rectangle([422, 106, 532, 172], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((432, 114), "ЧАС ЗІ СВІТЛОМ", font=f_micro, fill=C_TEXT_MUTED)
        uptime_col = C_ACCENT_SUCCESS if uptime_pct >= 90.0 else (C_ACCENT_AMBER if uptime_pct >= 75.0 else C_ACCENT_DANGER)
        draw.text((432, 130), f"{uptime_pct:.1f}%", font=f_title, fill=uptime_col)
        draw.text((432, 154), "за 7 днів", font=f_micro, fill=C_TEXT_DARK)

        # Кількість відключень
        draw.rounded_rectangle([540, 106, 650, 172], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((550, 114), "ВІДКЛЮЧЕНЬ", font=f_micro, fill=C_TEXT_MUTED)
        draw.text((550, 130), f"{len(week_outages)}", font=f_title, fill=C_TEXT_PRIMARY)
        draw.text((550, 154), "інцидентів", font=f_micro, fill=C_TEXT_DARK)

        # Сумарно без світла
        draw.rounded_rectangle([658, 106, 768, 172], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
        draw.text((668, 114), "БЕЗ СВІТЛА", font=f_micro, fill=C_TEXT_MUTED)
        dur_str = self._format_duration_ua(total_outage_sec) if total_outage_sec > 0 else "0 хв"
        draw.text((668, 130), dur_str, font=f_h2, fill=C_ACCENT_WARM)
        draw.text((668, 154), "сумарно", font=f_micro, fill=C_TEXT_DARK)

        # Візуальна шкала останніх відключень (Timeline)
        draw.text((424, 188), "ОСТАННІ ВІДКЛЮЧЕННЯ (ХРОНОЛОГІЯ)", font=f_micro, fill=C_TEXT_MUTED)

        box_y = 206
        if recent_outages:
            for idx, outg in enumerate(recent_outages[:5]):
                d_at = outg.get("disconnected_at", 0)
                dur_s = outg.get("duration_sec", 0)
                dt_obj = datetime.fromtimestamp(d_at, tz=timezone(timedelta(hours=3)))
                date_str = dt_obj.strftime("%d.%m %H:%M")
                dur_lbl = self._format_duration_ua(dur_s)

                draw.rounded_rectangle([422, box_y, 768, box_y + 42], radius=6, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)

                # Червоний акцентний стовпчик зліва
                draw.rectangle([422, box_y, 426, box_y + 42], fill=C_ACCENT_DANGER)

                draw.text((436, box_y + 8), f"⚡ Відключення #{idx + 1} · {date_str}", font=f_body, fill=C_TEXT_PRIMARY)
                draw.text((756, box_y + 8), f"Тривалість: {dur_lbl}", font=f_h2, fill=C_ACCENT_WARM, anchor="ra")

                # Мініатюрний прогрес-бар
                bar_len = min(int((dur_s / 14400.0) * 310), 310)
                draw.line([(436, box_y + 30), (756, box_y + 30)], fill=C_BORDER, width=3)
                if bar_len > 0:
                    draw.line([(436, box_y + 30), (436 + bar_len, box_y + 30)], fill=C_ACCENT_DANGER, width=3)

                box_y += 50
        else:
            draw.rounded_rectangle([422, box_y, 768, box_y + 80], radius=8, fill=C_BG_CARD_INNER, outline=C_BORDER, width=1)
            draw.text((595, box_y + 32), "✨ Відключень світла зафіксовано не було!", font=f_body, fill=C_ACCENT_SUCCESS, anchor="mm")
            draw.text((595, box_y + 52), "Електропостачання працює стабільно", font=f_micro, fill=C_TEXT_MUTED, anchor="mm")

        # Зберігаємо готовий PNG у RAM-диск (/dev/shm)
        buffer = io.BytesIO() if 'io' in globals() else None
        import io
        buf = io.BytesIO()
        img.save(buf, format="PNG", optimize=True)
        png_bytes = buf.getvalue()

        try:
            with open(settings.INFOGRAPHIC_PATH, "wb") as f:
                f.write(png_bytes)
        except Exception:
            pass

        return png_bytes


infographic_service = InfographicService()
