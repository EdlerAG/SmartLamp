from __future__ import annotations
import io
import os
import re
import time
import base64
import asyncio
import urllib.parse
from typing import Dict, Any, Optional, Tuple, List
from PIL import Image, ImageOps, ImageFilter, ImageEnhance

from app.core.config import settings
from app.core.logger import logger
from app.core.http_client import http_pool
from app.core.memory_guard import memory_guard
from app.services.database_service import db_service
from app.services.state_service import state_service


_VINYL_LUT = None

def _get_vinyl_lut():
    global _VINYL_LUT
    if _VINYL_LUT is None:
        import numpy as np
        size = 136
        center = (size - 1) / 2.0
        y, x = np.ogrid[:size, :size]
        dist = np.sqrt((x - center) ** 2 + (y - center) ** 2).astype(np.float32)
        theta = np.arctan2(y - center, x - center).astype(np.float32)
        sheen = (9.0 * (np.cos(2.0 * theta - 0.785398) ** 4)).astype(np.float32)

        spindle_mask = (dist <= 4.0)
        ring_mask = (dist > 4.0) & (dist <= 6.0)
        vinyl_mask = (dist > 54.0) & (dist <= 67.0)
        outer_mask = (dist > 67.0)
        groove_59 = (dist >= 58.6) & (dist <= 59.4)
        groove_63 = (dist >= 62.6) & (dist <= 63.4)

        bayer_matrix = np.array([
            [ 0,  8,  2, 10],
            [12,  4, 14,  6],
            [ 3, 11,  1,  9],
            [15,  7, 13,  5]
        ], dtype=np.float32)
        tile_y = (size + 3) // 4
        tile_x = (size + 3) // 4
        bayer_grid = np.tile(bayer_matrix, (tile_y, tile_x))[:size, :size]
        dither_r = ((bayer_grid / 16.0 - 0.5) * 6.0).astype(np.float32)
        dither_g = ((bayer_grid / 16.0 - 0.5) * 3.5).astype(np.float32)
        dither_b = ((bayer_grid / 16.0 - 0.5) * 6.0).astype(np.float32)

        # Ваги для країв обкладинки 108x108 (чим ближче до кромки, тим вагоміший колір)
        cy_c, cx_c = np.ogrid[:108, :108]
        cdist = np.sqrt((cx_c - 53.5) ** 2 + (cy_c - 53.5) ** 2).astype(np.float32)
        cover_edge_weight = np.clip(1.0 + (cdist / 54.0) * 1.6, 1.0, 2.6).astype(np.float32)

        _VINYL_LUT = {
            "dist": dist,
            "theta": theta,
            "sheen": sheen,
            "spindle_mask": spindle_mask,
            "ring_mask": ring_mask,
            "vinyl_mask": vinyl_mask,
            "outer_mask": outer_mask,
            "groove_59": groove_59,
            "groove_63": groove_63,
            "dither_r": dither_r,
            "dither_g": dither_g,
            "dither_b": dither_b,
            "cover_edge_weight": cover_edge_weight,
        }
    return _VINYL_LUT


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
        "_last_ambi2_rgb",
        "_last_ambi3_rgb",
        "_last_is_mono",
        "_vinyl_cache",
        "_bpm_cache",
        "_hub_active",
        "_action_lock_until",
        "_rate_limited_until",
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
            "cover_url": "",
            "progress_ms": 0,
            "duration_ms": 0,
            "ambi_rgb": [54, 48, 44],
            "ambi2_rgb": [38, 34, 32],
            "ambi3_rgb": [78, 66, 54],
            "is_mono": False,
            "bpm": 120,
            "energy": 65,
        }
        self._last_ambi_rgb: List[int] = [54, 48, 44]
        self._last_ambi2_rgb: List[int] = [38, 34, 32]
        self._last_ambi3_rgb: List[int] = [78, 66, 54]
        self._last_is_mono: bool = False
        self._vinyl_cache: Dict[str, Tuple[bytes, List[int], List[int], bool]] = {}
        self._bpm_cache: Dict[str, int] = {}
        self._hub_active: bool = False
        self._action_lock_until: float = 0.0
        self._rate_limited_until: float = 0.0

        # Відновлення останнього треку для миттєвого Spotify Standby Deck
        try:
            saved_json = db_service.get_setting("spotify_last_state", "")
            if saved_json:
                import orjson
                saved_st = orjson.loads(saved_json)
                if isinstance(saved_st, dict) and saved_st.get("title"):
                    saved_st["playing"] = False
                    # Очищення старих дефолтів та застарілої палітри [70, 95, 130]
                    if saved_st.get("ambi_rgb") in ([70, 95, 130], [56, 189, 248]):
                        saved_st["ambi_rgb"] = [54, 48, 44]
                        saved_st["ambi2_rgb"] = [38, 34, 32]
                        saved_st["ambi3_rgb"] = [78, 66, 54]
                    self._cached_state.update(saved_st)
                    self._last_track_id = saved_st.get("track_id", "")
                    self._last_ambi_rgb = saved_st.get("ambi_rgb", [54, 48, 44])
                    self._last_ambi2_rgb = saved_st.get("ambi2_rgb", [38, 34, 32])
                    self._last_ambi3_rgb = saved_st.get("ambi3_rgb", [78, 66, 54])
                    self._last_is_mono = saved_st.get("is_mono", False)
        except Exception as rest_err:
            logger.debug("Spotify Standby state restore error: %s", rest_err)

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
        self._hub_active = False
        self._cached_state = {
            "playing": False,
            "title": "",
            "artist": "",
            "track_id": "",
            "progress_ms": 0,
            "duration_ms": 0,
            "ambi_rgb": [54, 48, 44],
            "ambi2_rgb": [38, 34, 32],
            "ambi3_rgb": [78, 66, 54],
            "is_mono": False,
            "bpm": 120,
            "energy": 65,
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
        except Exception as e:
            logger.warning("Spotify callback exchange error: %s", e)
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
        except Exception as refresh_err:
            logger.warning("Spotify token refresh error: %s", refresh_err)
        return None

    async def _fast_track_switch_poll(self, prev_track_id: str) -> None:
        """Fast Track Switch: адаптивне ультрашвидке опитування доки не перемкнеться track_id."""
        delays = [0.05, 0.12, 0.20, 0.30, 0.45, 0.65, 0.90, 1.20]
        for delay in delays:
            await asyncio.sleep(delay)
            st = await self.fetch_current_playback(force=True)
            if st.get("track_id") and st.get("track_id") != prev_track_id:
                logger.info("Fast track switch detected new track '%s' in %.2fs", st.get("title"), delay)
                break

    async def _prefetch_next_queue_track(self) -> None:
        """Pre-fetch next track in queue and pre-generate vinyl RGB565 into cache."""
        try:
            token = await self._ensure_access_token()
            if not token:
                return
            headers = {"Authorization": f"Bearer {token}"}
            client = http_pool.get()
            resp = await client.get("https://api.spotify.com/v1/me/player/queue", headers=headers, timeout=4.0)
            if resp.status_code == 200:
                data = resp.json()
                queue = data.get("queue", [])
                if queue:
                    next_item = queue[0]
                    next_tid = next_item.get("id")
                    if next_tid and f"{next_tid}_v69" not in self._vinyl_cache:
                        images = (next_item.get("album") or {}).get("images", [])
                        art_url = images[1].get("url") if len(images) > 1 else (images[0].get("url") if images else "")
                        if art_url:
                            art_resp = await client.get(art_url, timeout=5.0)
                            if art_resp.status_code == 200:
                                v_bytes, a1, a2, a3, mono = self._create_vinyl_art_bytes(art_resp.content, 120, 65)
                                if len(self._vinyl_cache) > 30:
                                    self._vinyl_cache.pop(next(iter(self._vinyl_cache)))
                                self._vinyl_cache[f"{next_tid}_v69"] = (v_bytes, a1, a2, a3, mono)
                                logger.info("Pre-cached vinyl for next queue track: %s (%s)", next_item.get("name"), next_tid)
        except Exception as q_err:
            logger.debug("Spotify queue prefetch error: %s", q_err)

    async def seek_track(self, position_ms: int) -> bool:
        """Перемотує поточний трек на позицію position_ms (Spotify Player Seek API)."""
        token = await self._ensure_access_token()
        if not token:
            return False

        headers = {"Authorization": f"Bearer {token}"}
        client = http_pool.get()
        url = f"https://api.spotify.com/v1/me/player/seek?position_ms={max(0, int(position_ms))}"
        try:
            resp = await client.put(url, headers=headers, timeout=5.0)
            ok = resp.status_code in (200, 204)
            if ok:
                self._cached_state["progress_ms"] = max(0, int(position_ms))
                self._last_poll_time = time.time()
                try:
                    from app.core.events import realtime_hub
                    cur_st = self.get_cached_state()
                    realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                    realtime_hub.emit_spotify_to_box(cur_st)
                except Exception as emit_err:
                    logger.debug("Spotify seek broadcast error: %s", emit_err)
            return ok
        except Exception as e:
            logger.warning("Spotify seek error: %s", e)
            return False

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

        # ⚡️ Миттєве оптимістичне оновлення кешу та пуш (Sub-5ms реакція на клік)
        now = time.time()
        self._action_lock_until = now + 2.5
        if action == "pause":
            if self._cached_state.get("playing") and self._last_poll_time > 0:
                elapsed_ms = int((now - self._last_poll_time) * 1000)
                orig_prog = self._cached_state.get("progress_ms", 0)
                dur = self._cached_state.get("duration_ms", 0)
                cur_prog = orig_prog + elapsed_ms
                self._cached_state["progress_ms"] = min(cur_prog, dur) if dur > 0 else cur_prog
            self._cached_state["playing"] = False
            self._last_poll_time = now
        elif action == "play":
            self._cached_state["playing"] = True
            self._last_poll_time = now
        elif action in ("next", "previous"):
            self._action_lock_until = now + 2.0
            self._cached_state["progress_ms"] = 0
            self._cached_state["playing"] = True
            self._last_poll_time = now

        try:
            from app.core.events import realtime_hub
            cur_st = self.get_cached_state()
            realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
            realtime_hub.emit_spotify_to_box(cur_st)
        except Exception as emit_err:
            logger.debug("Spotify optimistic broadcast error: %s", emit_err)

        method, url = endpoint_map[action]
        try:
            if method == "POST":
                resp = await client.post(url, headers=headers, timeout=5.0)
            else:
                resp = await client.put(url, headers=headers, timeout=5.0)
            ok = resp.status_code in (200, 204)
            if not ok and action == "play" and resp.status_code == 404:
                # Waking up idle Spotify device if 404 No Active Device
                try:
                    dev_resp = await client.get("https://api.spotify.com/v1/me/player/devices", headers=headers, timeout=4.0)
                    if dev_resp.status_code == 200:
                        devices = dev_resp.json().get("devices", [])
                        if devices:
                            dev_id = devices[0].get("id")
                            if dev_id:
                                retry_resp = await client.put(f"https://api.spotify.com/v1/me/player/play?device_id={dev_id}", headers=headers, timeout=5.0)
                                ok = retry_resp.status_code in (200, 204)
                                logger.info("Spotify resume fallback to device %s (status %s)", dev_id, retry_resp.status_code)
                except Exception as dev_err:
                    logger.warning("Spotify device resume fallback error: %s", dev_err)
            if ok and action in ("next", "previous"):
                asyncio.create_task(self._fast_track_switch_poll(self._last_track_id))
            return ok
        except Exception as action_err:
            logger.warning("Spotify player action HTTP error for '%s': %s", action, action_err)
            return False

    @staticmethod
    def _create_vinyl_art_bytes(album_img_bytes: bytes, bpm: int = 120, energy: int = 65) -> Tuple[bytes, List[int], List[int], List[int], bool]:
        """
        Генерує кругле зображення вінілової платівки 136x136 px RGB565 (рівно 36992 байти)
        з оптимізованим LUT кешем (Zero CPU waste) та адаптивною палітрою Mood-True Ambient 3.0.
        """
        import numpy as np
        import colorsys

        lut = _get_vinyl_lut()
        dist = lut["dist"]
        theta = lut["theta"]
        sheen = lut["sheen"]
        spindle_mask = lut["spindle_mask"]
        ring_mask = lut["ring_mask"]
        vinyl_mask = lut["vinyl_mask"]
        outer_mask = lut["outer_mask"]
        groove_59 = lut["groove_59"]
        groove_63 = lut["groove_63"]
        dither_r = lut["dither_r"]
        dither_g = lut["dither_g"]
        dither_b = lut["dither_b"]
        cover_edge_weight = lut["cover_edge_weight"]

        size = 136
        canvas_arr = np.zeros((size, size, 3), dtype=np.float32)

        ambi_rgb = [54, 48, 44]
        ambi2_rgb = [38, 34, 32]
        ambi3_rgb = [78, 66, 54]
        is_mono = False
        blur_bytes = b"\x00" * 2048

        try:
            with Image.open(io.BytesIO(album_img_bytes)) as raw_img:
                raw_rgb = raw_img.convert("RGB")

                # Apple Music / Canvas style blurred cover art background (32x32 RGB565 = 2048 bytes)
                try:
                    blur_img = raw_rgb.resize((32, 32), Image.Resampling.BILINEAR)
                    blur_img = blur_img.filter(ImageFilter.GaussianBlur(radius=2.5))
                    blur_img = ImageEnhance.Color(blur_img).enhance(1.25)
                    blur_arr = np.asarray(blur_img, dtype=np.uint16)
                    br = (blur_arr[:, :, 0] & 0xF8) << 8
                    bg = (blur_arr[:, :, 1] & 0xFC) << 3
                    bb = blur_arr[:, :, 2] >> 3
                    blur_rgb565 = br | bg | bb
                    blur_bytes = blur_rgb565.astype(">u2").tobytes()
                except Exception as blur_err:
                    logger.debug("Spotify blur texture generation fallback: %s", blur_err)

                enhanced = raw_rgb
                enhanced = ImageEnhance.Contrast(enhanced).enhance(1.15)
                enhanced = enhanced.filter(ImageFilter.UnsharpMask(radius=1.2, percent=125, threshold=3))

                # Масштабуємо обкладинку до 108x108 px (R = 54 px)
                rgb_img = ImageOps.fit(enhanced, (108, 108), method=Image.Resampling.LANCZOS)
                art_arr = np.asarray(rgb_img, dtype=np.float32)

                # Вставляємо обкладинку в центр 136x136 (відступ 14 px: 14..122)
                canvas_arr[14:122, 14:122] = art_arr

                # Кольоровий аналіз HSV (0.0 .. 1.0)
                norm_rgb = art_arr / 255.0
                max_c = np.max(norm_rgb, axis=2)
                min_c = np.min(norm_rgb, axis=2)
                delta = max_c - min_c

                v_chan = max_c
                s_chan = np.zeros_like(max_c)
                nz = max_c > 1e-5
                s_chan[nz] = delta[nz] / max_c[nz]

                h_chan = np.zeros_like(max_c)
                r_c = norm_rgb[:, :, 0]
                g_c = norm_rgb[:, :, 1]
                b_c = norm_rgb[:, :, 2]

                mask_r = nz & (delta > 1e-5) & (max_c == r_c)
                mask_g = nz & (delta > 1e-5) & (max_c == g_c)
                mask_b = nz & (delta > 1e-5) & (max_c == b_c)

                h_chan[mask_r] = ((g_c[mask_r] - b_c[mask_r]) / delta[mask_r]) % 6.0
                h_chan[mask_g] = ((b_c[mask_g] - r_c[mask_g]) / delta[mask_g]) + 2.0
                h_chan[mask_b] = ((r_c[mask_b] - g_c[mask_b]) / delta[mask_b]) + 4.0
                h_chan = (h_chan / 6.0) % 1.0

                mean_sat = float(np.mean(s_chan))
                mean_val = float(np.mean(v_chan))

                if mean_sat < 0.12:
                    # Чорно-біла / монохромна обкладинка -> Преміальний «Теплий графіт»
                    is_mono = True
                    ambi_rgb = [54, 48, 44]
                    ambi2_rgb = [38, 34, 32]
                    ambi3_rgb = [78, 66, 54]
                else:
                    is_mono = False
                    valid_mask = (v_chan >= 0.15) & (s_chan >= 0.15)
                    if np.sum(valid_mask) < 20:
                        valid_mask = (v_chan >= 0.08) & (s_chan >= 0.08)

                    # Застосовуємо просторовий крайній ваговий коефіцієнт обкладинки
                    weights = cover_edge_weight * (s_chan ** 1.2) * (v_chan ** 0.8)

                    h_valid = h_chan[valid_mask]
                    s_valid = s_chan[valid_mask]
                    v_valid = v_chan[valid_mask]
                    w_valid = weights[valid_mask]

                    if len(h_valid) > 0:
                        # 1. Primary: домінантний тон
                        bin_indices = np.clip((h_valid * 24.0).astype(int), 0, 23)
                        bin_weights = np.bincount(bin_indices, weights=w_valid, minlength=24)
                        best_bin = int(np.argmax(bin_weights))
                        h1 = (best_bin + 0.5) / 24.0

                        diff_h1 = np.abs(h_valid - h1)
                        circ_diff1 = np.minimum(diff_h1, 1.0 - diff_h1)

                        cluster1_mask = circ_diff1 <= (1.0 / 24.0)
                        if np.any(cluster1_mask):
                            raw_s = float(np.median(s_valid[cluster1_mask]))
                            raw_v = float(np.median(v_valid[cluster1_mask]))
                            # Виразний колір обкладинки із захистом від кислотного неону
                            s1 = float(np.clip(raw_s * 0.90, 0.28, 0.78))
                            v1 = float(np.clip(raw_v * 0.96, 0.54, 0.88))
                        else:
                            s1 = 0.48
                            v1 = 0.68

                        # Врахування настрою треку (реальний BPM, енергія та тональність обкладинки):
                        cur_bpm = bpm
                        cur_energy = energy

                        if cur_bpm < 95 or (cur_energy < 48 and mean_val < 0.35):
                            # Спокійний, інтимний лаунж: глибші оксамитові тони
                            s1 = float(np.clip(s1 * 0.88, 0.24, 0.68))
                            v1 = float(np.clip(v1 * 0.90, 0.48, 0.78))
                        elif cur_bpm > 126 or cur_energy > 72:
                            # Драйвовий, енергійний трек: насичений живий акцент
                            s1 = float(np.clip(s1 * 1.06, 0.32, 0.82))
                            v1 = float(np.clip(v1 * 1.05, 0.58, 0.90))

                        # 2. Secondary: реальний другий тон обкладинки або багата аналогова гармонія
                        mask2 = circ_diff1 >= (25.0 / 360.0)
                        if np.sum(mask2) >= 25:
                            b2 = np.clip((h_valid[mask2] * 24.0).astype(int), 0, 23)
                            w2 = np.bincount(b2, weights=w_valid[mask2], minlength=24)
                            h2 = (int(np.argmax(w2)) + 0.5) / 24.0
                            c2_mask = mask2 & (np.minimum(np.abs(h_valid - h2), 1.0 - np.abs(h_valid - h2)) <= (1.0 / 24.0))
                            if np.any(c2_mask):
                                s2 = float(np.clip(np.median(s_valid[c2_mask]) * 0.88, 0.24, 0.75))
                                v2 = float(np.clip(np.median(v_valid[c2_mask]) * 0.92, 0.48, 0.84))
                            else:
                                s2 = float(np.clip(s1 * 0.88, 0.24, 0.72))
                                v2 = float(np.clip(v1 * 0.90, 0.48, 0.82))
                        else:
                            h2 = (h1 + (18.0 / 360.0)) % 1.0
                            s2 = float(np.clip(s1 * 0.88, 0.24, 0.72))
                            v2 = float(np.clip(v1 * 0.90, 0.48, 0.82))

                        # 3. Tertiary: глибокий фоновий компаньйон обкладинки
                        h3 = (h1 - (18.0 / 360.0)) % 1.0
                        s3 = float(np.clip(s1 * 0.80, 0.20, 0.68))
                        v3 = float(np.clip(v1 * 0.82, 0.42, 0.76))

                        # Mood-True Ambient 3.0: збереження природної тональності без синтетичного неону
                        rgb1 = colorsys.hsv_to_rgb(h1, s1, v1)
                        rgb2 = colorsys.hsv_to_rgb(h2, s2, v2)
                        rgb3 = colorsys.hsv_to_rgb(h3, s3, v3)

                        ambi_rgb = [int(np.clip(rgb1[0] * 255.0, 0, 255)),
                                    int(np.clip(rgb1[1] * 255.0, 0, 255)),
                                    int(np.clip(rgb1[2] * 255.0, 0, 255))]
                        ambi2_rgb = [int(np.clip(rgb2[0] * 255.0, 0, 255)),
                                     int(np.clip(rgb2[1] * 255.0, 0, 255)),
                                     int(np.clip(rgb2[2] * 255.0, 0, 255))]
                        ambi3_rgb = [int(np.clip(rgb3[0] * 255.0, 0, 255)),
                                     int(np.clip(rgb3[1] * 255.0, 0, 255)),
                                     int(np.clip(rgb3[2] * 255.0, 0, 255))]
        except Exception as color_err:
            logger.debug("Spotify album color extraction fallback: %s", color_err)

        # 1. Spindle hole: r <= 4.0 px -> pure black #000000
        canvas_arr[spindle_mask] = [0.0, 0.0, 0.0]

        # 2. Ring near hole: 4.0 < r <= 6.0 px -> cover color (ambi_rgb)
        canvas_arr[ring_mask] = ambi_rgb

        # 3. Custom-pressed Colored Vinyl Rim: 54.0 < r <= 67.0 px -> tinted luxury vinyl
        tint_r = float(ambi_rgb[0]) * 0.22
        tint_g = float(ambi_rgb[1]) * 0.22
        tint_b = float(ambi_rgb[2]) * 0.22
        base_r = 14.0 + tint_r + sheen
        base_g = 14.0 + tint_g + sheen * 1.05
        base_b = 16.0 + tint_b + sheen * 1.15

        canvas_arr[vinyl_mask, 0] = np.clip(base_r[vinyl_mask], 10.0, 80.0)
        canvas_arr[vinyl_mask, 1] = np.clip(base_g[vinyl_mask], 10.0, 80.0)
        canvas_arr[vinyl_mask, 2] = np.clip(base_b[vinyl_mask], 10.0, 80.0)

        # Concentric grooves at r = 59.0 and r = 63.0
        canvas_arr[groove_59] = np.clip(canvas_arr[groove_59] * 0.7, 0.0, 255.0)
        canvas_arr[groove_63] = np.clip(canvas_arr[groove_63] * 0.7, 0.0, 255.0)

        # 4. Outer boundary: r > 67.0 px -> pure black #000000 (transparent mask)
        canvas_arr[outer_mask] = [0.0, 0.0, 0.0]

        # 5. Soft Bayer dithering on cover art (6.0 < r <= 54.0)
        art_mask = (dist > 6.0) & (dist <= 54.0)
        canvas_arr[art_mask, 0] = np.clip(canvas_arr[art_mask, 0] + dither_r[art_mask], 0.0, 255.0)
        canvas_arr[art_mask, 1] = np.clip(canvas_arr[art_mask, 1] + dither_g[art_mask], 0.0, 255.0)
        canvas_arr[art_mask, 2] = np.clip(canvas_arr[art_mask, 2] + dither_b[art_mask], 0.0, 255.0)

        # Global safety clip before uint16 cast
        canvas_arr = np.clip(canvas_arr, 0.0, 255.0)

        r_final = canvas_arr[:, :, 0].astype(np.uint16)
        g_final = canvas_arr[:, :, 1].astype(np.uint16)
        b_final = canvas_arr[:, :, 2].astype(np.uint16)

        r = (r_final & 0xF8) << 8
        g = (g_final & 0xFC) << 3
        b = b_final >> 3
        rgb565 = r | g | b

        # 100% захист від накривання пластинки фоном:
        # жоден внутрішній піксель вінілу не повинен мати прозорий ключ 0x0000!
        inside_mask = (dist <= 67.0)
        rgb565[(rgb565 == 0) & inside_mask] = 0x0821  # мінімальний оксамитовий чорний RGB565

        raw_bytes = rgb565.astype(">u2").tobytes()
        bundle_bytes = raw_bytes + blur_bytes
        return bundle_bytes, ambi_rgb, ambi2_rgb, ambi3_rgb, is_mono

    async def _fetch_track_bpm(self, track_id: str, title: str, artist: str) -> int:
        """Отримує точний BPM треку через Deezer API або повертає значення за замовчуванням (120)."""
        if track_id in self._bpm_cache:
            return self._bpm_cache[track_id]
        if not title:
            return 120
        clean_title = re.sub(r"\s*\(.*?\)|\s*\[.*?\]", "", title).strip()
        first_artist = artist.split(",")[0].strip() if artist else ""
        q = f"{clean_title} {first_artist}".strip()
        bpm = 120
        try:
            resp = await http_pool.get().get(
                "https://api.deezer.com/search",
                params={"q": q, "limit": 1},
                timeout=3.5,
            )
            if resp.status_code == 200:
                data = resp.json().get("data", [])
                if data:
                    dz_id = data[0].get("id")
                    if dz_id:
                        tr_resp = await http_pool.get().get(f"https://api.deezer.com/track/{dz_id}", timeout=3.5)
                        if tr_resp.status_code == 200:
                            val = float(tr_resp.json().get("bpm") or 0.0)
                            if 45.0 <= val <= 220.0:
                                bpm = int(round(val))
        except Exception as err:
            logger.debug("Deezer BPM fetch fallback: %s", err)
        if len(self._bpm_cache) > 50:
            self._bpm_cache.pop(next(iter(self._bpm_cache)))
        self._bpm_cache[track_id] = bpm
        return bpm

    def _calc_song_energy(self, bpm: int, progress_ms: int, duration_ms: int) -> int:
        """Розрахунок динамічної енергії треку (30..95) за BPM та прогресом пісні."""
        if duration_ms <= 0:
            return 50
        import math
        prog_frac = max(0.0, min(1.0, float(progress_ms) / float(duration_ms)))
        base = int(min(70, max(30, (bpm or 120) * 0.45)))
        arc = int(math.sin(prog_frac * math.pi) * 25.0)
        return min(95, max(25, base + arc))

    async def fetch_current_playback(self, force: bool = False) -> Dict[str, Any]:
        """Опитує офіційний Spotify API про поточний трек із захистом від rate-limit."""
        if not self.is_linked():
            return self.get_cached_state()

        now = time.time()
        # 1. Перевірка на активний Spotify rate-limit cooldown
        if self._rate_limited_until > now:
            if self._cached_state.get("playing"):
                self._cached_state["playing"] = False
                try:
                    from app.core.events import realtime_hub
                    cur_st = self.get_cached_state()
                    realtime_hub.emit_spotify_to_box(cur_st)
                    asyncio.create_task(realtime_hub.broadcast_to_dashboards({"type": "spotify", "spotify": cur_st}))
                except Exception:
                    pass
            return self.get_cached_state()

        min_poll_interval = 3.5 if (self._cached_state.get("playing") or self._hub_active) else 7.0
        if not force and (now - self._last_poll_time < min_poll_interval):
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

            # Обробка 429 Too Many Requests (Spotify Quota Exceeded)
            if resp.status_code == 429:
                retry_after = int(resp.headers.get("Retry-After", 60))
                self._rate_limited_until = now + retry_after
                logger.warning("Spotify API 429 Rate Limit! Cooldown for %d seconds", retry_after)
                prev_playing = self._cached_state.get("playing", False)
                self._cached_state["playing"] = False
                if prev_playing:
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state()
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                        realtime_hub.emit_spotify_to_box(cur_st)
                    except Exception:
                        pass
                return self.get_cached_state()

            if resp.status_code == 204 or not resp.content:
                if time.time() < self._action_lock_until:
                    return self.get_cached_state()
                prev_playing = self._cached_state.get("playing", False)
                self._cached_state["playing"] = False
                if prev_playing:
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state()
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                        realtime_hub.emit_spotify_to_box(cur_st)
                    except Exception as emit_err:
                        logger.debug("Spotify 204 pause state emit error: %s", emit_err)
                return self.get_cached_state()

            if resp.status_code == 200:
                self._rate_limited_until = 0.0
                data = resp.json()
                item = data.get("item") or {}
                track_id = item.get("id") or ""
                api_playing = bool(data.get("is_playing", False))
                if now < self._action_lock_until:
                    is_playing = self._cached_state.get("playing", api_playing)
                else:
                    is_playing = api_playing

                title = item.get("name", "")
                artists = [a.get("name", "") for a in item.get("artists", []) if a.get("name")]
                artist = ", ".join(artists) if artists else ""
                progress_ms = int(data.get("progress_ms", 0))
                duration_ms = int(item.get("duration_ms", 0))

                images = (item.get("album") or {}).get("images", [])
                # Оптимізація 300x300 preview (images[1]) для блискавичного скачування (< 40 мс)
                art_url = images[1].get("url") if len(images) > 1 else (images[0].get("url") if images else "")

                cache_key = f"{track_id}_v69"
                # Якщо змінився трек або палітри ще немає в кеші — Two-Phase Fast Track Push
                track_changed = bool(track_id and (track_id != self._last_track_id or cache_key not in self._vinyl_cache))
                new_vinyl_bytes = None
                if track_changed:
                    self._last_track_id = track_id
                    # Phase 0: Миттєвий Metadata Push у WebSocket лампи (назва, виконавець, скидання затримки)
                    self._cached_state.update({
                        "playing": is_playing,
                        "title": title[:120],
                        "artist": artist[:90],
                        "track_id": track_id[:32],
                        "cover_url": art_url,
                        "progress_ms": progress_ms,
                        "duration_ms": duration_ms,
                        "bpm": self._bpm_cache.get(track_id, 120),
                        "energy": 55,
                    })
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state()
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                        realtime_hub.emit_spotify_to_box(cur_st)
                    except Exception as p_imm_err:
                        logger.debug("Immediate metadata push to box error: %s", p_imm_err)

                    if cache_key in self._vinyl_cache:
                        vinyl_bytes, ambi, ambi2, ambi3, is_mono = self._vinyl_cache[cache_key]
                        self._last_ambi3_rgb = ambi3
                        self._last_ambi_rgb = ambi
                        self._last_ambi2_rgb = ambi2
                        self._last_is_mono = is_mono
                        new_vinyl_bytes = vinyl_bytes
                        with open(settings.SPOTIFY_VINYL_PATH, "wb") as f:
                            f.write(vinyl_bytes)
                    else:
                        if art_url:
                            # Паралельне завантаження обкладинки та реального BPM (Deezer) для 100% точної палітри за настроєм
                            art_task = http_pool.get().get(art_url, timeout=4.5)
                            bpm_task = self._fetch_track_bpm(track_id, title, artist)
                            art_resp, track_bpm = await asyncio.gather(art_task, bpm_task, return_exceptions=True)
                            if isinstance(track_bpm, Exception) or not isinstance(track_bpm, int):
                                track_bpm = self._bpm_cache.get(track_id, 120)
                            track_energy = self._calc_song_energy(track_bpm, progress_ms, duration_ms)

                            if not isinstance(art_resp, Exception) and getattr(art_resp, "status_code", 0) == 200:
                                vinyl_bytes, ambi, ambi2, ambi3, is_mono = await asyncio.to_thread(
                                    self._create_vinyl_art_bytes, art_resp.content, track_bpm, track_energy
                                )
                                self._last_ambi3_rgb = ambi3
                                self._last_ambi_rgb = ambi
                                self._last_ambi2_rgb = ambi2
                                self._last_is_mono = is_mono
                                new_vinyl_bytes = vinyl_bytes
                                if len(self._vinyl_cache) > 12:
                                    self._vinyl_cache.pop(next(iter(self._vinyl_cache)))
                                self._vinyl_cache[cache_key] = (vinyl_bytes, ambi, ambi2, ambi3, is_mono)
                                with open(settings.SPOTIFY_VINYL_PATH, "wb") as f:
                                    f.write(vinyl_bytes)
                                memory_guard.schedule_lazy_trim(5.0)

                    # Оновлюємо стан новими кольорами
                    self._cached_state["ambi_rgb"] = self._last_ambi_rgb
                    self._cached_state["ambi2_rgb"] = self._last_ambi2_rgb
                    self._cached_state["ambi3_rgb"] = self._last_ambi3_rgb
                    self._cached_state["is_mono"] = self._last_is_mono

                    # Phase 1: Push бінарного вінілу та стану з кольорами у відкритий WebSocket лампи
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state()
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                        realtime_hub.emit_spotify_to_box(cur_st)
                        if new_vinyl_bytes:
                            asyncio.create_task(realtime_hub.send_bytes_to_box(new_vinyl_bytes))
                    except Exception as p_err:
                        logger.debug("Vinyl and state push to box error: %s", p_err)

                    asyncio.create_task(self._prefetch_next_queue_track())

                # Швидкий розрахунок енергії з кешу або дефолту 120 (0 ms блокування)
                bpm = self._bpm_cache.get(track_id, 120)
                energy = self._calc_song_energy(bpm, progress_ms, duration_ms)

                prev_playing = self._cached_state.get("playing", False)
                self._cached_state["playing"] = is_playing
                self._cached_state["progress_ms"] = progress_ms
                self._cached_state["duration_ms"] = duration_ms
                self._cached_state["bpm"] = bpm
                self._cached_state["energy"] = energy

                try:
                    from app.core.events import realtime_hub
                    cur_st = self.get_cached_state()
                    realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                    if track_changed or (is_playing != prev_playing):
                        realtime_hub.emit_spotify_to_box(cur_st)
                    if track_changed:
                        import orjson
                        save_st = dict(cur_st)
                        save_st["playing"] = False
                        db_service.set_setting("spotify_last_state", orjson.dumps(save_st).decode("utf-8"))
                except Exception as emit_err:
                    logger.debug("Spotify state emit error: %s", emit_err)

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
        self._hub_active = hub_active or ambi_active
        dash_active = False
        try:
            from app.core.events import realtime_hub
            dash_active = len(realtime_hub._dashboard_clients) > 0
        except Exception as check_err:
            logger.debug("Failed checking active dashboard count: %s", check_err)

        if not hub_active and not ambi_active and not dash_active:
            return self.get_cached_state()

        return await self.fetch_current_playback()

    def get_lyric_offset_ms(self) -> int:
        """Повертає час випередження слів пісні в мс (за замовчуванням 150 мс для точної синхронізації без поспіху)."""
        try:
            val = db_service.get_setting("spotify_lyric_offset_ms", "150")
            offset = int(val)
            # Якщо в базі збереглося старе завищене значення (наприклад, 2500 мс), скидаємо до 150 мс
            if offset > 800 or offset < -800:
                offset = 150
                db_service.set_setting("spotify_lyric_offset_ms", "150")
            return offset
        except Exception:
            return 150

    def set_lyric_offset_ms(self, offset_ms: int) -> None:
        """Зберігає налаштування випередження тексту пісні у SQLite WAL."""
        db_service.set_setting("spotify_lyric_offset_ms", str(int(offset_ms)))
        self._last_pushed_lyric = ""
        self._ensure_lyrics_sync_task()

    def _ensure_lyrics_sync_task(self) -> None:
        pass

    async def _lyrics_sync_loop(self) -> None:
        pass

    async def _fetch_lyrics(self, track_id: str, title: str, artist: str, duration_sec: int) -> None:
        pass

    def _get_active_lyric(self, progress_ms: int) -> str:
        return ""

    def get_cached_state(self, include_lyrics: bool = False) -> Dict[str, Any]:
        st = dict(self._cached_state)
        st["linked"] = self.is_linked()
        try:
            st["bg_style"] = int(db_service.get_setting("spotify_bg_style", "0") or 0)
        except Exception:
            st["bg_style"] = 0
        if st.get("playing") and self._last_poll_time > 0:
            elapsed_ms = int((time.time() - self._last_poll_time) * 1000)
            orig_prog = st.get("progress_ms", 0)
            dur = st.get("duration_ms", 0)
            if dur > 0:
                cur_prog = orig_prog + elapsed_ms
                if cur_prog >= dur:
                    st["playing"] = False
                    st["progress_ms"] = dur
                    self._cached_state["playing"] = False
                    self._cached_state["progress_ms"] = dur
                else:
                    st["progress_ms"] = cur_prog
            else:
                st["progress_ms"] = orig_prog + elapsed_ms
        return st


spotify_service = SpotifyService()
