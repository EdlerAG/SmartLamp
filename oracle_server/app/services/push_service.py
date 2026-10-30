# Файл: oracle_server/app/services/push_service.py
import asyncio
import logging
from typing import Dict, Any, Optional
import orjson

from app.services.database_service import db_service

logger = logging.getLogger("magicbox.push")

try:
    from py_vapid import Vapid
    from py_vapid.utils import b64urlencode
    from cryptography.hazmat.primitives import serialization
    from pywebpush import webpush, WebPushException
    PYWEBPUSH_AVAILABLE = True
except ImportError:
    PYWEBPUSH_AVAILABLE = False


class PushService:
    def __init__(self):
        self._public_b64: Optional[str] = None
        self._private_pem: Optional[str] = None
        self._init_keys()

    def _init_keys(self) -> None:
        if not PYWEBPUSH_AVAILABLE:
            return

        pub = db_service.get_setting("vapid_public_b64")
        priv = db_service.get_setting("vapid_private_pem")

        if pub and priv:
            self._public_b64 = pub
            self._private_pem = priv
            return

        try:
            v = Vapid()
            v.generate_keys()
            raw_pub = v.public_key.public_bytes(
                serialization.Encoding.X962,
                serialization.PublicFormat.UncompressedPoint
            )
            pub_b64 = b64urlencode(raw_pub)
            priv_pem = v.private_pem().decode("utf-8")

            db_service.set_setting("vapid_public_b64", pub_b64)
            db_service.set_setting("vapid_private_pem", priv_pem)

            self._public_b64 = pub_b64
            self._private_pem = priv_pem
            logger.info("Generated and stored new VAPID keys for Web Push")
        except Exception as e:
            logger.error("Failed to generate VAPID keys: %s", e)

    def get_public_key(self) -> str:
        return self._public_b64 or ""

    def is_available(self) -> bool:
        return PYWEBPUSH_AVAILABLE and bool(self._public_b64 and self._private_pem)

    async def broadcast_push(self, title: str, body: str, url: str = "/", tag: Optional[str] = None) -> int:
        if not self.is_available():
            logger.debug("Push service not available or not configured")
            return 0

        subs = db_service.get_push_subscriptions()
        if not subs:
            return 0

        payload = orjson.dumps({
            "title": title,
            "body": body,
            "url": url,
            "tag": tag or "smartlamp-alert",
        }).decode("utf-8")

        sent_count = 0
        tasks = []
        for sub in subs:
            tasks.append(self._send_single_push(sub, payload))

        results = await asyncio.gather(*tasks, return_exceptions=True)
        for r in results:
            if isinstance(r, bool) and r:
                sent_count += 1

        logger.info("Broadcast push sent to %d/%d active subscriptions", sent_count, len(subs))
        return sent_count

    async def _send_single_push(self, sub: Dict[str, Any], payload: str) -> bool:
        endpoint = sub.get("endpoint", "")
        if not endpoint:
            return False

        def _sync_send() -> bool:
            try:
                webpush(
                    subscription_info=sub,
                    data=payload,
                    vapid_private_key=self._private_pem,
                    vapid_claims={"sub": "mailto:admin@smartlamp.local"},
                    timeout=5,
                )
                return True
            except WebPushException as ex:
                status_code = getattr(ex.response, "status_code", 0) if hasattr(ex, "response") else 0
                if status_code in (404, 410):
                    logger.info("Removing expired push subscription (%s): %s", status_code, endpoint[:40])
                    db_service.delete_push_subscription(endpoint)
                else:
                    logger.warning("WebPush error (%s): %s", status_code, ex)
                return False
            except Exception as ex:
                logger.warning("Failed to deliver push to %s: %s", endpoint[:40], ex)
                return False

        return await asyncio.to_thread(_sync_send)


push_service = PushService()
