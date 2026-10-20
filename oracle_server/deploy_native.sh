# Файл: oracle_server/deploy_native.sh
#!/usr/bin/env bash
set -e

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
USER_NAME="$(whoami)"

echo "🚀 [1/4] Перевірка системних пакетів Python venv..."
sudo apt-get update -qq
sudo apt-get install -y python3-venv python3-pip -qq

echo "📦 [2/4] Оновлення віртуального оточення .venv..."
cd "$APP_DIR"
python3 -m venv .venv
./.venv/bin/pip install --upgrade pip -q
./.venv/bin/pip install --no-cache-dir --compile -r requirements.txt

mkdir -p "$APP_DIR/storage/ota" "$APP_DIR/storage/stickers" "$APP_DIR/storage/sounds"

if [ ! -f .env ]; then
    cp .env.example .env
    echo "⚠️ Створено файл .env із шаблону. Відредагуй його: nano .env"
fi

echo "⚙️ [3/4] Налаштування фонової служби magicbox.service (HyperLight v5.0, ліміт RAM: 110MB)..."
sudo tee /etc/systemd/system/magicbox.service > /dev/null <<EOF
[Unit]
Description=MagicBox Sovereign Cloud v5.0
After=network.target network-online.target

[Service]
Type=simple
User=$USER_NAME
WorkingDirectory=$APP_DIR
EnvironmentFile=$APP_DIR/.env
Environment="PYTHONOPTIMIZE=1"
Environment="PYTHONDONTWRITEBYTECODE=1"
Environment="OMP_NUM_THREADS=1"
Environment="OPENBLAS_NUM_THREADS=1"
Environment="MALLOC_TRIM_THRESHOLD_=65536"
Environment="MALLOC_ARENA_MAX=2"
MemoryHigh=85M
MemoryMax=120M
ExecStart=$APP_DIR/.venv/bin/uvicorn main:app --host 0.0.0.0 --port 8000 --loop uvloop --http httptools --no-access-log --limit-concurrency 24
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF

echo "🔥 [4/4] Перезапуск служби magicbox..."
sudo systemctl daemon-reload
sudo systemctl enable magicbox
sudo systemctl restart magicbox

echo "✅ MagicBox Sovereign Cloud v5.0 запущено! Перевірка статусу:"
sudo systemctl status magicbox --no-pager