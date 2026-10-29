import io
import os
import time
import base64
import urllib.parse
from typing import Dict, Any, Optional, Tuple, List
import numpy as np
from PIL import Image, ImageOps

from app.core.config import settings
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service


BAYER_4X4 = np.array([
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5]
], dtype=np.float32)


class SpotifyService:
    """
    Demand-Driven Zero-Idle Spotify Service (v4.0):
    - OAuth2 Authorization Code Flow (токен зберігається у SQLite WAL).
    - Zero-Idle Polling: 0.00% CPU та 0 HTTP-запитів доки на лампі не відкритий Хаб або AmbiLight.
    - Автоматична генерація 136x136 круглої вінілової платівки з обкладинкою та Bayer 4x4 Dithering.
    """

    __slots__ = (
        "_access_token",
        "_token_expires_at",
        "_last_poll_time",
        "_last_track_id",
        "_cached_state",
        "_last_ambi_rgb",
    )

    SCOPES = "user-read-currently-playing user-read-playback-state user-modify-playback-state"

    @property
    def client_id(self) -> str:
        return db_service.get_setting("spotify_client_id", settings.SPOTIFY_CLIENT_ID or "").strip()

    @property
    def client_secret(self) -> str:
        return db_service.get_setting("spotify_client_secret", settings.SPOTIFY_CLIENT_SECRET or "").strip()

    def save_credentials(self, client_id: str, client_secret: str) -> None:
        """Зберігає ключі Spotify OAuth у SQLite WAL для динамічного налаштування без перезапуску."""
        db_service.set_setting("spotify_client_id", client_id.strip())
        db_service.set_setting("spotify_client_secret", client_secret.strip())
        db_service.log_event("system", "spotify", "save_credentials", {"client_id": client_id[:6]})

    def __init__(self) -> None:
        self._access_token: Optional[str] = None
        self._token_expires_at: float = 0.0
        self._last_poll_time: float = 0.0
        self._last_track_id: str = ""
        self._cached_state: Dict[str, Any] = {
            "playing": False,
            "title": "",
            "artist": "",
            "track_id": "",
            "progress_ms": 0,
            "duration_ms": 0,
            "ambi_rgb": [56, 189, 248],
        }
        self._last_ambi_rgb: List[int] = [56, 189, 248]

    def _get_redirect_uri(self, base_url: str = "") -> str:
        if settings.SPOTIFY_REDIRECT_URI:
            return settings.SPOTIFY_REDIRECT_URI
        host = state_service.resolve_public_url(base_url)
        return f"{host}/api/v5/spotify/callback"

    def get_authorize_url(self, base_url: str = "") -> str:
        cid = self.client_id
        if not cid:
            return ""
        params = {
            "client_id": cid,
            "response_type": "code",
            "redirect_uri": self._get_redirect_uri(base_url),
            "scope": self.SCOPES,
            "show_dialog": "true",
        }
        return f"https://accounts.spotify.com/authorize?{urllib.parse.urlencode(params)}"

    def is_linked(self) -> bool:
        refresh_tok = db_service.get_setting("spotify_refresh_token", "")
        return bool(refresh_tok and self.client_id and self.client_secret)

    def disconnect(self) -> None:
        """Повністю відв'язує Spotify-акаунт та очищує кеш."""
        db_service.set_setting("spotify_refresh_token", "")
        self._access_token = None
        self._token_expires_at = 0.0
        self._last_track_id = ""
        self._cached_state = {
            "playing": False,
            "title": "",
            "artist": "",
            "track_id": "",
            "progress_ms": 0,
            "duration_ms": 0,
            "ambi_rgb": [56, 189, 248],
        }
        if os.path.exists(settings.SPOTIFY_VINYL_PATH):
            try:
                os.remove(settings.SPOTIFY_VINYL_PATH)
            except OSError:
                pass
        db_service.log_event("system", "spotify", "disconnect", {"status": "unlinked"})

    async def handle_callback(self, code: str, base_url: str = "") -> bool:
        cid, csec = self.client_id, self.client_secret
        if not cid or not csec:
            return False

        redirect_uri = self._get_redirect_uri(base_url)
        auth_header = base64.b64encode(
            f"{cid}:{csec}".encode()
        ).decode()

        data = {
            "grant_type": "authorization_code",
            "code": code,
            "redirect_uri": redirect_uri,
        }
        headers = {
            "Authorization": f"Basic {auth_header}",
            "Content-Type": "application/x-www-form-urlencoded",
        }

        try:
            resp = await http_pool.get().post(
                "https://accounts.spotify.com/api/token",
                data=data,
                headers=headers,
                timeout=12.0,
            )
            if resp.status_code == 200:
                body = resp.json()
                self._access_token = body.get("access_token")
                expires_in = body.get("expires_in", 3600)
                self._token_expires_at = time.time() + float(expires_in) - 60.0

                refresh_tok = body.get("refresh_token")
                if refresh_tok:
                    db_service.set_setting("spotify_refresh_token", refresh_tok)
                await self.fetch_current_playback(force=True)
                return True
        except Exception:
            pass
        return False

    async def _ensure_access_token(self) -> Optional[str]:
        now = time.time()
        if self._access_token and now < self._token_expires_at:
            return self._access_token

        refresh_tok = db_service.get_setting("spotify_refresh_token", "")
        cid, csec = self.client_id, self.client_secret
        if not refresh_tok or not cid or not csec:
            return None

        auth_header = base64.b64encode(
            f"{cid}:{csec}".encode()
        ).decode()

        data = {
            "grant_type": "refresh_token",
            "refresh_token": refresh_tok,
        }
        headers = {
            "Authorization": f"Basic {auth_header}",
            "Content-Type": "application/x-www-form-urlencoded",
        }

        try:
            resp = await http_pool.get().post(
                "https://accounts.spotify.com/api/token",
                data=data,
                headers=headers,
                timeout=10.0,
            )
            if resp.status_code == 200:
                body = resp.json()
                self._access_token = body.get("access_token")
                expires_in = body.get("expires_in", 3600)
                self._token_expires_at = now + float(expires_in) - 60.0
                new_refresh = body.get("refresh_token")
                if new_refresh:
                    db_service.set_setting("spotify_refresh_token", new_refresh)
                return self._access_token
        except Exception:
            pass
        return None

    async def player_action(self, action: str) -> bool:
        token = await self._ensure_access_token()
        if not token:
            return False

        headers = {"Authorization": f"Bearer {token}"}
        client = http_pool.get()

        endpoint_map = {
            "next": ("POST", "https://api.spotify.com/v1/me/player/next"),
            "previous": ("POST", "https://api.spotify.com/v1/me/player/previous"),
            "play": ("PUT", "https://api.spotify.com/v1/me/player/play"),
            "pause": ("PUT", "https://api.spotify.com/v1/me/player/pause"),
        }

        if action not in endpoint_map:
            return False

        method, url = endpoint_map[action]
        try:
            if method == "POST":
                resp = await client.post(url, headers=headers, timeout=5.0)
            else:
                resp = await client.put(url, headers=headers, timeout=5.0)
            return resp.status_code in (200, 204)
        except Exception:
            return False

    @staticmethod
    def _create_vinyl_art_bytes(album_img_bytes: bytes) -> Tuple[bytes, List[int]]:
        """
        Вирізає круглу вінілову платівку 136x136 px:
        - Обідок з концентричними доріжками R = 44..68.
        - Обкладинка в центрі R = 6..44.
        - Отвір шпинделя R < 6.
        - Застосовує Bayer 4x4 Ordered Dithering та повертає 36992 байти RGB565 Big-Endian.
        """
        size = 136
        center = (size - 1) / 2.0
        y, x = np.ogrid[:size, :size]
        dist = np.sqrt((x - center) ** 2 + (y - center) ** 2)

        # 1. Завантажуємо обкладинку і масштабуємо центральне коло
        canvas_arr = np.zeros((size, size, 3), dtype=np.float32)

        ambi_rgb = [56, 189, 248]
        try:
            with Image.open(io.BytesIO(album_img_bytes)) as raw_img:
                rgb_img = ImageOps.fit(raw_img.convert("RGB"), (80, 80), method=Image.Resampling.BILINEAR)
                art_arr = np.asarray(rgb_img, dtype=np.float32)

                # Вставляємо обкладинку в центр 136x136 (відступ 28 px)
                canvas_arr[28:108, 28:108] = art_arr

                # Визначаємо соковитий AmbiLight колір
                flat = art_arr.reshape(-1, 3)
                bright = np.max(flat, axis=1)
                sat = bright - np.min(flat, axis=1)
                mask = (bright > 40.0) & (sat > 20.0)
                vivid = flat[mask]
                if len(vivid) > 20:
                    mean_c = np.mean(vivid, axis=0)
                    peak = float(np.max(mean_c))
                    if peak > 0:
                        mean_c = np.clip(mean_c * (235.0 / peak), 0, 255)
                    ambi_rgb = [int(mean_c[0]), int(mean_c[1]), int(mean_c[2])]
        except Exception:
            pass

        # 2. Малюємо вініловий обідок з концентричними доріжками (44 < dist <= 68)
        vinyl_mask = (dist > 44.0) & (dist <= 68.0)
        groove_pattern = 14.0 + 8.0 * np.sin(dist * 2.8)
        canvas_arr[vinyl_mask, 0] = groove_pattern[vinyl_mask]
        canvas_arr[vinyl_mask, 1] = groove_pattern[vinyl_mask] * 1.15
        canvas_arr[vinyl_mask, 2] = groove_pattern[vinyl_mask] * 1.35

        # 3. Темний шпиндель у центрі (dist < 6.0)
        hole_mask = dist < 6.0
        canvas_arr[hole_mask] = [8.0, 9.0, 12.0]

        # 4. Обрізаємо все за межами вінілового кола (dist > 68.0)
        outer_mask = dist > 68.0
        canvas_arr[outer_mask] = [8.0, 9.0, 12.0]

        # 5. Векторизоване Bayer 4x4 Ordered Dithering перед перетворенням у RGB565
        h, w = size, size
        tile_y = (h + 3) // 4
        tile_x = (w + 3) // 4
        bayer_grid = np.tile(BAYER_4X4, (tile_y, tile_x))[:h, :w]

        dither_r = (bayer_grid / 16.0 - 0.5) * 8.0
        dither_g = (bayer_grid / 16.0 - 0.5) * 4.0
        dither_b = (bayer_grid / 16.0 - 0.5) * 8.0

        r_dith = np.clip(canvas_arr[:, :, 0] + dither_r, 0, 255).astype(np.uint16)
        g_dith = np.clip(canvas_arr[:, :, 1] + dither_g, 0, 255).astype(np.uint16)
        b_dith = np.clip(canvas_arr[:, :, 2] + dither_b, 0, 255).astype(np.uint16)

        r = (r_dith & 0xF8) << 8
        g = (g_dith & 0xFC) << 3
        b = b_dith >> 3
        rgb565 = r | g | b
        raw_bytes = rgb565.astype(">u2").tobytes()
        return raw_bytes, ambi_rgb

    async def fetch_current_playback(self, force: bool = False) -> Dict[str, Any]:
        """Опитує офіційний Spotify API про поточний трек із захистом від rate-limit."""
        if not self.is_linked():
            return self.get_cached_state()

        now = time.time()
        if not force and (now - self._last_poll_time < 5.0):
            return self.get_cached_state()
        self._last_poll_time = now

        token = await self._ensure_access_token()
        if not token:
            return self.get_cached_state()

        try:
            resp = await http_pool.get().get(
                "https://api.spotify.com/v1/me/player",
                headers={"Authorization": f"Bearer {token}"},
                timeout=4.5,
            )

            if resp.status_code == 204 or not resp.content:
                self._cached_state["playing"] = False
                return self.get_cached_state()

            if resp.status_code == 200:
                data = resp.json()
                item = data.get("item") or {}
                track_id = item.get("id") or ""
                is_playing = bool(data.get("is_playing", False))

                title = item.get("name", "")
                artists = [a.get("name", "") for a in item.get("artists", []) if a.get("name")]
                artist = ", ".join(artists) if artists else ""
                progress_ms = int(data.get("progress_ms", 0))
                duration_ms = int(item.get("duration_ms", 0))

                self._cached_state = {
                    "playing": is_playing,
                    "title": title[:60],
                    "artist": artist[:45],
                    "track_id": track_id[:32],
                    "progress_ms": progress_ms,
                    "duration_ms": duration_ms,
                    "ambi_rgb": self._last_ambi_rgb,
                }

                # Якщо змінився трек — генеруємо нове вінілове коло з обкладинкою
                if track_id and track_id != self._last_track_id:
                    self._last_track_id = track_id
                    images = (item.get("album") or {}).get("images", [])
                    art_url = images[0].get("url") if images else ""

                    if art_url:
                        art_resp = await http_pool.get().get(art_url, timeout=7.0)
                        if art_resp.status_code == 200:
                            vinyl_bytes, ambi = self._create_vinyl_art_bytes(art_resp.content)
                            self._last_ambi_rgb = ambi
                            self._cached_state["ambi_rgb"] = ambi

                            with open(settings.SPOTIFY_VINYL_PATH, "wb") as f:
                                f.write(vinyl_bytes)
                            memory_guard.trim_ram()

                return self.get_cached_state()
        except Exception as e:
            logger.warning("Spotify playback poll error: %s", e)

        return self.get_cached_state()

    async def get_sync_state(
        self,
        box_screen: int,
        box_unread: bool,
        box_led_mode: int,
    ) -> Dict[str, Any]:
        """
        Zero-Idle Умова для апаратної плати (ESP32):
        Якщо box_screen != 2 (не Хаб) АБО box_unread == True (є повідомлення),
        І box_led_mode != 11 (не Spotify AmbiLight) — 0 HTTP-запитів, відповідь за 0.00 мс!
        """
        hub_active = (box_screen == 2) and (not box_unread)
        ambi_active = (box_led_mode == 11)

        if not hub_active and not ambi_active:
            return self.get_cached_state()

        return await self.fetch_current_playback()

    def get_cached_state(self) -> Dict[str, Any]:
        st = dict(self._cached_state)
        st["linked"] = self.is_linked()
        return st


spotify_service = SpotifyService()
