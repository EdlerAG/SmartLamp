from pydantic import BaseModel, Field
from typing import Optional


class BoxStatusPayload(BaseModel):
    fw: str
    t: float
    h: float
    p: float
    p_diff: float = 0.0
    l: int
    m: int
    m_name: str
    auto_b: bool
    b_idx: int
    b_pct: int
    sleep_m: int = 0
    scr: int = 0
    rssi: int
    time: str
    unread: bool = False
    inbox_cnt: int = 1
    oracle: Optional[bool] = True
    heap: int
    ack: Optional[bool] = False
    ping: Optional[int] = 0


class QueueCommandRequest(BaseModel):
    set_mode: Optional[int] = Field(None, ge=0, le=10)
    set_brt: Optional[str] = Field(None, description="'auto' або '0'..'3'")
    sleep: Optional[int] = Field(None, ge=0, le=180)
    react: Optional[int] = Field(None, ge=2, le=6)
    sender: Optional[str] = Field(None, max_length=26)
    msg: Optional[str] = Field(None, max_length=62)
    ota_url: Optional[str] = None
    play_url: Optional[str] = None
    img_url: Optional[str] = None


class AiBriefingRequest(BaseModel):
    custom_note: Optional[str] = None