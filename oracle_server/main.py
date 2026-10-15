from fastapi import FastAPI, Header, HTTPException, UploadFile, File, Request
from fastapi.responses import FileResponse
from pydantic import BaseModel
from typing import Optional, List
import secrets
import time
import os

app = FastAPI(title="MagicBox Oracle Cloud Hub", version="4.1")

API_KEY = os.getenv("MAGICBOX_KEY", "smartlamp-secret-key")
FIRMWARE_PATH = "latest_firmware.bin"
MAX_FW_SIZE = 4 * 1024 * 1024  # Максимум 4 МБ

box_telemetry = {"online": False, "last_seen": 0.0, "data": {}}
command_queue: List[dict] = []


def verify_key(provided_key: Optional[str]):
    if not provided_key or not secrets.compare_digest(provided_key, API_KEY):
        raise HTTPException(status_code=403, detail="Forbidden: Invalid API Key")


class BoxStatusPayload(BaseModel):
    fw: str
    t: float
    h: float
    p: float
    p_diff: float
    l: int
    m: int
    m_name: str
    auto_b: bool
    b_idx: int
    b_pct: int
    sleep_m: int
    scr: int
    rssi: int
    time: str
    unread: bool
    inbox_cnt: int
    oracle: Optional[bool] = True
    heap: int


class QueueCommandRequest(BaseModel):
    set_mode: Optional[int] = None
    sleep: Optional[int] = None
    react: Optional[int] = None
    sender: Optional[str] = None
    msg: Optional[str] = None
    ota_url: Optional[str] = None


@app.post("/api/v1/box/sync")
async def sync_box(payload: BoxStatusPayload, x_device_key: Optional[str] = Header(None)):
    verify_key(x_device_key)

    box_telemetry["online"] = True
    box_telemetry["last_seen"] = time.time()
    box_telemetry["data"] = payload.model_dump()

    if command_queue:
        cmd = command_queue.pop(0)
        cmd["has_cmd"] = True
        return cmd

    return {"has_cmd": False}


@app.post("/api/v1/box/command")
async def enqueue_command(cmd: QueueCommandRequest, x_device_key: Optional[str] = Header(None)):
    verify_key(x_device_key)
    command_queue.append(cmd.model_dump(exclude_none=True))
    return {"ok": True, "queued": len(command_queue)}


@app.post("/api/v1/ota/upload")
async def upload_firmware(
    request: Request,
    file: UploadFile = File(...),
    x_device_key: Optional[str] = Header(None)
):
    verify_key(x_device_key)

    content = await file.read(MAX_FW_SIZE + 1)
    if len(content) > MAX_FW_SIZE:
        raise HTTPException(status_code=413, detail="Firmware file too large")
    if len(content) < 150000 or content[0] != 0xE9:
        raise HTTPException(status_code=400, detail="Invalid ESP32 firmware binary (magic 0xE9 missing)")

    with open(FIRMWARE_PATH, "wb") as f:
        f.write(content)

    base_url = str(request.base_url).rstrip("/")
    command_queue.append({"ota_url": f"{base_url}/api/v1/ota/firmware.bin"})
    return {"ok": True, "size": len(content), "status": "OTA queued for MagicBox"}


@app.get("/api/v1/ota/firmware.bin")
async def download_firmware():
    if not os.path.exists(FIRMWARE_PATH):
        raise HTTPException(status_code=404, detail="Firmware not found")
    return FileResponse(FIRMWARE_PATH, media_type="application/octet-stream", filename="firmware.bin")


@app.get("/api/v1/box/state")
async def get_cloud_state(x_device_key: Optional[str] = Header(None)):
    verify_key(x_device_key)
    age = time.time() - box_telemetry["last_seen"]
    is_alive = box_telemetry["last_seen"] > 0 and age < 20.0
    return {
        "is_online": is_alive,
        "seconds_since_last_ping": round(age, 1) if box_telemetry["last_seen"] > 0 else None,
        "pending_commands": len(command_queue),
        "telemetry": box_telemetry["data"]
    }