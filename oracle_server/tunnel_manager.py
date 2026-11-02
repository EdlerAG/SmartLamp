import subprocess
import re
import os
import sys
import urllib.request
import json

ENV_PATH = "/home/ubuntu/SmartLamp/oracle_server/.env"


def get_env_vars() -> dict:
    env_vars = {}
    if not os.path.exists(ENV_PATH):
        return env_vars
    with open(ENV_PATH, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                k, v = line.split("=", 1)
                env_vars[k.strip()] = v.strip().strip('"').strip("'")
    return env_vars


def update_telegram_menu_button(url: str, env_vars: dict) -> None:
    token = env_vars.get("TG_BOT_TOKEN") or os.getenv("TG_BOT_TOKEN", "")
    if not token:
        return
    clean_url = url.rstrip("/")
    web_app_url = f"{clean_url}/?v=5.2.0"
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

    # 2. Кнопка для зареєстрованих чатів з .env
    target_chats = set()
    for key in ("TG_ADMIN_ID", "TG_GIRL_ID"):
        val = env_vars.get(key) or os.getenv(key)
        if val:
            try:
                target_chats.add(int(val))
            except ValueError:
                pass

    allowed = env_vars.get("ALLOWED_CHAT_IDS") or os.getenv("ALLOWED_CHAT_IDS")
    if allowed:
        for cid in allowed.split(","):
            cid = cid.strip()
            if cid:
                try:
                    target_chats.add(int(cid))
                except ValueError:
                    pass

    for chat_id in target_chats:
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

    # Check existing URL to avoid duplicate restarts
    m = re.search(r"PUBLIC_SERVER_URL=(.*)", content)
    current_url = m.group(1).strip().strip('"').strip("'") if m else ""

    if current_url == url:
        print(f"[Tunnel] PUBLIC_SERVER_URL is already {url}. Skipping systemd restart.")
        env_vars = get_env_vars()
        update_telegram_menu_button(url, env_vars)
        return

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
    env_vars = get_env_vars()
    update_telegram_menu_button(url, env_vars)
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
