import urllib.request
import os

url = "http://89.168.126.103:8000/api/v1/ota/upload"
fw_path = os.path.join(".pio", "build", "esp32-s3-devkitc-1", "firmware.bin")
boundary = "----SmartLampBoundary20261104"

if not os.path.exists(fw_path):
    print("Error: firmware.bin not found at", fw_path)
    exit(1)

with open(fw_path, "rb") as f:
    file_bytes = f.read()

body = bytearray()
body.extend(f"--{boundary}\r\n".encode("utf-8"))
body.extend(b'Content-Disposition: form-data; name="file"; filename="firmware.bin"\r\n')
body.extend(b"Content-Type: application/octet-stream\r\n\r\n")
body.extend(file_bytes)
body.extend(f"\r\n--{boundary}--\r\n".encode("utf-8"))

req = urllib.request.Request(
    url,
    data=bytes(body),
    headers={
        "Authorization": "Bearer smartlamp-secret-key",
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body)),
    },
    method="POST",
)

try:
    print(f"Uploading firmware.bin ({len(file_bytes)} bytes) to Oracle Server OTA...")
    with urllib.request.urlopen(req, timeout=90) as resp:
        print("Upload status:", resp.status)
        print("Response:", resp.read().decode())
except Exception as e:
    print("Upload failed:", e)
