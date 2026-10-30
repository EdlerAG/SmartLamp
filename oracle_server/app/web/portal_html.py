# Файл: oracle_server/app/web/portal_html.py
"""Sovereign Web Portal HTML Loader v5.0.

Завантажує оптимізований статичний шаблон portal.html, який модульно
підключає portal.css, portal.js, PWA manifest.json та Service Worker.
"""
from pathlib import Path

_STATIC_DIR = Path(__file__).resolve().parent / "static"
_PORTAL_HTML_FILE = _STATIC_DIR / "portal.html"


def _load_portal_html() -> str:
    if _PORTAL_HTML_FILE.exists():
        try:
            return _PORTAL_HTML_FILE.read_text(encoding="utf-8")
        except Exception:
            pass
    return (
        "<!DOCTYPE html><html lang=\"uk\"><head><meta charset=\"utf-8\">"
        "<title>SmartLamp Studio</title></head><body>"
        "<h1>SmartLamp Sovereign Studio</h1></body></html>"
    )


PORTAL_HTML: str = _load_portal_html()
