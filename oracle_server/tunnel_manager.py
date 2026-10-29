import subprocess
import re
import os
import sys
import urllib.request
import json

ENV_PATH = "/home/ubuntu/SmartLamp/oracle_server/.env"


def get_bot_token() -> str:
    if not os.path.exists(ENV_PATH):
        return ""
    with open(ENV_PATH, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("TG_BOT_TOKEN="):
                return line.split("=", 1)[1].strip()
    return ""


def update_telegram_menu_button(url: str) -> None:
    token = get_bot_token()
    if not token:
        return
    clean_url = url.rstrip("/")
    web_app_url = f"{clean_url}/?v=5.1.0"
    api_url = f"https://api.telegram.org/bot{token}/setChatMenuButton"
    
    # 1. Загальна кнопка за замовчуванням
    payload = json.dumps({
        "menu_button": {
            "type": "web_app",
            "text": "📱 Studio",
            "web_app": {"url": web_app_url},
        }
    }).encode("utf-8")
    try:
        req = urllib.request.Request(api_url, data=payload, headers={"Content-Type": "application/json"})
        with urllib.request.urlopen(req, timeout=10) as resp:
            print(f"[Tunnel] Telegram Default Menu Button updated: {resp.read().decode()}")
    except Exception as e:
        print(f"[Tunnel] Failed to update Telegram Default Menu Button: {e}")

    # 2. Кнопка конкретно для зареєстрованих чатів
    for chat_id in (1047202957, 1294097414):
        user_payload = json.dumps({
            "chat_id": chat_id,
            "menu_button": {
                "type": "web_app",
                "text": "📱 Studio",
                "web_app": {"url": web_app_url},
            }
        }).encode("utf-8")
        try:
            req = urllib.request.Request(api_url, data=user_payload, headers={"Content-Type": "application/json"})
            with urllib.request.urlopen(req, timeout=10) as resp:
                print(f"[Tunnel] Telegram Menu Button for {chat_id} updated: {resp.read().decode()}")
        except Exception as e:
            print(f"[Tunnel] Failed to update Telegram Menu Button for {chat_id}: {e}")


def update_env_url(url: str) -> None:
    if not os.path.exists(ENV_PATH):
        print(f"[Tunnel] Warning: {ENV_PATH} not found.")
        return

    with open(ENV_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    if "PUBLIC_SERVER_URL=" in content:
        content = re.sub(r"PUBLIC_SERVER_URL=.*", f"PUBLIC_SERVER_URL={url}", content)
    else:
        content += f"\nPUBLIC_SERVER_URL={url}\n"

    if "SPOTIFY_REDIRECT_URI=" in content:
        content = re.sub(r"SPOTIFY_REDIRECT_URI=.*", f"SPOTIFY_REDIRECT_URI={url}/api/v5/spotify/callback", content)

    if "GOOGLE_REDIRECT_URI=" in content:
        content = re.sub(r"GOOGLE_REDIRECT_URI=.*", f"GOOGLE_REDIRECT_URI={url}/api/v5/google/callback", content)

    with open(ENV_PATH, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"[Tunnel] Successfully updated PUBLIC_SERVER_URL to {url}")
    update_telegram_menu_button(url)
    subprocess.run(["sudo", "systemctl", "restart", "smartlamp.service"], check=False)
    print("[Tunnel] smartlamp.service restarted with new HTTPS endpoint.")


STATIC_DOMAIN = os.getenv("STATIC_DOMAIN", "yearling-shy-reviving.ngrok-free.dev")


def main() -> None:
    ngrok_bin = "/usr/local/bin/ngrok"
    if os.path.exists(ngrok_bin):
        tunnel_url = f"https://{STATIC_DOMAIN}"
        print(f"[Tunnel] Starting permanent Ngrok static HTTPS tunnel ({tunnel_url}) to http://127.0.0.1:8000 ...")
        update_env_url(tunnel_url)
        cmd = [
            ngrok_bin,
            "http",
            f"--url={STATIC_DOMAIN}",
            "8000",
            "--log=stdout",
        ]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        try:
            for line in proc.stdout:
                sys.stdout.write(line)
                sys.stdout.flush()
        except KeyboardInterrupt:
            proc.terminate()
        finally:
            if proc.poll() is None:
                proc.terminate()
        return

    cmd = [
        "/usr/bin/ssh",
        "-o", "StrictHostKeyChecking=no",
        "-o", "ServerAliveInterval=30",
        "-o", "ServerAliveCountMax=3",
        "-o", "ExitOnForwardFailure=yes",
        "-R", "80:127.0.0.1:8000",
        "nokey@localhost.run",
    ]
    print("[Tunnel] Starting localhost.run HTTPS tunnel to http://127.0.0.1:8000 ...")
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)

    last_url = ""
    try:
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            m = re.search(r"https://[a-zA-Z0-9-]+\.lhr\.life", line)
            if m:
                tunnel_url = m.group(0)
                if tunnel_url != last_url:
                    last_url = tunnel_url
                    update_env_url(tunnel_url)
    except KeyboardInterrupt:
        proc.terminate()
    finally:
        if proc.poll() is None:
            proc.terminate()


if __name__ == "__main__":
    main()
