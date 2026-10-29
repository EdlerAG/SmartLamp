import subprocess
import re
import os
import sys

ENV_PATH = "/home/ubuntu/SmartLamp/oracle_server/.env"

def update_env_url(url: str) -> None:
    if not os.path.exists(ENV_PATH):
        print(f"[Tunnel] Warning: {ENV_PATH} not found.")
        return

    with open(ENV_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    # Update PUBLIC_SERVER_URL
    if "PUBLIC_SERVER_URL=" in content:
        content = re.sub(r"PUBLIC_SERVER_URL=.*", f"PUBLIC_SERVER_URL={url}", content)
    else:
        content += f"\nPUBLIC_SERVER_URL={url}\n"

    # Update SPOTIFY_REDIRECT_URI if present
    if "SPOTIFY_REDIRECT_URI=" in content:
        content = re.sub(r"SPOTIFY_REDIRECT_URI=.*", f"SPOTIFY_REDIRECT_URI={url}/api/v5/spotify/callback", content)

    # Update GOOGLE_REDIRECT_URI if present
    if "GOOGLE_REDIRECT_URI=" in content:
        content = re.sub(r"GOOGLE_REDIRECT_URI=.*", f"GOOGLE_REDIRECT_URI={url}/api/v5/google/callback", content)

    with open(ENV_PATH, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"[Tunnel] Successfully updated PUBLIC_SERVER_URL to {url}")
    subprocess.run(["sudo", "systemctl", "restart", "smartlamp.service"], check=False)
    print("[Tunnel] smartlamp.service restarted with new HTTPS endpoint.")

def main() -> None:
    cmd = ["/usr/local/bin/cloudflared", "tunnel", "--url", "http://127.0.0.1:8000", "--no-autoupdate"]
    print(f"[Tunnel] Starting cloudflared tunnel to http://127.0.0.1:8000 ...")
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)

    url_found = False
    try:
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            if not url_found:
                m = re.search(r"https://[a-zA-Z0-9-]+\.trycloudflare\.com", line)
                if m:
                    tunnel_url = m.group(0)
                    url_found = True
                    update_env_url(tunnel_url)
    except KeyboardInterrupt:
        proc.terminate()
    finally:
        if proc.poll() is None:
            proc.terminate()

if __name__ == "__main__":
    main()
