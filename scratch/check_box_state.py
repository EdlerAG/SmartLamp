import urllib.request
import json
import time

for i in range(4):
    time.sleep(2)
    req = urllib.request.Request(
        "http://89.168.126.103:8000/api/v1/box/state",
        headers={"Authorization": "Bearer smartlamp-secret-key"},
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            data = json.loads(resp.read().decode())
            telem = data.get("telemetry", {})
            print(f"Check {i+1}: ping_age={data.get('seconds_since_last_ping')}s, online={data.get('is_online')}, fw={telem.get('fw')}, ota={telem.get('ota')}, rst={telem.get('rst')}")
    except Exception as e:
        print(f"Check {i+1} err:", e)
