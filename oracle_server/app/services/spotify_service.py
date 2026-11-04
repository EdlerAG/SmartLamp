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
        "_lyrics_cache",
        "_current_lyrics",
        "_bpm_cache",
        "_hub_active",
        "_lyrics_task",
        "_last_pushed_lyric",
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
            "ambi_rgb": [56, 189, 248],
            "ambi2_rgb": [248, 113, 113],
            "ambi3_rgb": [250, 204, 21],
            "is_mono": False,
            "bpm": 120,
            "energy": 65,
        }
        self._last_ambi_rgb: List[int] = [56, 189, 248]
        self._last_ambi2_rgb: List[int] = [248, 113, 113]
        self._last_ambi3_rgb: List[int] = [250, 204, 21]
        self._last_is_mono: bool = False
        self._vinyl_cache: Dict[str, Tuple[bytes, List[int], List[int], bool]] = {}
        self._lyrics_cache: Dict[str, List[Dict[str, Any]]] = {}
        self._current_lyrics: List[Dict[str, Any]] = []
        self._bpm_cache: Dict[str, int] = {}
        self._hub_active: bool = False
        self._lyrics_task: Optional[asyncio.Task] = None
        self._last_pushed_lyric: str = ""

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
            "ambi_rgb": [56, 189, 248],
            "ambi2_rgb": [248, 113, 113],
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
        """Fast Track Switch: адаптивне опитування протягом 2.5с доки не перемкнеться track_id."""
        delays = [0.15, 0.25, 0.35, 0.45, 0.60, 0.80, 1.10, 1.50]
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
                    if next_tid and next_tid not in self._vinyl_cache:
                        images = (next_item.get("album") or {}).get("images", [])
                        art_url = images[1].get("url") if len(images) > 1 else (images[0].get("url") if images else "")
                        if art_url:
                            art_resp = await client.get(art_url, timeout=5.0)
                            if art_resp.status_code == 200:
                                v_bytes, a1, a2, a3, mono = self._create_vinyl_art_bytes(art_resp.content)
                                if len(self._vinyl_cache) > 30:
                                    self._vinyl_cache.pop(next(iter(self._vinyl_cache)))
                                self._vinyl_cache[next_tid] = (v_bytes, a1, a2, a3, mono)
                                logger.info("Pre-cached vinyl for next queue track: %s (%s)", next_item.get("name"), next_tid)
        except Exception as q_err:
            logger.debug("Spotify queue prefetch error: %s", q_err)

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
            ok = resp.status_code in (200, 204)
            if ok:
                now = time.time()
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
                    self._cached_state["progress_ms"] = 0
                    self._cached_state["playing"] = True
                    self._last_poll_time = now
                    asyncio.create_task(self._fast_track_switch_poll(self._last_track_id))

                try:
                    from app.core.events import realtime_hub
                    cur_st = self.get_cached_state()
                    realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                    realtime_hub.emit_spotify_to_box(cur_st)
                except Exception as emit_err:
                    logger.debug("Spotify action broadcast error: %s", emit_err)
            return ok
        except Exception as action_err:
            logger.warning("Spotify player action HTTP error for '%s': %s", action, action_err)
            return False

    @staticmethod
    def _create_vinyl_art_bytes(album_img_bytes: bytes) -> Tuple[bytes, List[int], List[int], List[int], bool]:
        """
        Генерує кругле зображення вінілової платівки 136x136 px RGB565 (рівно 36992 байти):
        - r <= 4 px: чорний отвір по центру (#000000).
        - 4 < r <= 6 px: тонке кільце біля отвору в колір обкладинки.
        - 6 < r <= 54 px: чисте, яскраве зображення обкладинки альбому (LANCZOS + м'який Bayer-дизеринг).
        - 54 < r <= 67 px: темний вініловий обідок (#0C0E14) з доріжками r=59, r=63 та бліком.
        - r > 67 px: чистий чорний #000000 (прозорий фон для круглої маски).
        """
        import numpy as np
        import colorsys

        size = 136
        center = (size - 1) / 2.0
        y, x = np.ogrid[:size, :size]
        dist = np.sqrt((x - center) ** 2 + (y - center) ** 2)
        theta = np.arctan2(y - center, x - center)

        canvas_arr = np.zeros((size, size, 3), dtype=np.float32)

        ambi_rgb = [56, 189, 248]
        ambi2_rgb = [248, 113, 113]
        ambi3_rgb = [250, 204, 21]
        is_mono = False

        try:
            with Image.open(io.BytesIO(album_img_bytes)) as raw_img:
                enhanced = raw_img.convert("RGB")
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
                if mean_sat < 0.14:
                    is_mono = True
                    ambi_rgb = [255, 180, 50]   # Primary: Теплий бурштин
                    ambi2_rgb = [255, 120, 80]  # Secondary: Персиковий корал
                    ambi3_rgb = [255, 220, 140] # Accent: Перлинне сяйво
                else:
                    is_mono = False
                    valid_mask = (v_chan >= 0.15) & (s_chan >= 0.15)
                    if np.sum(valid_mask) < 20:
                        valid_mask = (v_chan >= 0.08) & (s_chan >= 0.08)

                    weights = np.ones((108, 108), dtype=np.float32)
                    weights[54:, :] = 2.0
                    weights = weights * s_chan * v_chan

                    h_valid = h_chan[valid_mask]
                    s_valid = s_chan[valid_mask]
                    v_valid = v_chan[valid_mask]
                    w_valid = weights[valid_mask]

                    if len(h_valid) > 0:
                        # 1. Primary: домінантний насичений тон (S >= 0.65, V >= 0.85)
                        bin_indices = np.clip((h_valid * 24.0).astype(int), 0, 23)
                        bin_weights = np.bincount(bin_indices, weights=w_valid, minlength=24)
                        best_bin = int(np.argmax(bin_weights))
                        h1 = (best_bin + 0.5) / 24.0

                        diff_h1 = np.abs(h_valid - h1)
                        circ_diff1 = np.minimum(diff_h1, 1.0 - diff_h1)

                        # 2. Secondary: контрастний тон (Delta H >= 35 deg)
                        mask2 = circ_diff1 >= (35.0 / 360.0)
                        if np.sum(mask2) >= 10:
                            b2 = np.clip((h_valid[mask2] * 24.0).astype(int), 0, 23)
                            w2 = np.bincount(b2, weights=w_valid[mask2], minlength=24)
                            h2 = (int(np.argmax(w2)) + 0.5) / 24.0
                        else:
                            h2 = (h1 + 0.5) % 1.0

                        # 3. Accent col3
                        diff_h2 = np.abs(h_valid - h2)
                        circ_diff2 = np.minimum(diff_h2, 1.0 - diff_h2)
                        mask3 = (circ_diff1 >= (30.0 / 360.0)) & (circ_diff2 >= (30.0 / 360.0))
                        if np.sum(mask3) >= 8:
                            score3 = s_valid[mask3] * v_valid[mask3]
                            idx3 = int(np.argmax(score3))
                            h3 = float(h_valid[mask3][idx3])
                        else:
                            h3 = (h1 + 0.25) % 1.0

                        # Гарантуємо соковитість кольорів (S >= 0.70, V >= 0.90)
                        rgb1 = colorsys.hsv_to_rgb(h1, 0.90, 1.0)
                        rgb2 = colorsys.hsv_to_rgb(h2, 0.82, 0.95)
                        rgb3 = colorsys.hsv_to_rgb(h3, 0.95, 1.0)

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
        canvas_arr[dist <= 4.0] = [0.0, 0.0, 0.0]

        # 2. Ring near hole: 4.0 < r <= 6.0 px -> cover color (ambi_rgb)
        ring_mask = (dist > 4.0) & (dist <= 6.0)
        canvas_arr[ring_mask] = ambi_rgb

        # 3. Custom-pressed Colored Vinyl Rim: 54.0 < r <= 67.0 px -> tinted luxury vinyl
        vinyl_mask = (dist > 54.0) & (dist <= 67.0)
        sheen = 9.0 * (np.cos(2.0 * theta - 0.785398) ** 4)
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
        gr_59 = (dist >= 58.6) & (dist <= 59.4)
        gr_63 = (dist >= 62.6) & (dist <= 63.4)
        canvas_arr[gr_59] = np.clip(canvas_arr[gr_59] * 0.7, 0.0, 255.0)
        canvas_arr[gr_63] = np.clip(canvas_arr[gr_63] * 0.7, 0.0, 255.0)

        # 4. Outer boundary: r > 67.0 px -> pure black #000000 (transparent mask)
        canvas_arr[dist > 67.0] = [0.0, 0.0, 0.0]

        # 5. Soft Bayer dithering on cover art (6.0 < r <= 54.0)
        bayer_matrix = np.array([
            [ 0,  8,  2, 10],
            [12,  4, 14,  6],
            [ 3, 11,  1,  9],
            [15,  7, 13,  5]
        ], dtype=np.float32)
        tile_y = (size + 3) // 4
        tile_x = (size + 3) // 4
        bayer_grid = np.tile(bayer_matrix, (tile_y, tile_x))[:size, :size]
        dither_r = (bayer_grid / 16.0 - 0.5) * 6.0
        dither_g = (bayer_grid / 16.0 - 0.5) * 3.5
        dither_b = (bayer_grid / 16.0 - 0.5) * 6.0

        art_mask = (dist > 6.0) & (dist <= 54.0)
        canvas_arr[art_mask, 0] = np.clip(canvas_arr[art_mask, 0] + dither_r[art_mask], 0.0, 255.0)
        canvas_arr[art_mask, 1] = np.clip(canvas_arr[art_mask, 1] + dither_g[art_mask], 0.0, 255.0)
        canvas_arr[art_mask, 2] = np.clip(canvas_arr[art_mask, 2] + dither_b[art_mask], 0.0, 255.0)

        # Global safety clip before uint16 cast (never overflow to 0xFFFF!)
        canvas_arr = np.clip(canvas_arr, 0.0, 255.0)

        r_final = canvas_arr[:, :, 0].astype(np.uint16)
        g_final = canvas_arr[:, :, 1].astype(np.uint16)
        b_final = canvas_arr[:, :, 2].astype(np.uint16)

        r = (r_final & 0xF8) << 8
        g = (g_final & 0xFC) << 3
        b = b_final >> 3
        rgb565 = r | g | b
        raw_bytes = rgb565.astype(">u2").tobytes()
        return raw_bytes, ambi_rgb, ambi2_rgb, ambi3_rgb, is_mono

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
        """
        Розрахунок реальної енергії треку (0..100) через щільність вокалу LRCLIB:
        - Без слів або довга пауза > 6.5 с (інтро, яма, соло, аутро): energy = 25 (0.25)
        - Звичайний темп рядків 3.5..6.0 с (куплет): energy = 55 (0.55)
        - Щільні рядки 1.0..3.2 с або повторення рядків (приспів, дроп, речитатив): energy = 90 (0.90)
        """
        if not self._current_lyrics or len(self._current_lyrics) < 2:
            return 25

        cur_sec = progress_ms / 1000.0
        lyrics = self._current_lyrics

        # Інтро до першого рядка
        if cur_sec < lyrics[0]["sec"]:
            gap = lyrics[0]["sec"] - cur_sec
            return 25 if gap > 6.5 else 55

        # Знаходимо активний рядок
        idx = 0
        for i, item in enumerate(lyrics):
            if item["sec"] <= cur_sec:
                idx = i
            else:
                break

        # Аутро після останнього рядка
        if idx >= len(lyrics) - 1:
            gap = cur_sec - lyrics[-1]["sec"]
            return 25 if gap > 6.5 else 55

        # Дельта часу до наступного рядка
        dt = lyrics[idx + 1]["sec"] - lyrics[idx]["sec"]

        # Перевірка на повторення рядків тексту (ознака приспіву / хука)
        is_repeat = False
        cur_txt = lyrics[idx]["text"].strip().lower()
        if idx > 0:
            prev_txt = lyrics[idx - 1]["text"].strip().lower()
            if cur_txt and cur_txt == prev_txt:
                is_repeat = True
        if idx + 1 < len(lyrics):
            next_txt = lyrics[idx + 1]["text"].strip().lower()
            if cur_txt and cur_txt == next_txt:
                is_repeat = True

        if is_repeat or dt <= 3.2:
            return 90
        elif dt <= 6.5:
            return 55
        else:
            return 25

    async def fetch_current_playback(self, force: bool = False) -> Dict[str, Any]:
        """Опитує офіційний Spotify API про поточний трек із захистом від rate-limit."""
        if not self.is_linked():
            return self.get_cached_state()

        now = time.time()
        min_poll_interval = 1.0 if (self._cached_state.get("playing") or self._hub_active) else 3.0
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

            if resp.status_code == 204 or not resp.content:
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
                data = resp.json()
                item = data.get("item") or {}
                track_id = item.get("id") or ""
                is_playing = bool(data.get("is_playing", False))

                title = item.get("name", "")
                artists = [a.get("name", "") for a in item.get("artists", []) if a.get("name")]
                artist = ", ".join(artists) if artists else ""
                progress_ms = int(data.get("progress_ms", 0))
                duration_ms = int(item.get("duration_ms", 0))

                images = (item.get("album") or {}).get("images", [])
                # Оптимізація 300x300 preview (images[1]) для блискавичного скачування (< 40 мс)
                art_url = images[1].get("url") if len(images) > 1 else (images[0].get("url") if images else "")

                # Якщо змінився трек — Two-Phase Fast Track Push
                track_changed = bool(track_id and track_id != self._last_track_id)
                new_vinyl_bytes = None
                if track_changed:
                    self._last_track_id = track_id
                    self._current_lyrics = []
                    self._cached_state["lyric"] = ""
                    if track_id in self._vinyl_cache:
                        vinyl_bytes, ambi, ambi2, ambi3, is_mono = self._vinyl_cache[track_id]
                        self._last_ambi3_rgb = ambi3
                        self._last_ambi_rgb = ambi
                        self._last_ambi2_rgb = ambi2
                        self._last_is_mono = is_mono
                        new_vinyl_bytes = vinyl_bytes
                        with open(settings.SPOTIFY_VINYL_PATH, "wb") as f:
                            f.write(vinyl_bytes)
                    else:
                        if art_url:
                            art_resp = await http_pool.get().get(art_url, timeout=5.0)
                            if art_resp.status_code == 200:
                                vinyl_bytes, ambi, ambi2, ambi3, is_mono = self._create_vinyl_art_bytes(art_resp.content)
                                self._last_ambi3_rgb = ambi3
                                self._last_ambi_rgb = ambi
                                self._last_ambi2_rgb = ambi2
                                self._last_is_mono = is_mono
                                new_vinyl_bytes = vinyl_bytes
                                if len(self._vinyl_cache) > 30:
                                    self._vinyl_cache.pop(next(iter(self._vinyl_cache)))
                                self._vinyl_cache[track_id] = (vinyl_bytes, ambi, ambi2, ambi3, is_mono)
                                with open(settings.SPOTIFY_VINYL_PATH, "wb") as f:
                                    f.write(vinyl_bytes)
                                memory_guard.trim_ram()

                    # Оновлюємо кешований стан негайно з новими кольорами та метаданими
                    self._cached_state = {
                        "playing": is_playing,
                        "title": title[:60],
                        "artist": artist[:45],
                        "track_id": track_id[:32],
                        "cover_url": art_url,
                        "progress_ms": progress_ms,
                        "duration_ms": duration_ms,
                        "ambi_rgb": self._last_ambi_rgb,
                        "ambi2_rgb": self._last_ambi2_rgb,
                        "ambi3_rgb": self._last_ambi3_rgb,
                        "is_mono": self._last_is_mono,
                        "bpm": self._bpm_cache.get(track_id, 120),
                        "energy": 55,
                        "lyric": "",
                    }

                    # Phase 1: Миттєвий Push бінарного вінілу та стану у відкритий WebSocket лампи (< 40 мс)
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state()
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                        realtime_hub.emit_spotify_to_box(cur_st)
                        if new_vinyl_bytes:
                            asyncio.create_task(realtime_hub.send_bytes_to_box(new_vinyl_bytes))
                    except Exception as p_err:
                        logger.debug("Vinyl and state push to box error: %s", p_err)

                    # Phase 2 (Background task): Асинхронне підвантаження BPM (Deezer) без лірики LRCLIB
                    async def _bg_enrich_and_update(tid: str, ttl: str, art: str, dur_sec: int):
                        await self._fetch_track_bpm(tid, ttl, art)
                        await asyncio.sleep(0.1)
                        if self._last_track_id == tid:
                            upd_st = self.get_cached_state()
                            try:
                                from app.core.events import realtime_hub
                                realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": upd_st})
                                realtime_hub.emit_spotify_to_box(upd_st)
                            except Exception:
                                pass

                    asyncio.create_task(_bg_enrich_and_update(track_id, title, artist, duration_ms // 1000))
                    asyncio.create_task(self._prefetch_next_queue_track())

                # Швидкий розрахунок енергії з кешу або дефолту 120 (0 ms блокування)
                bpm = self._bpm_cache.get(track_id, 120)
                energy = self._calc_song_energy(bpm, progress_ms, duration_ms)

                prev_playing = self._cached_state.get("playing", False)
                self._cached_state["playing"] = is_playing
                if is_playing:
                    self._ensure_lyrics_sync_task()
                elif self._lyrics_task and not self._lyrics_task.done():
                    self._lyrics_task.cancel()
                self._cached_state["progress_ms"] = progress_ms
                self._cached_state["duration_ms"] = duration_ms
                self._cached_state["bpm"] = bpm
                self._cached_state["energy"] = energy

                # Визначаємо поточний рядок тексту за прогресом
                cur_lyric = self._get_active_lyric(progress_ms)
                if cur_lyric:
                    self._cached_state["lyric"] = cur_lyric

                try:
                    from app.core.events import realtime_hub
                    cur_st = self.get_cached_state()
                    realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                    if track_changed or (is_playing != prev_playing):
                        realtime_hub.emit_spotify_to_box(cur_st)
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
        """Гарантує роботу фонового таймера своєчасного відправлення слів пісні."""
        if self._cached_state.get("playing") and self._current_lyrics:
            if self._lyrics_task is None or self._lyrics_task.done():
                try:
                    self._lyrics_task = asyncio.create_task(self._lyrics_sync_loop())
                except RuntimeError:
                    pass

    async def _lyrics_sync_loop(self) -> None:
        """
        Високоточний фоновий таймер синхронізації слів пісні (LRCLIB):
        - Розраховує локальний прогрес з урахуванням випередження (offset_ms ~2500 мс).
        - Спить точно до моменту настання наступного рядка (0.08..1.0 с).
        - Щойно рядок змінюється — миттєво пушить оновлений стан через WebSocket (< 15 мс latency).
        """
        try:
            while True:
                if not self._cached_state.get("playing") or not self._current_lyrics:
                    break

                now = time.time()
                elapsed_ms = int((now - self._last_poll_time) * 1000) if self._last_poll_time > 0 else 0
                dur = self._cached_state.get("duration_ms", 0)
                base_prog = self._cached_state.get("progress_ms", 0)
                cur_prog = base_prog + elapsed_ms
                if dur > 0 and cur_prog >= dur:
                    break

                offset_ms = self.get_lyric_offset_ms()
                eff_prog_sec = (cur_prog + offset_ms) / 1000.0

                active_lyric = ""
                next_sec: Optional[float] = None
                for item in self._current_lyrics:
                    sec = item["sec"]
                    if sec <= eff_prog_sec:
                        active_lyric = item["text"]
                    else:
                        next_sec = sec
                        break

                if active_lyric != self._last_pushed_lyric:
                    self._last_pushed_lyric = active_lyric
                    self._cached_state["lyric"] = active_lyric
                    try:
                        from app.core.events import realtime_hub
                        cur_st = self.get_cached_state(include_lyrics=False)
                        realtime_hub.emit_spotify_to_box(cur_st)
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": cur_st})
                    except Exception as emit_err:
                        logger.debug("Lyrics loop emit error: %s", emit_err)

                if next_sec is not None:
                    time_to_next = next_sec - eff_prog_sec
                    sleep_sec = max(0.08, min(time_to_next, 1.0))
                else:
                    sleep_sec = 1.0

                await asyncio.sleep(sleep_sec)
        except asyncio.CancelledError:
            pass
        except Exception as loop_err:
            logger.debug("Lyrics loop error: %s", loop_err)
        finally:
            self._lyrics_task = None

    async def _fetch_lyrics(self, track_id: str, title: str, artist: str, duration_sec: int) -> None:
        if track_id in self._lyrics_cache:
            if self._last_track_id == track_id:
                self._current_lyrics = self._lyrics_cache[track_id]
                try:
                    from app.core.events import realtime_hub
                    realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": self.get_cached_state()})
                except Exception:
                    pass
            return
        clean_title = re.sub(r"\s*\(.*?\)|\s*\[.*?\]", "", title).strip()
        first_artist = artist.split(",")[0].strip()
        params = {
            "track_name": clean_title,
            "artist_name": first_artist,
        }
        if duration_sec > 10:
            params["duration"] = str(duration_sec)
        try:
            resp = await http_pool.get().get("https://lrclib.net/api/get", params=params, timeout=5.0)
            if resp.status_code == 200:
                data = resp.json()
                synced = data.get("syncedLyrics") or ""
                parsed = []
                for line in synced.splitlines():
                    m = re.match(r"\[(\d+):(\d+(?:\.\d+)?)\]\s*(.*)", line)
                    if m:
                        sec = int(m.group(1)) * 60 + float(m.group(2))
                        txt = m.group(3).strip()
                        if txt:
                            parsed.append({"sec": sec, "text": txt})
                if len(self._lyrics_cache) > 20:
                    self._lyrics_cache.pop(next(iter(self._lyrics_cache)))
                self._lyrics_cache[track_id] = parsed
                if self._last_track_id == track_id:
                    self._current_lyrics = parsed
                    try:
                        from app.core.events import realtime_hub
                        realtime_hub.emit_to_dashboards({"type": "spotify", "spotify": self.get_cached_state()})
                    except Exception:
                        pass
            else:
                if self._last_track_id == track_id:
                    self._current_lyrics = []
        except Exception as lrc_err:
            logger.debug("Lyrics fetch error from lrclib: %s", lrc_err)
            if self._last_track_id == track_id:
                self._current_lyrics = []

    def _get_active_lyric(self, progress_ms: int) -> str:
        if not self._current_lyrics:
            return ""
        offset_ms = self.get_lyric_offset_ms()
        cur_sec = (progress_ms + offset_ms) / 1000.0
        active = ""
        for item in self._current_lyrics:
            if item["sec"] <= cur_sec:
                active = item["text"]
            else:
                break
        return active

    def get_cached_state(self, include_lyrics: bool = True) -> Dict[str, Any]:
        st = dict(self._cached_state)
        st["linked"] = self.is_linked()
        if st.get("playing") and self._last_poll_time > 0:
            elapsed_ms = int((time.time() - self._last_poll_time) * 1000)
            orig_prog = st.get("progress_ms", 0)
            dur = st.get("duration_ms", 0)
            if dur > 0:
                st["progress_ms"] = min(orig_prog + elapsed_ms, dur)
            else:
                st["progress_ms"] = orig_prog + elapsed_ms
        cur_prog = st.get("progress_ms", 0)
        active_l = self._get_active_lyric(cur_prog)
        if active_l:
            st["lyric"] = active_l
        if include_lyrics:
            st["lyrics"] = self._current_lyrics
        return st


spotify_service = SpotifyService()
