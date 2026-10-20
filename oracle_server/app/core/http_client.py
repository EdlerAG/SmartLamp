# Файл: oracle_server/app/core/http_client.py
from typing import Optional
import httpx


class SharedHttpClient:
    __slots__ = ("_client",)

    def __init__(self) -> None:
        self._client: Optional[httpx.AsyncClient] = None

    def get(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            limits = httpx.Limits(max_connections=8, max_keepalive_connections=3, keepalive_expiry=45.0)
            self._client = httpx.AsyncClient(limits=limits, timeout=10.0)
        return self._client

    async def close(self) -> None:
        if self._client and not self._client.is_closed:
            await self._client.aclose()


http_pool = SharedHttpClient()