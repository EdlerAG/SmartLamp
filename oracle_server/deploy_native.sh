#!/usr/bin/env bash
set -e

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
USER_NAME="$(whoami)"

echo "🚀 [1/4] Встановлення системних пакетів Python venv..."
sudo apt-get update -qq
sudo apt-get install -y python3-venv python3-pip -qq

echo "📦 [2/4] Створення легкого оточення .venv та встановлення бібліотек..."
cd "$APP_DIR"
python3 -m venv .venv
./.venv/bin/pip install --upgrade pip -q
./.venv/bin/pip install --no-cache-dir -r requirements.txt

if [ ! -f .env ]; then
    cp .env.example .env
    echo "⚠️ Створено файл .env із шаблону. Відредагуй його: nano .env"
fi

echo "⚙️ [3/4] Налаштування фонової служби systemd (ліміт RAM: 96MB, автозапуск 24/7)..."
sudo tee /etc/systemd/system/magicbox.service > /dev/null <<EOF
[Unit]
Description=MagicBox Oracle Cloud Hub (Ultra-Low RAM)
After=network.target

[Service]
Type=simple
User=$USER_NAME
WorkingDirectory=$APP_DIR
EnvironmentFile=$APP_DIR/.env
Environment="PYTHONOPTIMIZE=1"
Environment="PYTHONDONTWRITEBYTECODE=1"
Environment="MALLOC_TRIM_THRESHOLD_=65536"
Environment="MALLOC_ARENA_MAX=2"
MemoryMax=96M
ExecStart=$APP_DIR/.venv/bin/uvicorn main:app --host 0.0.0.0 --port 8000 --loop uvloop --http httptools --no-access-log --limit-concurrency 32
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF

echo "🔥 [4/4] Запуск служби magicbox..."
sudo systemctl daemon-reload
sudo systemctl enable magicbox
sudo systemctl restart magicbox

echo "✅ Готово! Перевірка статусу:"
sudo systemctl status magicbox --no-pager