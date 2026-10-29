# Файл: oracle_server/app/web/portal_html.py
PORTAL_HTML: str = r"""<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,viewport-fit=cover">
<title>MagicBox Sovereign Studio • v5.1.0</title>
<!-- Hallmark & Sovereign Protocol v5.0 -->
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600;700&family=Plus+Jakarta+Sans:wght@400;500;600;700&display=swap" rel="stylesheet">
<script src="https://telegram.org/js/telegram-web-app.js"></script>
<script src="https://cdn.plot.ly/plotly-2.35.2.min.js"></script>
<style>
:root {
  --bg-primary: #07090e;
  --bg-elevated: #0f121a;
  --bg-surface: #151924;
  --bg-surface-active: #1e2434;
  --border-hairline: #1e2536;
  --border-focus: #38bdf8;
  --text-primary: #f8fafc;
  --text-secondary: #94a3b8;
  --text-muted: #64748b;
  --accent-warm: #f59e0b;
  --accent-cool: #38bdf8;
  --accent-success: #10b981;
  --accent-danger: #ef4444;
  --accent-spotify: #1ed760;
  --accent-pink: #f472b6;
  --font-sans: 'Plus Jakarta Sans', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  --font-mono: 'JetBrains Mono', ui-monospace, Menlo, Monaco, Consolas, monospace;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg-primary);
  color: var(--text-primary);
  font-family: var(--font-sans);
  min-height: 100vh;
  padding: 12px 12px calc(80px + env(safe-area-inset-bottom, 0px)) 12px;
  -webkit-font-smoothing: antialiased;
  overflow-x: clip;
}
.mono {
  font-family: var(--font-mono);
  font-variant-numeric: tabular-nums;
}
.wrap {
  max-width: 980px;
  margin: 0 auto;
}

/* Header bar */
.top-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 14px;
  padding: 12px 16px;
  margin-bottom: 12px;
}
.brand {
  display: flex;
  align-items: center;
  gap: 10px;
}
.logo {
  width: 36px;
  height: 36px;
  border-radius: 10px;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
}
.pulse {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  display: inline-block;
  margin-right: 6px;
}
.pulse.on { background: var(--accent-success); box-shadow: 0 0 8px rgba(16,185,129,0.5); }
.pulse.off { background: var(--accent-danger); }

/* Quick Telemetry Strip */
.quick-strip {
  display: flex;
  align-items: center;
  gap: 8px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 8px 12px;
  margin-bottom: 12px;
  overflow-x: auto;
}
.quick-pill {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 5px 10px;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 8px;
  font-size: 11px;
  white-space: nowrap;
}
.quick-pill span { color: var(--text-muted); }
.quick-pill b { color: var(--text-primary); font-family: var(--font-mono); font-variant-numeric: tabular-nums; }

/* Tabs Header */
.tabs-header {
  display: flex;
  gap: 6px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 5px;
  margin-bottom: 14px;
  overflow-x: auto;
}
.tab-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 9px 12px;
  border-radius: 8px;
  border: 1px solid transparent;
  background: transparent;
  color: var(--text-secondary);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s ease;
  white-space: nowrap;
}
.tab-btn:hover {
  color: var(--text-primary);
  background: var(--bg-surface);
}
.tab-btn.active {
  background: var(--bg-surface-active);
  color: var(--accent-cool);
  border-color: var(--border-hairline);
  box-shadow: 0 2px 6px rgba(0,0,0,0.3);
}

/* Bottom Navigation Dock */
.bottom-dock {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  background: rgba(15, 18, 26, 0.95);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border-top: 1px solid var(--border-hairline);
  display: flex;
  justify-content: space-around;
  align-items: center;
  padding: 8px 12px calc(8px + env(safe-area-inset-bottom, 0px));
  z-index: 100;
  box-shadow: 0 -4px 20px rgba(0, 0, 0, 0.4);
}
.bnav-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 3px;
  background: none;
  border: none;
  color: var(--text-muted);
  font-family: var(--font-sans);
  font-size: 10px;
  font-weight: 600;
  cursor: pointer;
  padding: 5px 8px;
  border-radius: 8px;
  transition: color 0.15s, background-color 0.15s;
  flex: 1;
  max-width: 80px;
}
.bnav-item .bnav-icon {
  font-size: 17px;
  line-height: 1;
}
.bnav-item.active {
  color: var(--accent-cool);
  background: rgba(56, 189, 248, 0.1);
}

/* Card & Layout Components */
.card {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 14px;
  padding: 16px;
  margin-bottom: 12px;
}
.card h3 {
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: var(--text-secondary);
  margin-bottom: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.grid-2 {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}
@media (max-width: 768px) {
  .grid-2 { grid-template-columns: 1fr; }
}
.grid-3 {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 10px;
}
@media (max-width: 600px) {
  .grid-3 { grid-template-columns: 1fr; }
}

/* Buttons */
.btn {
  background: var(--bg-surface);
  color: var(--text-secondary);
  border: 1px solid var(--border-hairline);
  padding: 9px 12px;
  border-radius: 8px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: border-color 0.15s, color 0.15s, background-color 0.15s;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
}
.btn:hover {
  border-color: var(--text-secondary);
  color: var(--text-primary);
}
.btn.active {
  background: var(--bg-surface-active);
  border-color: var(--accent-warm);
  color: var(--accent-warm);
  font-weight: 700;
}
.btn-emerald {
  background: var(--bg-surface);
  border-color: var(--accent-success);
  color: var(--accent-success);
}
.btn-cool {
  background: var(--accent-cool);
  color: #08090c;
  border: none;
  font-weight: 700;
}
.btn-danger {
  background: var(--bg-surface);
  border-color: var(--accent-danger);
  color: var(--accent-danger);
}
.btn-spotify {
  background: #1ed760;
  color: #041006;
  border: none;
  font-weight: 700;
}

/* Input Fields */
input, select, textarea {
  width: 100%;
  background: var(--bg-surface);
  color: var(--text-primary);
  border: 1px solid var(--border-hairline);
  padding: 9px 12px;
  border-radius: 8px;
  font-size: 13px;
  font-family: var(--font-sans);
  outline: none;
  margin: 4px 0;
  transition: border-color 0.15s;
}
input:focus, textarea:focus, select:focus {
  border-color: var(--border-focus);
}
input::placeholder, textarea::placeholder {
  color: var(--text-muted);
}

/* Vinyl Hero Animation */
@keyframes spinVinyl {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}
.vinyl-disc {
  width: 72px;
  height: 72px;
  border-radius: 50%;
  background: radial-gradient(circle, #252a3a 15%, #08090d 16%, #08090d 45%, #181c28 46%, #08090d 48%, #1f2535 70%);
  border: 2px solid var(--accent-warm);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 26px;
  box-shadow: 0 4px 16px rgba(0,0,0,0.6);
  flex-shrink: 0;
}
.vinyl-disc.spinning {
  animation: spinVinyl 5s linear infinite;
}

/* Range Sliders */
.slider-container {
  display: flex;
  align-items: center;
  gap: 12px;
}
.slider-container input[type="range"] {
  flex: 1;
  accent-color: var(--accent-warm);
  cursor: pointer;
}

/* Media Grid */
.media-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
  gap: 10px;
  max-height: 280px;
  overflow-y: auto;
  padding-right: 4px;
}
.media-item {
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 8px;
  padding: 10px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 8px;
}
.ambi-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  display: inline-block;
  vertical-align: middle;
  border: 1px solid var(--border-hairline);
}

/* Modal and Toast */
#loginModal {
  position: fixed;
  inset: 0;
  background: rgba(7,9,14,0.94);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 999;
}
.login-box {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 14px;
  padding: 26px;
  width: 340px;
  text-align: center;
}
.toast {
  position: fixed;
  bottom: 74px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--bg-surface-active);
  border: 1px solid var(--border-hairline);
  color: var(--accent-success);
  padding: 8px 16px;
  border-radius: 8px;
  font-family: var(--font-mono);
  font-size: 12px;
  font-weight: 600;
  display: none;
  z-index: 1000;
  box-shadow: 0 8px 24px rgba(0,0,0,0.6);
  white-space: nowrap;
}
</style>
</head>
<body>

<div id="loginModal">
  <div class="login-box">
    <div style="font-size:32px;margin-bottom:8px">🪄</div>
    <h2 style="font-size:16px;margin-bottom:6px">MagicBox Studio • Sovereign v5.0</h2>
    <p style="font-size:12px;color:var(--text-secondary);margin-bottom:16px">Введіть PIN (0511) або пароль адміністратора</p>
    <input type="password" id="pwdInput" placeholder="PIN (0511)...">
    <button class="btn btn-cool" style="width:100%;margin-top:10px" onclick="doLogin()">Увійти</button>
  </div>
</div>

<div id="hellcatModal" style="position:fixed;inset:0;background:rgba(5,6,10,0.94);backdrop-filter:blur(10px);display:none;align-items:center;justify-content:center;z-index:1000;padding:16px">
  <div style="background:var(--bg-elevated);border:1px solid #e63946;box-shadow:0 0 35px rgba(230,57,70,0.25);border-radius:16px;max-width:420px;width:100%;padding:24px;text-align:center;position:relative">
    <button onclick="closeHellcatModal()" style="position:absolute;top:14px;right:16px;background:none;border:none;color:var(--text-secondary);font-size:22px;cursor:pointer;line-height:1">&times;</button>
    <div style="display:inline-flex;align-items:center;gap:6px;padding:3px 10px;border-radius:20px;background:rgba(230,57,70,0.15);border:1px solid #e63946;color:#ff6b6b;font-size:10px;font-weight:700;letter-spacing:1px;text-transform:uppercase;margin-bottom:12px">
      ⚡️ HELLCAT PROTOCOL • V5.0
    </div>
    <h2 style="font-size:18px;font-weight:800;color:var(--text-primary);margin-bottom:6px">
      SmartLamp Sovereign Gateway
    </h2>
    <p style="font-size:12px;color:var(--text-secondary);margin-bottom:14px;line-height:1.4">
      Відскануйте QR-код для керування шкатулкою через Telegram-бота:
    </p>
    <div style="background:#fff;padding:10px;border-radius:10px;display:inline-block;margin-bottom:12px">
      <img src="https://api.qrserver.com/v1/create-qr-code/?size=180x180&amp;margin=2&amp;data=https%3A%2F%2Ft.me%2FSmartLampEdlerAGBot" alt="QR Code" width="180" height="180" style="display:block">
    </div>
    <div class="mono" style="font-size:13px;font-weight:700;color:#ff6b6b;margin-bottom:14px">
      @SmartLampEdlerAGBot
    </div>
    <div style="display:flex;flex-direction:column;gap:8px">
      <a href="https://t.me/SmartLampEdlerAGBot" target="_blank" class="btn btn-cool" style="background:#e63946;color:#fff;border:none;text-decoration:none;padding:10px;border-radius:8px;font-weight:700;font-size:12px">
        📱 Відкрити @SmartLampEdlerAGBot
      </a>
      <button onclick="dismissHellcatWelcome()" class="btn" style="background:var(--bg-surface);border:1px solid var(--border-hairline);padding:9px;border-radius:8px;font-size:12px">
        ✨ Перейти до Студії
      </button>
    </div>
  </div>
</div>

<div class="wrap">
  <!-- Top Bar -->
  <div class="top-bar">
    <div class="brand">
      <div class="logo">🪄</div>
      <div>
        <div style="display:flex;align-items:center;gap:6px">
          <span id="onlineDot" class="pulse on"></span>
          <b id="onlineTxt" style="font-size:13px;letter-spacing:0.4px">MAGICBOX SOVEREIGN</b>
          <span class="mono" style="font-size:10px;padding:2px 6px;border-radius:4px;background:rgba(56,189,248,0.15);border:1px solid #38bdf8;color:#38bdf8" id="verBadge">v5.1.0 ⚡ Live</span>
        </div>
        <div class="mono" style="font-size:10px;color:var(--text-secondary);margin-top:2px">
          FW: <span id="fwBadge">--</span> | RAM: <b id="ramTxt" style="color:var(--accent-success)">-- MB</b> | Диск: <b id="diskTxt">-- MB</b> | Черга: <b id="queueTxt">0</b>
        </div>
      </div>
    </div>
    <div style="display:flex;gap:6px;align-items:center">
      <span id="roleBadge" class="mono" style="font-size:11px;padding:4px 8px;border-radius:6px;background:var(--bg-surface);border:1px solid var(--border-hairline);color:var(--accent-warm)">🌸 НАСТЯ</span>
      <button id="dndTopBtn" class="btn" style="padding:5px 10px;font-size:11px" onclick="toggleDnd()">🌙 DND: ВИКЛ</button>
      <button onclick="location.reload(true)" class="btn" style="padding:5px 8px;font-size:11px" title="Оновити сторінку">🔄</button>
      <button onclick="openHellcatModal()" class="btn" style="border-color:#e63946;color:#ff6b6b;padding:5px 8px;font-size:11px" title="QR Код">⚡️ QR</button>
      <button onclick="doLogout()" class="btn" style="padding:5px 8px;font-size:11px">🚪</button>
    </div>
  </div>

  <!-- Quick Telemetry Strip -->
  <div class="quick-strip">
    <div class="quick-pill">
      <span>Темп:</span>
      <b id="kTemp" style="color:var(--accent-cool)">--.- °C</b>
    </div>
    <div class="quick-pill">
      <span>Вологість:</span>
      <b id="kHum" style="color:var(--accent-success)">-- %</b>
    </div>
    <div class="quick-pill">
      <span>Тиск:</span>
      <b id="kPres" style="color:var(--accent-warm)">--- мм</b>
    </div>
    <div class="quick-pill" style="margin-left:auto">
      <span>Режим:</span>
      <b id="kMode" style="color:var(--text-primary)">--</b>
    </div>
  </div>

  <!-- Segmented Navigation Tabs (Top) -->
  <div class="tabs-header">
    <button class="tab-btn active" id="tab-btn-music" onclick="switchTab('music')">
      🎵 Музика
    </button>
    <button class="tab-btn" id="tab-btn-light" onclick="switchTab('light')">
      💡 Світло
    </button>
    <button class="tab-btn" id="tab-btn-climate" onclick="switchTab('climate')">
      📊 Клімат
    </button>
    <button class="tab-btn" id="tab-btn-routine" onclick="switchTab('routine')">
      ⏱ Рутина
    </button>
    <button class="tab-btn" id="tab-btn-system" onclick="switchTab('system')">
      ⚙️ Система
    </button>
  </div>

  <!-- TAB 1: 🎵 МУЗИКА & ЗВУК -->
  <div id="view-music" class="tab-view">
    <!-- Spotify Vinyl Hero -->
    <div class="card" style="background:radial-gradient(ellipse at top left, rgba(30,215,96,0.06), transparent 70%), var(--bg-elevated)">
      <div style="display:flex;align-items:center;gap:16px;margin-bottom:14px">
        <div id="spDisc" class="vinyl-disc">🎵</div>
        <div style="flex:1;min-width:0">
          <div style="display:flex;align-items:center;gap:8px;margin-bottom:2px">
            <b id="spTitle" style="font-size:15px;color:var(--text-primary);overflow:hidden;text-overflow:ellipsis;white-space:nowrap;display:block">Spotify не підключено</b>
            <span id="spBadge" class="mono" style="font-size:9px;padding:2px 6px;border-radius:6px;background:var(--border-hairline);color:var(--text-secondary);flex-shrink:0">IDLE</span>
          </div>
          <div id="spArtist" class="mono" style="font-size:11px;color:var(--text-secondary);overflow:hidden;text-overflow:ellipsis;white-space:nowrap">
            Підключіть Spotify для трансляції вінілу на шкатулку (Вікно 3)
          </div>
          <div style="width:100%;height:5px;background:var(--border-hairline);border-radius:3px;margin-top:8px;overflow:hidden">
            <div id="spProgressBar" style="width:0%;height:100%;background:var(--accent-spotify);transition:width 0.4s linear;border-radius:3px"></div>
          </div>
        </div>
      </div>

      <!-- Controls -->
      <div style="display:flex;gap:8px;align-items:center;justify-content:center;flex-wrap:wrap" id="spControls">
        <button class="btn" style="padding:8px 16px;font-size:14px" onclick="spotifyAction('previous')" title="Попередній трек">⏮</button>
        <button class="btn btn-emerald" style="padding:8px 24px;font-size:15px;font-weight:700" id="spPlayBtn" onclick="spotifyAction('toggle')" title="Play / Pause">⏯</button>
        <button class="btn" style="padding:8px 16px;font-size:14px" onclick="spotifyAction('next')" title="Наступний трек">⏭</button>
        <button class="btn" style="padding:8px 12px;font-size:11px" onclick="sendCmd({set_mode:11})">🎨 Spotify AmbiLight</button>
        <button class="btn btn-cool" style="padding:8px 14px;font-size:11px" id="spLoginBtn" onclick="loginSpotify()">🔗 Підключити Spotify</button>
        <button class="btn btn-danger" style="padding:8px 10px;font-size:11px;display:none" id="spDisconnectBtn" onclick="disconnectSpotify()">✕ Відключити</button>
      </div>
    </div>

    <!-- Акустика & Гучність -->
    <div class="card">
      <h3><span>🔊 Акустика та Гучність Динаміка</span></h3>
      <div style="margin-bottom:12px;background:var(--bg-surface);padding:12px;border-radius:10px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
          <span style="font-size:11px;font-weight:600;color:var(--text-secondary)">ГУЧНІСТЬ КОЛОНКИ: <b id="volVal" class="mono" style="color:var(--accent-warm)">12 / 21</b></span>
          <div style="display:flex;gap:6px">
            <button class="btn" id="muteBtn" onclick="toggleMute()" style="padding:4px 8px;font-size:11px">🔊 ЗВУК: УВІМК</button>
            <button class="btn" id="touchBeepBtn" onclick="toggleTouchBeep()" style="padding:4px 8px;font-size:11px">🔔 КЛІКИ: ТАК</button>
          </div>
        </div>
        <div class="slider-container">
          <span class="mono" style="font-size:11px;color:var(--text-muted)">0</span>
          <input type="range" id="volSlider" min="0" max="21" value="12" oninput="onVolSlider(this.value)" onchange="applyVolume(this.value)">
          <span class="mono" style="font-size:11px;color:var(--text-muted)">21</span>
        </div>
      </div>

      <!-- Синтез голосу (TTS) -->
      <form onsubmit="saveVoiceConfig(event)">
        <div class="grid-2" style="margin-bottom:8px">
          <div>
            <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase">Рушій TTS:</label>
            <select id="ttsProviderSelect" onchange="onProviderChange()">
              <option value="edge-tts">Edge-TTS (Безкоштовно)</option>
              <option value="elevenlabs">ElevenLabs (Клонований)</option>
              <option value="fish_audio">Fish Audio (Нейро-голос)</option>
            </select>
          </div>
          <div>
            <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase">Голос Edge-TTS:</label>
            <select id="edgeVoiceSelect">
              <option value="uk-UA-OstapNeural">Остап 👨 (uk-UA-Ostap)</option>
              <option value="uk-UA-PolinaNeural">Поліна 👩 (uk-UA-Polina)</option>
            </select>
          </div>
        </div>
        <div id="cloneVoiceRow" style="display:none;margin-bottom:8px">
          <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase">Voice Clone ID:</label>
          <input type="text" id="cloneVoiceIdInput" placeholder="eleven_voice_id...">
        </div>
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px">
          <button type="submit" class="btn active">Зберегти налаштування голосу</button>
        </div>
      </form>

      <!-- Голосові пресети -->
      <div>
        <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase;display:block;margin-bottom:6px">Швидкі фрази:</label>
        <div id="voicePresetsList" style="display:flex;flex-wrap:wrap;gap:6px"></div>
      </div>
    </div>
  </div>

  <!-- TAB 2: 💡 СВІТЛО & ЕФЕКТИ -->
  <div id="view-light" class="tab-view" style="display:none">
    <!-- Яскравість & Пресети -->
    <div class="card">
      <h3><span>☀️ Швидка Яскравість Лампи</span></h3>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:14px">
        <button class="btn" onclick="sendCmd({set_brt:'auto'})">АВТО</button>
        <button class="btn" onclick="sendCmd({set_brt:'0'})">🌙 15%</button>
        <button class="btn" onclick="sendCmd({set_brt:'1'})">☕ 35%</button>
        <button class="btn" onclick="sendCmd({set_brt:'2'})">✨ 65%</button>
        <button class="btn" onclick="sendCmd({set_brt:'3'})">☀️ 100%</button>
      </div>

      <!-- Режими світла -->
      <h3 style="margin-top:10px"><span>🎨 Режими Підсвітки (LED Modes)</span></h3>
      <div class="mode-grid" id="modeGrid" style="display:grid;grid-template-columns:repeat(auto-fit, minmax(135px, 1fr));gap:6px;margin-bottom:14px"></div>

      <!-- Інтерактивні реакції -->
      <h3><span>✨ Живі Реакції на Екран</span></h3>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:12px">
        <button class="btn" onclick="sendCmd({react:2,sender:'Studio',msg:'FIRE VIBE!'})">🔥 Вогник</button>
        <button class="btn" onclick="sendCmd({react:3,sender:'Studio',msg:'KUS-ZAP!'})">⚡ Розряд</button>
        <button class="btn" onclick="sendCmd({react:4,sender:'Studio',msg:'TYTS-KUS!'})">🐱 Котик</button>
        <button class="btn" onclick="sendCmd({react:5,sender:'Studio',msg:'UFO BEAM!'})">👽 НЛО</button>
        <button class="btn" onclick="sendCmd({react:6,sender:'Studio',msg:'QUEEN!'})">👑 Корона</button>
      </div>

      <!-- Повідомлення на екран або в голос -->
      <div style="display:flex;gap:6px">
        <input type="text" id="msgTxt" placeholder="Повідомлення рідною кирилицею на екран або для озвучки...">
        <button class="btn active" style="white-space:nowrap" onclick="pushScreenMsg()">💬 На екран</button>
        <button class="btn btn-emerald" style="white-space:nowrap" onclick="speakTts()">🔊 Озвучити</button>
      </div>
    </div>
  </div>

  <!-- TAB 3: 📊 КЛІМАТ & ДІМ -->
  <div id="view-climate" class="tab-view" style="display:none">
    <!-- Повні датчики -->
    <div class="grid-3" style="margin-bottom:12px">
      <div class="card" style="text-align:center;padding:16px 12px">
        <div style="font-size:24px;margin-bottom:4px">🌡️</div>
        <div style="font-size:11px;color:var(--text-muted);text-transform:uppercase">Температура</div>
        <b style="font-size:26px;color:var(--accent-cool);font-family:var(--font-mono);display:block;margin-top:4px" id="kTempCard">--.- °C</b>
      </div>
      <div class="card" style="text-align:center;padding:16px 12px">
        <div style="font-size:24px;margin-bottom:4px">💧</div>
        <div style="font-size:11px;color:var(--text-muted);text-transform:uppercase">Вологість</div>
        <b style="font-size:26px;color:var(--accent-success);font-family:var(--font-mono);display:block;margin-top:4px" id="kHumCard">-- %</b>
      </div>
      <div class="card" style="text-align:center;padding:16px 12px">
        <div style="font-size:24px;margin-bottom:4px">🧭</div>
        <div style="font-size:11px;color:var(--text-muted);text-transform:uppercase">Барометр</div>
        <b style="font-size:22px;color:var(--accent-warm);font-family:var(--font-mono);display:block;margin-top:4px" id="kPresCard">--- мм</b>
      </div>
    </div>

    <!-- Графік клімату -->
    <div class="card">
      <h3>
        <span>📈 Історія Клімату та Барометра (24 години)</span>
        <button class="btn" style="padding:4px 8px;font-size:11px" onclick="pollState(true)">🔄 Оновити</button>
      </h3>
      <div id="climateChart" style="height:280px"></div>
    </div>
  </div>

  <!-- TAB 4: ⏱ РУТИНА & ФОКУС -->
  <div id="view-routine" class="tab-view" style="display:none">
    <div class="grid-2">
      <!-- Pomodoro Таймер -->
      <div class="card">
        <h3><span>📚 Фокус Pomodoro</span></h3>
        <div style="background:var(--bg-surface);padding:14px;border-radius:10px;border:1px solid var(--border-hairline);margin-bottom:12px;text-align:center">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
            <span style="font-size:11px;color:var(--text-muted)">СТАТУС:</span>
            <b id="pomoStatus" class="mono" style="font-size:11px;color:var(--accent-cool)">НЕАКТИВНИЙ</b>
          </div>
          <b id="pomoTimer" class="mono" style="font-size:36px;color:var(--accent-warm);display:block;margin:6px 0">25:00</b>
          <div style="display:flex;gap:6px;align-items:center;justify-content:center;margin-top:10px">
            <button class="btn" style="padding:6px 10px" onclick="adjustPomoInput(-5)">-5 хв</button>
            <input type="number" id="pomoInput" value="25" min="5" max="120" style="width:60px;text-align:center;padding:6px" class="mono">
            <button class="btn" style="padding:6px 10px" onclick="adjustPomoInput(5)">+5 хв</button>
            <button class="btn active" style="padding:6px 14px" onclick="startPomodoro(parseInt(document.getElementById('pomoInput').value)||25)">▶️ Старт</button>
            <button class="btn btn-danger" style="padding:6px 10px" onclick="stopPomodoro()">🛑 Стоп</button>
          </div>
        </div>
        <div style="font-size:11px;color:var(--text-muted);line-height:1.4">
          💡 Робота: теплий бурштин без сповіщень. Перерва: смарагдовий бриз. Кнопки шкатулки: Ліва -хв, Права +хв, Кришка Старт/Пауза.
        </div>
      </div>

      <!-- Тихі Години & Розумний Світанок -->
      <div class="card">
        <h3><span>🌸 Затишок і Сон (Настя)</span></h3>
        
        <!-- Тихі години -->
        <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline);margin-bottom:10px">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
            <span style="font-size:11px;font-weight:600;color:var(--text-secondary)">🌙 ТИХІ ГОДИНИ:</span>
            <button class="btn" id="qhToggleBtn" onclick="toggleQuietHours()" style="padding:3px 8px;font-size:10px">ВИМКНЕНО</button>
          </div>
          <div style="display:grid;grid-template-columns:1fr 1fr auto;gap:6px;align-items:center">
            <input type="time" id="qhStart" value="23:00" style="padding:5px 6px;font-size:11px">
            <input type="time" id="qhEnd" value="08:00" style="padding:5px 6px;font-size:11px">
            <button class="btn" style="padding:5px 8px;font-size:10px" onclick="saveQuietHours()">Зберегти</button>
          </div>
        </div>

        <!-- Світанок -->
        <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline);margin-bottom:10px">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
            <span style="font-size:11px;font-weight:600;color:var(--text-secondary)">🌅 РОЗУМНИЙ СВІТАНОК:</span>
            <button class="btn" id="alarmToggleBtn" onclick="toggleAlarm()" style="padding:3px 8px;font-size:10px">ВИМКНЕНО</button>
          </div>
          <div style="display:grid;grid-template-columns:1fr auto;gap:6px;align-items:center">
            <input type="time" id="alarmTime" value="07:30" style="padding:5px 6px;font-size:11px">
            <button class="btn" style="padding:5px 8px;font-size:10px" onclick="saveAlarm()">Зберегти</button>
          </div>
        </div>

        <!-- Ранкова порада -->
        <button class="btn btn-emerald" style="width:100%;padding:8px" onclick="triggerMorningBrief()">🌤 Отримати ранкову пораду Насті</button>
        <div id="morningBriefBox" style="margin-top:8px;font-size:11px;color:var(--text-primary);background:var(--bg-surface);border:1px solid var(--border-hairline);padding:8px;border-radius:6px;display:none"></div>
      </div>
    </div>

    <!-- Google Календар & Розклад -->
    <div class="card">
      <h3>
        <span>📅 Приватний Календар & Розклад</span>
        <span id="calStatusBadge" class="mono" style="font-size:10px;color:var(--text-muted)">НЕ ПІДКЛЮЧЕНО</span>
      </h3>
      <div id="calNextEvent" class="mono" style="font-size:12px;color:var(--accent-cool);margin-bottom:10px">
        📅 Найближча подія: --
      </div>

      <!-- Додати подію -->
      <div style="display:grid;grid-template-columns:80px 1fr auto;gap:6px;align-items:center;margin-bottom:10px">
        <input type="text" id="calTimeInput" placeholder="14:30" class="mono" style="margin:0;padding:6px;font-size:11px">
        <input type="text" id="calSummaryInput" placeholder="Назва події (напр. Лекція)..." style="margin:0;padding:6px;font-size:11px">
        <button class="btn btn-emerald" style="padding:6px 12px;font-size:11px" onclick="addCalendarEvent()">+ Додати</button>
      </div>

      <!-- Список -->
      <div id="calEventList" style="max-height:140px;overflow-y:auto;margin-bottom:10px;font-size:11px">
        <div style="color:var(--text-muted)">Завантаження розкладу...</div>
      </div>

      <div style="display:flex;gap:6px">
        <button class="btn" style="padding:5px 10px;font-size:11px" onclick="loadCalendarEvents()">🔄 Оновити</button>
        <button class="btn btn-cool" id="calLoginBtn" style="padding:5px 10px;font-size:11px" onclick="loginGoogle()">🔗 Підключити Google</button>
        <button class="btn btn-danger" id="calDisconnectBtn" style="padding:5px 10px;font-size:11px;display:none" onclick="disconnectGoogle()">✕ Відключити</button>
      </div>
    </div>
  </div>

  <!-- TAB 5: ⚙️ СИСТЕМА & МЕДІА -->
  <div id="view-system" class="tab-view" style="display:none">
    <!-- Студія Гіфок (.mbx AmbiLight) -->
    <div class="card">
      <h3>
        <span>🎞 Студія Кастомних Гіфок (.mbx)</span>
        <button class="btn btn-danger" style="padding:3px 8px;font-size:10px" onclick="stopMedia()">🛑 Зупинити показ</button>
      </h3>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:10px;background:var(--bg-surface);padding:8px;border-radius:8px;border:1px solid var(--border-hairline)">
        <span class="mono" style="font-size:10px;color:var(--text-secondary)">⏱ Таймер:</span>
        <button class="btn" style="padding:3px 6px;font-size:10px" onclick="setMediaTimer(15)">15 хв</button>
        <button class="btn" style="padding:3px 6px;font-size:10px" onclick="setMediaTimer(60)">1 год</button>
        <button class="btn" style="padding:3px 6px;font-size:10px" onclick="setMediaTimer(180)">3 год</button>
        <button class="btn" style="padding:3px 6px;font-size:10px" onclick="setMediaTimer(0)">♾ Безкінечно</button>
      </div>

      <form onsubmit="uploadMedia(event)" style="display:grid;grid-template-columns:1fr 1fr auto;gap:6px;align-items:center;margin-bottom:10px">
        <input type="file" id="mediaFile" accept=".gif,.webp,.png,.jpg,.jpeg" required style="padding:5px">
        <div style="display:flex;gap:4px">
          <input type="text" id="mediaName" placeholder="Назва..." style="padding:5px">
          <select id="mediaSlot" style="width:110px;padding:5px">
            <option value="0">Без прив'язки</option>
            <option value="2">🔥 Вогник</option>
            <option value="3">⚡ Блискавка</option>
            <option value="4">🐱 Котик</option>
            <option value="5">👽 НЛО</option>
            <option value="6">👑 Корона</option>
          </select>
        </div>
        <button type="submit" class="btn btn-cool" style="padding:8px 12px">Завантажити</button>
      </form>
      <div class="media-grid" id="mediaGrid"></div>
    </div>

    <!-- Персона ШІ -->
    <div class="card">
      <h3><span>🧠 Персона ШІ Духа (Gemini)</span></h3>
      <form onsubmit="saveAiPersona(event)">
        <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase">Ім'я Духа:</label>
        <input type="text" id="aiName" value="Затишний Дух">
        <label style="font-size:10px;color:var(--text-secondary);text-transform:uppercase;margin-top:4px;display:block">Характер (Промпт):</label>
        <textarea id="aiPrompt" rows="2"></textarea>
        <div style="display:flex;justify-content:space-between;align-items:center;margin-top:6px">
          <button type="submit" class="btn active">Зберегти характер</button>
          <button type="button" class="btn btn-emerald" onclick="askAiBriefing()">Слухати свіжий брифінг</button>
        </div>
      </form>
      <div id="aiResponseBox" style="margin-top:8px;font-size:11px;color:var(--text-secondary);background:var(--bg-surface);border:1px solid var(--border-hairline);padding:8px;border-radius:6px;display:none"></div>
    </div>

    <!-- 🛡️ Панель Архітектора (Ярослав) -->
    <div id="adminPanel" style="display:none" class="card">
      <h3><span>🛡️ Панель Архітектора (Ярослав)</span></h3>
      <div class="grid-2" style="margin-bottom:10px">
        <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline)">
          <div style="font-size:11px;font-weight:600;color:var(--accent-warm);margin-bottom:4px">🎂 ПРИВІТАННЯ:</div>
          <button class="btn btn-cool" style="width:100%;padding:6px;font-size:11px" onclick="triggerBirthdayCelebration()">🎉 Запустити шоу Дня Народження</button>
        </div>
        <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline)">
          <div style="font-size:11px;font-weight:600;color:var(--accent-cool);margin-bottom:4px">🤫 SILENT PUSH:</div>
          <div style="display:flex;gap:4px">
            <input type="text" id="silentMsgInput" placeholder="Тихий текст..." style="padding:5px">
            <button class="btn" style="padding:5px 8px;font-size:11px" onclick="sendSilentPush()">Надіслати</button>
          </div>
        </div>
      </div>

      <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline);margin-bottom:10px">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
          <span style="font-size:11px;font-weight:600;color:var(--text-secondary)">📅 РОЗКЛАД НАСТІ:</span>
          <button class="btn" style="padding:3px 8px;font-size:10px" onclick="loadNastyaSchedule()">🔄 Оновити</button>
        </div>
        <div id="scheduleContainer" class="mono" style="font-size:11px;color:var(--text-secondary);max-height:120px;overflow-y:auto">
          Натисніть «Оновити» для перегляду...
        </div>
      </div>

      <div style="background:var(--bg-surface);padding:10px;border-radius:8px;border:1px solid var(--border-hairline);margin-bottom:10px">
        <div style="font-size:11px;font-weight:600;color:var(--accent-cool);margin-bottom:6px">🛠️ ОБСЛУГОВУВАННЯ ШКАТУЛКИ:</div>
        <div style="display:grid;grid-template-columns:repeat(auto-fit, minmax(130px, 1fr));gap:6px;margin-bottom:6px">
          <button class="btn btn-danger" style="padding:6px;font-size:10px" onclick="systemAction('reboot')">🔄 Перезапуск ESP32</button>
          <button class="btn" style="padding:6px;font-size:10px" onclick="systemAction('clear_cache')">🧹 Очистити кеш</button>
          <button class="btn" style="padding:6px;font-size:10px" onclick="systemAction('start_ap')">📡 Wi-Fi AP</button>
        </div>
        <div id="boxDiagLine" class="mono" style="font-size:10px;color:var(--text-secondary)">
          Стан: <span id="diagStatus" style="color:var(--accent-success)">Підключено</span> | Перезапуск: <span id="diagRst" style="color:var(--accent-warm)">--</span> | Heap: <span id="diagHeap">--</span> KB, PSRAM: <span id="diagPsram">--</span> KB
        </div>
      </div>

      <div style="display:flex;gap:6px;flex-wrap:wrap">
        <button class="btn" style="font-size:11px;padding:6px 10px" onclick="triggerAdminBackup()">📦 Бекап БД в Telegram</button>
        <button class="btn" style="font-size:11px;padding:6px 10px" onclick="loadHealthReport()">🩺 Звіт здоров'я</button>
        <button class="btn btn-cool" style="font-size:11px;padding:6px 10px" onclick="triggerServerUpdate()">🔄 Оновити з GitHub</button>
        <button class="btn btn-danger" style="font-size:11px;padding:6px 10px" onclick="triggerSandboxReset()">🎁 Скинути тестові дані -> Настя</button>
      </div>
      <div id="healthReportBox" style="margin-top:8px;font-size:10px;font-family:var(--font-mono);background:var(--bg-primary);border:1px solid var(--border-hairline);padding:8px;border-radius:6px;display:none;white-space:pre-wrap"></div>
    </div>
  </div>
</div>

<!-- Bottom Navigation Dock for Telegram WebApp Mobile Thumbs -->
<nav class="bottom-dock">
  <button class="bnav-item active" id="bnav-music" onclick="switchTab('music')">
    <span class="bnav-icon">🎵</span>
    <span>Музика</span>
  </button>
  <button class="bnav-item" id="bnav-light" onclick="switchTab('light')">
    <span class="bnav-icon">💡</span>
    <span>Світло</span>
  </button>
  <button class="bnav-item" id="bnav-climate" onclick="switchTab('climate')">
    <span class="bnav-icon">📊</span>
    <span>Клімат</span>
  </button>
  <button class="bnav-item" id="bnav-routine" onclick="switchTab('routine')">
    <span class="bnav-icon">⏱</span>
    <span>Рутина</span>
  </button>
  <button class="bnav-item" id="bnav-system" onclick="switchTab('system')">
    <span class="bnav-icon">⚙️</span>
    <span>Система</span>
  </button>
</nav>

<div class="toast" id="toast">Виконано</div>

<script>
// Tab Switching System (Zero-Reload / Instant Response)
function switchTab(tabId) {
  const tabs = ['music', 'light', 'climate', 'routine', 'system'];
  tabs.forEach(t => {
    const view = document.getElementById('view-' + t);
    if (view) view.style.display = (t === tabId) ? 'block' : 'none';
    const topBtn = document.getElementById('tab-btn-' + t);
    if (topBtn) topBtn.classList.toggle('active', t === tabId);
    const botBtn = document.getElementById('bnav-' + t);
    if (botBtn) botBtn.classList.toggle('active', t === tabId);
  });
  localStorage.setItem('mb_active_tab', tabId);
  if (tabId === 'climate' && window.Plotly) {
    setTimeout(() => {
      try { Plotly.Plots.resize('climateChart'); } catch(_) {}
    }, 120);
  }
  if (window.Telegram && window.Telegram.WebApp && window.Telegram.WebApp.HapticFeedback) {
    try { window.Telegram.WebApp.HapticFeedback.impactOccurred('light'); } catch(_) {}
  }
}

const MODES = [
  "☀️ Теплий затишок", "🕯 Жива свічка", "🍯 Бурштинова свічка", "🔥 Камін з іскрами",
  "🌅 Захід сонця", "🌸 Сакура Неон", "🌌 Північне сяйво", "🌊 Океанський бриз",
  "⚡ Кібер-комета", "🌈 Веселка", "🌑 Вимкнено", "🎵 Spotify Ambi"
];

const mg = document.getElementById('modeGrid');
if (mg) {
  MODES.forEach((name, idx) => {
    const b = document.createElement('button');
    b.className = 'btn';
    b.id = 'mBtn' + idx;
    b.innerText = name;
    b.onclick = () => sendCmd({set_mode: idx});
    mg.appendChild(b);
  });
}

let currentMuted = false;
let currentTouchBeep = true;

function toast(msg){
  const t = document.getElementById('toast');
  t.innerText = msg;
  t.style.display = 'block';
  setTimeout(() => t.style.display = 'none', 2400);
}

function openHellcatModal(){
  const m = document.getElementById('hellcatModal');
  if(m) m.style.display = 'flex';
}
function closeHellcatModal(){
  const m = document.getElementById('hellcatModal');
  if(m) m.style.display = 'none';
}
function dismissHellcatWelcome(){
  localStorage.setItem('smartlamp_welcomed', 'true');
  closeHellcatModal();
}
function checkHellcatFirstLaunch(){
  if(!localStorage.getItem('smartlamp_welcomed')){
    openHellcatModal();
  }
}

function doLogin(){
  const p = document.getElementById('pwdInput').value;
  fetch('/api/v5/auth/login',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({password: p})
  }).then(r => {
    if(r.ok){
      document.getElementById('loginModal').style.display = 'none';
      pollState();
    } else {
      alert('Невірний пароль або PIN');
    }
  });
}

function doLogout(){
  fetch('/api/v5/auth/logout', {method:'POST'}).then(()=>{
    location.reload();
  });
}

function sendCmd(cmdObj){
  fetch('/api/v5/commands',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(cmdObj)
  }).then(r => r.json()).then(d => {
    toast('Команду передано в чергу');
  });
}

function pushScreenMsg(){
  const txt = document.getElementById('msgTxt').value.trim();
  if(!txt) return;
  sendCmd({react:1, sender:'Studio', msg: txt});
  document.getElementById('msgTxt').value = '';
}

function speakTts(){
  const txt = document.getElementById('msgTxt').value.trim();
  if(!txt) return;
  fetch(`/api/v1/audio/tts?text=${encodeURIComponent(txt)}`, {method:'POST'})
    .then(r => r.json())
    .then(d => {
      toast(d.ok ? 'Голос синтезовано!' : 'Помилка генерації аудіо');
    });
}

let volDebounce = null;
function onVolSlider(v){
  document.getElementById('volVal').innerText = `${v} / 21`;
}
function applyVolume(v){
  fetch('/api/v5/audio/volume', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({volume: parseInt(v)})
  }).then(r => r.json()).then(d => {
    toast(`Гучність: ${v}`);
  });
}

function toggleMute(){
  currentMuted = !currentMuted;
  updateMuteUI(currentMuted);
  fetch('/api/v5/audio/volume', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({muted: currentMuted})
  }).then(r => r.json()).then(d => {
    toast(currentMuted ? 'Звук вимкнено (Mute)' : 'Звук увімкнено');
  });
}

function updateMuteUI(m){
  const btn = document.getElementById('muteBtn');
  if(!btn) return;
  if(m){
    btn.innerText = '🔇 MUTE: ТАК';
    btn.className = 'btn btn-danger';
  } else {
    btn.innerText = '🔊 ЗВУК: УВІМК';
    btn.className = 'btn';
  }
}

function toggleTouchBeep(){
  currentTouchBeep = !currentTouchBeep;
  updateTouchBeepUI(currentTouchBeep);
  fetch('/api/v5/audio/volume', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({touch_beep: currentTouchBeep})
  }).then(r => r.json()).then(d => {
    toast(currentTouchBeep ? 'Кліки клавіш увімкнено' : 'Кліки клавіш вимкнено');
  });
}

function updateTouchBeepUI(tb){
  const btn = document.getElementById('touchBeepBtn');
  if(!btn) return;
  btn.innerText = tb ? '🔔 КЛІКИ: ТАК' : '🔕 КЛІКИ: НІ';
  btn.className = tb ? 'btn active' : 'btn';
}

function onProviderChange(){
  const p = document.getElementById('ttsProviderSelect').value;
  const cloneRow = document.getElementById('cloneVoiceRow');
  if(cloneRow) cloneRow.style.display = (p === 'elevenlabs' || p === 'fish_audio') ? 'block' : 'none';
}

function saveVoiceConfig(e){
  e.preventDefault();
  const provider = document.getElementById('ttsProviderSelect').value;
  const edge_voice = document.getElementById('edgeVoiceSelect').value;
  const clone_voice_id = document.getElementById('cloneVoiceIdInput').value.trim();
  fetch('/api/v5/audio/voice-config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({provider, edge_voice, clone_voice_id})
  }).then(r => r.json()).then(d => {
    toast('Конфігурацію голосу збережено');
  });
}

function renderVoicePresets(presets){
  const list = document.getElementById('voicePresetsList');
  if(!list) return;
  list.innerHTML = '';
  presets.forEach(p => {
    const b = document.createElement('button');
    b.className = 'btn';
    b.style.fontSize = '11px';
    b.style.padding = '4px 8px';
    b.innerText = p.title;
    b.onclick = () => {
      fetch(`/api/v1/audio/tts?text=${encodeURIComponent(p.text)}`, {method:'POST'})
        .then(r => r.json())
        .then(d => toast(`Озвучено: "${p.title}"`));
    };
    list.appendChild(b);
  });
}

function stopMedia(){
  sendCmd({stop_media: true});
  toast('Показ зупинено');
}

function setMediaTimer(minutes){
  sendCmd({media_timer_min: minutes});
  toast(minutes === 0 ? 'Таймер скинуто (безкінечно)' : `Таймер встановлено на ${minutes} хв`);
}

function uploadMedia(e){
  e.preventDefault();
  const fileInput = document.getElementById('mediaFile');
  const nameInput = document.getElementById('mediaName');
  const slotSelect = document.getElementById('mediaSlot');
  if(!fileInput.files.length) return;

  const file = fileInput.files[0];
  const name = nameInput.value.trim() || file.name.split('.')[0];
  const slot = parseInt(slotSelect.value);

  const fd = new FormData();
  fd.append('file', file);

  toast('Завантаження та конвертація у 136x136 Bayer 4x4...');
  fetch(`/api/v1/media/upload?name=${encodeURIComponent(name)}&slot=${slot}`, {
    method: 'POST',
    body: fd
  }).then(r => r.json()).then(d => {
    if(d.ok){
      toast('Успішно завантажено!');
      fileInput.value = '';
      nameInput.value = '';
      pollState();
    } else {
      alert('Помилка: ' + (d.error || 'невідома'));
    }
  });
}

function renderGallery(list){
  const g = document.getElementById('mediaGrid');
  if(!g) return;
  g.innerHTML = '';
  list.forEach(m => {
    const card = document.createElement('div');
    card.className = 'media-item';
    const rgbStr = `rgb(${m.ambi_rgb[0]},${m.ambi_rgb[1]},${m.ambi_rgb[2]})`;
    card.innerHTML = `
      <div>
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:4px">
          <b style="font-size:12px;color:var(--text-primary)">${m.name}</b>
          <span class="ambi-dot" style="background:${rgbStr}" title="Колір Ambi: ${rgbStr}"></span>
        </div>
        <div class="mono" style="font-size:10px;color:var(--text-muted)">
          Кадрів: ${m.frames} | ${m.fps} FPS<br>
          Слот: ${m.slot ? 'Кнопка ' + m.slot : 'Звичайний'}
        </div>
      </div>
      <div style="display:flex;gap:4px;flex-wrap:wrap;margin-top:6px">
        <button class="btn btn-cool" style="padding:4px 8px;font-size:10px;flex:1" onclick="triggerMedia('${m.name}')">Запустити</button>
        <button class="btn" style="padding:4px 8px;font-size:10px" onclick="setAsSaver('${m.name}')">На заставку</button>
        <button class="btn btn-danger" style="padding:4px 6px;font-size:10px" onclick="deleteMedia('${m.name}')">✕</button>
      </div>
    `;
    g.appendChild(card);
  });
}

function triggerMedia(name){
  sendCmd({play_media: name});
  toast(`Запущено медіа: ${name}`);
}

function setAsSaver(name){
  sendCmd({set_screensaver_media: name});
  toast(`Встановлено як заставку: ${name}`);
}

function spotifyAction(act){
  if (act === 'toggle') {
    act = (spotifyPlayback && spotifyPlayback.playing) ? 'pause' : 'play';
  }
  if (act === 'pause') {
    spotifyPlayback.playing = false;
    const spBadgeEl = document.getElementById('spBadge');
    const spPlayBtn = document.getElementById('spPlayBtn');
    const spDisc = document.getElementById('spDisc');
    if (spBadgeEl) { spBadgeEl.innerText = '⏸ PAUSED'; spBadgeEl.style.color = 'var(--text-secondary)'; }
    if (spPlayBtn) spPlayBtn.innerText = '▶️ Пуск';
    if (spDisc) spDisc.classList.remove('spinning');
    tickSpotifyProgress();
  } else if (act === 'play') {
    spotifyPlayback.playing = true;
    spotifyPlayback.last_sync_ts = Date.now();
    const spBadgeEl = document.getElementById('spBadge');
    const spPlayBtn = document.getElementById('spPlayBtn');
    const spDisc = document.getElementById('spDisc');
    if (spBadgeEl) { spBadgeEl.innerText = '▶️ PLAYING'; spBadgeEl.style.color = 'var(--accent-success)'; }
    if (spPlayBtn) spPlayBtn.innerText = '⏸ Пауза';
    if (spDisc) spDisc.classList.add('spinning');
    tickSpotifyProgress();
  } else if (act === 'next' || act === 'previous') {
    spotifyPlayback.progress_ms = 0;
    spotifyPlayback.last_sync_ts = Date.now();
    const spBarEl = document.getElementById('spProgressBar');
    if (spBarEl) spBarEl.style.width = '0%';
    tickSpotifyProgress();
  }

  fetch('/api/v5/spotify/action', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({action: act})
  }).then(r => r.json()).then(res => {
    toast(`Spotify: ${act}`);
    if (res && res.state) {
      updateSpotifyUI(res.state);
    }
    setTimeout(() => pollState(true), 500);
  });
}

function loginSpotify(){
  fetch('/api/v5/spotify/login')
    .then(r => r.json())
    .then(d => {
      const authUrl = d.auth_url || d.url;
      if(authUrl) {
        if(window.Telegram && window.Telegram.WebApp && window.Telegram.WebApp.openLink) {
          window.Telegram.WebApp.openLink(authUrl);
        } else {
          window.open(authUrl, '_blank');
        }
      } else {
        alert(d.error || 'Ключі Spotify ще не налаштовані');
      }
    })
    .catch(() => alert('Не вдалося отримати URL авторизації'));
}

function disconnectSpotify(){
  if(!confirm('Відключити Spotify акаунт від лампи?')) return;
  fetch('/api/v5/spotify/disconnect', {method:'POST'})
    .then(r => r.json())
    .then(() => {
      toast('Spotify відключено');
      pollState();
    });
}

let spotifyPlayback = {
  linked: false,
  playing: false,
  title: '',
  artist: '',
  progress_ms: 0,
  duration_ms: 0,
  last_sync_ts: 0
};

function tickSpotifyProgress() {
  const spArtistEl = document.getElementById('spArtist');
  const spBarEl = document.getElementById('spProgressBar');
  if (!spArtistEl) return;

  if (!spotifyPlayback.linked || !spotifyPlayback.title) {
    if (spBarEl) spBarEl.style.width = '0%';
    return;
  }

  let currentMs = spotifyPlayback.progress_ms;
  if (spotifyPlayback.playing && spotifyPlayback.last_sync_ts > 0) {
    const elapsed = Date.now() - spotifyPlayback.last_sync_ts;
    currentMs = Math.min(spotifyPlayback.duration_ms || (currentMs + elapsed), spotifyPlayback.progress_ms + elapsed);
  }

  const prog_m = Math.floor(currentMs / 60000);
  const prog_s = ('0' + Math.floor((currentMs % 60000) / 1000)).slice(-2);
  const dur_m = Math.floor((spotifyPlayback.duration_ms || 0) / 60000);
  const dur_s = ('0' + Math.floor(((spotifyPlayback.duration_ms || 0) % 60000) / 1000)).slice(-2);
  spArtistEl.innerText = `${spotifyPlayback.artist || 'Unknown'} · ${prog_m}:${prog_s} / ${dur_m}:${dur_s}`;

  if (spBarEl && spotifyPlayback.duration_ms > 0) {
    const pct = Math.min(100, Math.max(0, (currentMs / spotifyPlayback.duration_ms) * 100));
    spBarEl.style.width = pct.toFixed(1) + '%';
  }

  if (spotifyPlayback.duration_ms > 0 && currentMs >= spotifyPlayback.duration_ms && spotifyPlayback.playing) {
    if (!window._songEndTimer) {
      window._songEndTimer = setTimeout(() => {
        pollState(true);
        window._songEndTimer = null;
      }, 700);
    }
  }
}
setInterval(tickSpotifyProgress, 500);

function updateSpotifyUI(sp) {
  if (!sp) return;
  const spTitleEl = document.getElementById('spTitle');
  const spArtistEl = document.getElementById('spArtist');
  const spBadgeEl = document.getElementById('spBadge');
  const spPlayBtn = document.getElementById('spPlayBtn');
  const spLoginBtn = document.getElementById('spLoginBtn');
  const spDisconnectBtn = document.getElementById('spDisconnectBtn');
  const spBarEl = document.getElementById('spProgressBar');
  const spDisc = document.getElementById('spDisc');

  if (sp.linked) {
    if (spLoginBtn) spLoginBtn.style.display = 'none';
    if (spDisconnectBtn) spDisconnectBtn.style.display = 'inline-flex';

    const isNewTrack = Boolean(sp.track_id && sp.track_id !== spotifyPlayback.track_id);
    spotifyPlayback.linked = true;
    spotifyPlayback.playing = Boolean(sp.playing);
    spotifyPlayback.title = sp.title || '';
    spotifyPlayback.artist = sp.artist || '';
    spotifyPlayback.duration_ms = sp.duration_ms || 0;
    spotifyPlayback.track_id = sp.track_id || '';

    let estimatedLocalMs = spotifyPlayback.progress_ms;
    if (spotifyPlayback.playing && spotifyPlayback.last_sync_ts > 0) {
      estimatedLocalMs += (Date.now() - spotifyPlayback.last_sync_ts);
    }
    if (isNewTrack || Math.abs(estimatedLocalMs - (sp.progress_ms || 0)) > 2500 || !spotifyPlayback.last_sync_ts) {
      spotifyPlayback.progress_ms = sp.progress_ms || 0;
      spotifyPlayback.last_sync_ts = Date.now();
    }

    if (spDisc) {
      if (sp.playing) spDisc.classList.add('spinning');
      else spDisc.classList.remove('spinning');
    }

    if (sp.title) {
      if (spTitleEl) spTitleEl.innerText = sp.title;
      if (spBadgeEl) {
        spBadgeEl.innerText = sp.playing ? '▶️ PLAYING' : '⏸ PAUSED';
        spBadgeEl.style.color = sp.playing ? 'var(--accent-success)' : 'var(--text-secondary)';
      }
      if (spPlayBtn) spPlayBtn.innerText = sp.playing ? '⏸ Пауза' : '▶️ Пуск';
      tickSpotifyProgress();
    } else {
      if (spTitleEl) spTitleEl.innerText = 'Spotify підключено ✅';
      if (spArtistEl) spArtistEl.innerText = 'Увімкніть трек у додатку Spotify на телефоні чи ПК';
      if (spBadgeEl) {
        spBadgeEl.innerText = '⏸ ОЧІКУВАННЯ ТРЕКУ';
        spBadgeEl.style.color = 'var(--accent-cool)';
      }
      if (spPlayBtn) spPlayBtn.innerText = '▶️ Пуск';
      if (spBarEl) spBarEl.style.width = '0%';
    }
  } else {
    spotifyPlayback.linked = false;
    spotifyPlayback.title = '';
    if (spDisc) spDisc.classList.remove('spinning');
    if (spTitleEl) spTitleEl.innerText = 'Немає треку';
    if (spArtistEl) spArtistEl.innerText = 'Spotify не підключено';
    if (spBadgeEl) {
      spBadgeEl.innerText = 'НЕАКТИВНИЙ';
      spBadgeEl.style.color = 'var(--text-muted)';
    }
    if (spLoginBtn) spLoginBtn.style.display = 'inline-flex';
    if (spDisconnectBtn) spDisconnectBtn.style.display = 'none';
    if (spBarEl) spBarEl.style.width = '0%';
  }
}

function triggerSandboxReset(){
  if(!confirm('Ви дійсно хочете виконати передподарункове скидання?\n\n• Spotify та Google Calendar буде відключено\n• Тестові події, клімат та історію буде видалено\n• Власницю буде закріплено за Настею (1294097414)')) return;
  toast('Виконую скидання...');
  fetch('/api/v5/admin/sandbox-reset', {method:'POST'})
    .then(r => r.json())
    .then(d => {
      toast(d.message || 'Скидання завершено');
      pollState();
    });
}

function triggerServerUpdate(){
  if(!confirm('Перевірити наявність оновлень на GitHub та оновити сервер?')) return;
  toast('Перевіряю GitHub...');
  fetch('/api/v5/admin/server-update/status')
    .then(r => r.json())
    .then(st => {
      if(!st.ok){
        alert('Помилка перевірки оновлень: ' + (st.error || 'невідома'));
        return;
      }
      if(!st.update_available){
        alert(`Сервер уже має найновішу версію!\nПоточний коміт: ${st.current_commit} (${st.current_message})`);
        return;
      }
      if(confirm(`Знайдено нову версію на GitHub!\n\nКоміт: ${st.remote_commit}\nОпис: ${st.remote_message}\n\nВиконати оновлення прямо зараз?`)){
        toast('Оновлення сервера (git pull + restart)...');
        fetch('/api/v5/admin/server-update/apply', {method:'POST'})
          .then(r => r.json())
          .then(res => {
            if(res.ok){
              alert('Оновлення успішно застосовано! Сторінка перезавантажиться за кілька секунд.');
              setTimeout(() => location.reload(), 4000);
            } else {
              alert('Помилка застосування: ' + (res.error || 'невідома'));
            }
          });
      }
    });
}

function deleteMedia(name){
  if(!confirm(`Видалити медіа "${name}"?`)) return;
  fetch(`/api/v1/media/delete?name=${encodeURIComponent(name)}`, {method:'DELETE'})
    .then(r => r.json())
    .then(d => {
      toast('Видалено');
      pollState();
    });
}

function saveAiPersona(e){
  e.preventDefault();
  const name = document.getElementById('aiName').value.trim();
  const prompt = document.getElementById('aiPrompt').value.trim();
  fetch('/api/v5/ai/persona', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({name, prompt, temperature: 0.7})
  }).then(r => r.json()).then(d => {
    toast('Характер ШІ оновлено');
  });
}

function askAiBriefing(){
  toast('Генерую ШІ-брифінг...');
  fetch('/api/v5/ai/ask', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({
      prompt: "Розкажи короткий затишний ранковий брифінг українською мовою для Насті: погода, час, приємне побажання на день."
    })
  }).then(r => r.json()).then(d => {
    const box = document.getElementById('aiResponseBox');
    if(box){
      box.innerText = d.reply || 'Немає відповіді від ШІ';
      box.style.display = 'block';
    }
  });
}

function toggleQuietHours() {
  const btn = document.getElementById('qhToggleBtn');
  const isEn = btn.innerText.includes('УВІМКНЕНО');
  saveQuietHoursConfig(!isEn);
}
function saveQuietHours() {
  saveQuietHoursConfig(true);
}
function saveQuietHoursConfig(enabled) {
  const start = document.getElementById('qhStart').value;
  const end = document.getElementById('qhEnd').value;
  fetch('/api/v5/routine/quiet-hours', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled, start, end })
  }).then(r => r.json()).then(d => {
    updateQuietHoursUI(d.enabled, d.start, d.end);
    toast(d.enabled ? 'Тихі години увімкнено' : 'Тихі години вимкнено');
  });
}
function updateQuietHoursUI(enabled, start, end) {
  const btn = document.getElementById('qhToggleBtn');
  if (btn) {
    btn.innerText = enabled ? '🌙 УВІМКНЕНО' : 'ВИМКНЕНО';
    btn.className = enabled ? 'btn active' : 'btn';
  }
  if (start) document.getElementById('qhStart').value = start;
  if (end) document.getElementById('qhEnd').value = end;
}

function toggleAlarm() {
  const btn = document.getElementById('alarmToggleBtn');
  const isEn = btn.innerText.includes('УВІМКНЕНО');
  saveAlarmConfig(!isEn);
}
function saveAlarm() {
  saveAlarmConfig(true);
}
function saveAlarmConfig(enabled) {
  const timeVal = document.getElementById('alarmTime').value;
  fetch('/api/v5/routine/alarm', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled, time: timeVal })
  }).then(r => r.json()).then(d => {
    updateAlarmUI(d.enabled, d.time);
    toast(d.enabled ? `Світанок встановлено на ${d.time}` : 'Світанок вимкнено');
  });
}
function updateAlarmUI(enabled, timeVal) {
  const btn = document.getElementById('alarmToggleBtn');
  if (btn) {
    btn.innerText = enabled ? '🌅 УВІМКНЕНО' : 'ВИМКНЕНО';
    btn.className = enabled ? 'btn active' : 'btn';
  }
  if (timeVal) document.getElementById('alarmTime').value = timeVal;
}

function triggerMorningBrief() {
  toast('Готую ранкову пораду...');
  fetch('/api/v5/routine/morning-brief', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      const box = document.getElementById('morningBriefBox');
      if (box) {
        box.innerText = d.advice || 'Вдалого та світлого дня!';
        box.style.display = 'block';
      }
      toast('Пораду озвучено шкатулкою!');
    });
}

function adjustPomoInput(delta) {
  const el = document.getElementById('pomoInput');
  let val = (parseInt(el.value) || 25) + delta;
  if (val < 5) val = 5;
  if (val > 120) val = 120;
  el.value = val;
}

function startPomodoro(min) {
  fetch('/api/v5/routine/pomodoro/start', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ duration_min: min })
  }).then(r => r.json()).then(d => {
    toast(`Pomodoro: фокус на ${min} хв!`);
    updatePomodoroUI(d);
  });
}

function stopPomodoro() {
  fetch('/api/v5/routine/pomodoro/stop', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast('Pomodoro зупинено');
      updatePomodoroUI(d);
    });
}

function triggerBirthdayCelebration() {
  if (!confirm('Запустити святкову листівку на шкатулці та привітання Насті в Telegram?')) return;
  toast('Запуск привітання...');
  fetch('/api/v5/admin/nastya/birthday-celebration', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      alert(d.message || 'Свято активовано!');
    });
}

function sendSilentPush() {
  const input = document.getElementById('silentMsgInput');
  const txt = input.value.trim();
  if (!txt) return;
  fetch('/api/v5/admin/nastya/silent-push', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ message: txt })
  }).then(r => r.json()).then(d => {
    toast('Тихе повідомлення надіслано в Inbox');
    input.value = '';
  });
}

function loadNastyaSchedule() {
  const cont = document.getElementById('scheduleContainer');
  if (!cont) return;
  cont.innerHTML = 'Завантаження розкладу Насті...';
  fetch('/api/v5/admin/nastya/calendar-view')
    .then(r => r.json())
    .then(d => {
      if (!d.ok || !d.events || d.events.length === 0) {
        cont.innerHTML = 'Немає запланованих подій у найближчі 48 годин.';
        return;
      }
      let html = '<ul style="padding-left:16px;margin:0">';
      d.events.forEach(ev => {
        html += `<li style="margin-bottom:4px"><b>${ev.time || ''}</b>: ${ev.summary || 'Без назви'}</li>`;
      });
      html += '</ul>';
      cont.innerHTML = html;
    })
    .catch(() => { cont.innerHTML = 'Помилка завантаження розкладу'; });
}

function triggerAdminBackup() {
  toast('Створення бекапу SQLite WAL...');
  fetch('/api/v5/admin/backup-telegram', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      if (d.ok) toast('Бекап надіслано архітектору в Telegram!');
      else alert('Помилка бекапу: ' + (d.error || 'невідома'));
    });
}

function loadHealthReport() {
  const box = document.getElementById('healthReportBox');
  if (!box) return;
  box.style.display = 'block';
  box.innerText = 'Опитування модулів системи...';
  fetch('/api/v5/admin/health-report')
    .then(r => r.json())
    .then(d => {
      box.innerText = JSON.stringify(d, null, 2);
    })
    .catch(e => { box.innerText = 'Помилка запиту: ' + e; });
}

function updatePomodoroUI(pomo) {
  if (!pomo) return;
  const statusEl = document.getElementById('pomoStatus');
  const timerEl = document.getElementById('pomoTimer');
  if (statusEl) {
    if (pomo.active) {
      statusEl.innerText = pomo.is_break ? 'ПЕРЕРВА ☕' : 'ФОКУС 📚';
      statusEl.style.color = pomo.is_break ? 'var(--accent-success)' : 'var(--accent-warm)';
    } else {
      statusEl.innerText = 'НЕАКТИВНИЙ';
      statusEl.style.color = 'var(--text-muted)';
    }
  }
  if (timerEl) {
    const sec = pomo.remaining_sec || 0;
    const m = Math.floor(sec / 60);
    const s = sec % 60;
    timerEl.innerText = `${m}:${s < 10 ? '0' : ''}${s}`;
  }
}

function loginGoogle() {
  fetch('/api/v5/calendar/login')
    .then(r => r.json())
    .then(d => {
      const authUrl = d.auth_url || d.url;
      if (authUrl) {
        if (window.Telegram && window.Telegram.WebApp && window.Telegram.WebApp.openLink) {
          window.Telegram.WebApp.openLink(authUrl);
        } else {
          window.open(authUrl, '_blank');
        }
      } else {
        const icalUrl = prompt('Ключі Google OAuth2 ще не налаштовані.\n\nВведіть публічне посилання iCal (.ics) вашого календаря для швидкої синхронізації:');
        if (icalUrl) {
          fetch('/api/v5/calendar/ical', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ url: icalUrl })
          }).then(r => r.json()).then(res => {
            if (res.ok) {
              toast(`iCal підключено! Завантажено ${res.events_count} подій`);
              pollState();
            } else {
              alert('Помилка імпорту iCal: ' + (res.detail || 'перевірте URL'));
            }
          });
        }
      }
    })
    .catch(() => alert('Не вдалося отримати URL авторизації Google Calendar'));
}

function disconnectGoogle() {
  if (!confirm('Відключити Google Календар від лампи?')) return;
  fetch('/api/v5/calendar/disconnect', { method: 'POST' })
    .then(r => r.json())
    .then(() => {
      toast('Google Календар відключено');
      pollState();
    });
}

function updateGoogleUI(linked, nextEvt) {
  const badge = document.getElementById('calStatusBadge');
  const loginBtn = document.getElementById('calLoginBtn');
  const discBtn = document.getElementById('calDisconnectBtn');
  const nextEl = document.getElementById('calNextEvent');

  if (badge) {
    badge.innerText = linked ? 'ПІДКЛЮЧЕНО ✅' : 'НЕ ПІДКЛЮЧЕНО';
    badge.style.color = linked ? 'var(--accent-success)' : 'var(--text-muted)';
  }
  if (loginBtn) loginBtn.style.display = linked ? 'none' : 'inline-flex';
  if (discBtn) discBtn.style.display = linked ? 'inline-flex' : 'none';

  if (nextEl) {
    if (nextEvt && nextEvt.summary) {
      nextEl.innerText = `📅 Найближча: ${nextEvt.summary} (${nextEvt.start || '--:--'})`;
      nextEl.style.color = 'var(--accent-cool)';
    } else {
      nextEl.innerText = linked ? '📅 Немає подій на сьогодні' : '📅 Календар не синхронізовано';
      nextEl.style.color = 'var(--text-muted)';
    }
  }
}

function toggleDnd() {
  const btn = document.getElementById('dndTopBtn');
  const isDnd = btn.innerText.includes('ВКЛ');
  sendCmd({ set_dnd: !isDnd });
  updateDndUI(!isDnd);
  toast(!isDnd ? 'Режим "Не турбувати" увімкнено' : 'Режим "Не турбувати" вимкнено');
}

function updateDndUI(isDnd) {
  const btn = document.getElementById('dndTopBtn');
  if (!btn) return;
  btn.innerText = isDnd ? '🌙 DND: ВКЛ' : '🌙 DND: ВИКЛ';
  btn.className = isDnd ? 'btn btn-danger' : 'btn';
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/[&<>"']/g, function(m) {
    return {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&#039;'
    }[m];
  });
}

function loadCalendarEvents() {
  const listEl = document.getElementById('calEventList');
  if (!listEl) return;
  fetch('/api/v5/calendar/events')
    .then(r => r.json())
    .then(d => {
      if (!d.ok || !d.events || d.events.length === 0) {
        listEl.innerHTML = '<div style="color:var(--text-muted);padding:4px 0">Немає запланованих подій.</div>';
        return;
      }
      let html = '';
      d.events.forEach(ev => {
        const timeStr = ev.start || 'Цілий день';
        const isPast = ev.is_past ? 'opacity:0.45;' : '';
        html += `
          <div style="display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px solid var(--border-hairline);${isPast}">
            <div>
              <b class="mono" style="color:var(--accent-cool);margin-right:6px">${escapeHtml(timeStr)}</b>
              <span>${escapeHtml(ev.summary || 'Без назви')}</span>
            </div>
            <button class="btn btn-danger" style="padding:2px 6px;font-size:10px" onclick="deleteCalendarEvent('${ev.id}')">✕</button>
          </div>
        `;
      });
      listEl.innerHTML = html;
    })
    .catch(() => {
      listEl.innerHTML = '<div style="color:var(--accent-danger);padding:4px 0">Помилка завантаження подій.</div>';
    });
}

function addCalendarEvent() {
  const timeInput = document.getElementById('calTimeInput');
  const sumInput = document.getElementById('calSummaryInput');
  const summary = sumInput.value.trim();
  const timeVal = timeInput.value.trim();
  if (!summary) {
    alert('Введіть назву події');
    return;
  }
  fetch('/api/v5/calendar/events', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ summary, time: timeVal, date: 'today' })
  }).then(r => r.json()).then(d => {
    if (d.ok) {
      toast('Подію додано!');
      sumInput.value = '';
      timeInput.value = '';
      loadCalendarEvents();
      pollState();
    } else {
      alert('Помилка: ' + (d.detail || 'невідома'));
    }
  });
}

function deleteCalendarEvent(id) {
  if (!confirm('Видалити цю подію?')) return;
  fetch(`/api/v5/calendar/events/${id}`, { method: 'DELETE' })
    .then(r => r.json())
    .then(d => {
      toast('Подію видалено');
      loadCalendarEvents();
      pollState();
    });
}

function systemAction(action) {
  const msgs = {
    reboot: 'Перезавантажити контролер ESP32-S3 шкатулки?',
    clear_cache: 'Очистити кеш поточної заставки та перезавантажити графіку?',
    start_ap: 'Перевести шкатулку в режим тимчасової точки доступу Wi-Fi (AP Config)?'
  };
  if (!confirm(msgs[action] || 'Виконати системну дію?')) return;
  fetch('/api/v5/system/action', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: action })
  }).then(r => r.json()).then(d => {
    toast(d.message || 'Команду надіслано до ESP32');
  });
}

let personaLoaded = false;
let configLoaded = false;
let pollTickCount = 0;
function pollState(forceFull = false){
  pollTickCount++;
  const isFull = forceFull || (pollTickCount % 10 === 1);
  const url = isFull ? '/api/v5/state?hours=24' : '/api/v5/state?fast=true';
  fetch(url).then(r => {
    if(r.status === 401) {
      document.getElementById('loginModal').style.display = 'flex';
      return null;
    }
    return r.json();
  }).then(d => {
    if(!d) return;

    const on = d.is_online;
    document.getElementById('onlineDot').className = 'pulse ' + (on ? 'on' : 'off');
    document.getElementById('onlineTxt').innerText = on ? 'MAGICBOX SOVEREIGN' : 'MAGICBOX OFFLINE';
    document.getElementById('queueTxt').innerText = d.pending_commands;

    const res = d.resources || {};
    document.getElementById('ramTxt').innerText = (res.ram_rss_mb !== undefined ? res.ram_rss_mb : (res.rss_mb || 0)).toFixed(1) + ' MB';
    document.getElementById('diskTxt').innerText = (res.storage_used_mb !== undefined ? res.storage_used_mb : (res.disk_used_mb || 0)).toFixed(1) + ' MB';

    // Spotify Hub Update
    updateSpotifyUI(d.spotify || {});

    const t = d.telemetry || {};
    if(t.t !== undefined){
      document.getElementById('fwBadge').innerText = `${t.fw || 'v5.0'} (${t.rssi || -60} dBm)`;
      const tStr = t.t.toFixed(1) + ' °C';
      const hStr = t.h.toFixed(0) + ' %';
      const pStr = `${t.p.toFixed(0)} мм (${t.p_diff>=0?'+':''}${(t.p_diff||0).toFixed(1)})`;
      const mStr = `${t.m_name || '--'} (${t.b_pct || 0}%)`;

      document.getElementById('kTemp').innerText = tStr;
      document.getElementById('kHum').innerText = hStr;
      document.getElementById('kPres').innerText = pStr;
      document.getElementById('kMode').innerText = mStr;

      const ktc = document.getElementById('kTempCard');
      const khc = document.getElementById('kHumCard');
      const kpc = document.getElementById('kPresCard');
      if (ktc) ktc.innerText = tStr;
      if (khc) khc.innerText = hStr;
      if (kpc) kpc.innerText = pStr;

      MODES.forEach((_,i)=>{
        const el = document.getElementById('mBtn'+i);
        if(el) el.className = (t.m === i) ? 'btn active' : 'btn';
      });

      if(t.vol !== undefined && !document.getElementById('volSlider').matches(':active')){
        document.getElementById('volSlider').value = t.vol;
        document.getElementById('volVal').innerText = `${t.vol} / 21`;
      }
      if(t.muted !== undefined){
        currentMuted = Boolean(t.muted);
        updateMuteUI(currentMuted);
      }
      if(t.t_beep !== undefined){
        currentTouchBeep = Boolean(t.t_beep);
        updateTouchBeepUI(currentTouchBeep);
      }
      if(t.dnd !== undefined){
        updateDndUI(Boolean(t.dnd));
      }
      if(t.heap !== undefined){
        const dh = document.getElementById('diagHeap');
        if(dh) dh.innerText = Math.round(t.heap / 1024);
        const dp = document.getElementById('diagPsram');
        if(dp) dp.innerText = Math.round((t.psram_free || 0) / 1024);
        const dr = document.getElementById('diagRst');
        if(dr) dr.innerText = t.rst || 'NORMAL';
      }
    }

    if(!personaLoaded && d.ai_persona){
      personaLoaded = true;
      document.getElementById('aiName').value = d.ai_persona.name;
      document.getElementById('aiPrompt').value = d.ai_persona.prompt;
    }

    if(!configLoaded && d.tts_config){
      configLoaded = true;
      document.getElementById('ttsProviderSelect').value = d.tts_config.provider || 'edge-tts';
      document.getElementById('edgeVoiceSelect').value = d.tts_config.edge_voice || 'uk-UA-OstapNeural';
      if(d.tts_config.clone_voice_id){
        document.getElementById('cloneVoiceIdInput').value = d.tts_config.clone_voice_id;
      }
      onProviderChange();
    }

    if(d.custom_media) renderGallery(d.custom_media);
    if(d.voice_presets) renderVoicePresets(d.voice_presets);

    if(d.quiet_hours){
      updateQuietHoursUI(d.quiet_hours.enabled, d.quiet_hours.start, d.quiet_hours.end);
    }
    if(d.alarm){
      updateAlarmUI(d.alarm.enabled, d.alarm.time);
    }
    if(d.pomodoro){
      updatePomodoroUI(d.pomodoro);
    }
    updateGoogleUI(Boolean(d.google_calendar_linked), d.next_calendar_event);

    if (d.user_role === 'admin') {
      const ap = document.getElementById('adminPanel');
      if (ap) ap.style.display = 'block';
      const rb = document.getElementById('roleBadge');
      if (rb) { rb.innerText = '🛡️ ЯРОСЛАВ'; rb.style.color = 'var(--accent-cool)'; }
    } else {
      const ap = document.getElementById('adminPanel');
      if (ap) ap.style.display = 'none';
      const rb = document.getElementById('roleBadge');
      if (rb) { rb.innerText = '🌸 НАСТЯ'; rb.style.color = 'var(--accent-pink)'; }
    }

    const ch = d.climate_history || [];
    if(ch.length > 1){
      const times = ch.map(x => new Date(x.ts*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}));
      Plotly.react('climateChart',[
        {x:times, y:ch.map(x=>x.t), name:'Темп (°C)', type:'scatter', mode:'lines', line:{color:'#38bdf8',width:2.5}},
        {x:times, y:ch.map(x=>x.p), name:'Барометр (мм)', type:'scatter', mode:'lines', line:{color:'#f59e0b',width:2,dash:'dot'}, yaxis:'y2'}
      ],{
        paper_bgcolor:'rgba(0,0,0,0)', plot_bgcolor:'rgba(7,9,14,0.6)',
        font:{color:'#94a3b8', family:'JetBrains Mono', size:10},
        margin:{l:35,r:35,t:10,b:25}, legend:{orientation:'h',y:1.15},
        xaxis:{gridcolor:'#1e2536'},
        yaxis:{gridcolor:'#1e2536'},
        yaxis2:{overlaying:'y', side:'right', gridcolor:'rgba(0,0,0,0)'}
      },{responsive:true, displayModeBar:false});
    }
  }).catch(()=>{});
}

// Telegram Mini App Auto-Auth
if (window.Telegram && window.Telegram.WebApp && window.Telegram.WebApp.initData) {
  try {
    window.Telegram.WebApp.ready();
    window.Telegram.WebApp.expand();
    fetch('/api/v5/auth/tg-webapp', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ init_data: window.Telegram.WebApp.initData })
    })
    .then(r => r.json())
    .then(d => {
      if (d && d.ok) {
        const lm = document.getElementById('loginModal');
        if (lm) lm.style.display = 'none';
        pollState(true);
        loadCalendarEvents();
      }
    })
    .catch(() => {});
  } catch (_) {}
}

// Restore saved tab or default to 'music'
const savedTab = localStorage.getItem('mb_active_tab') || 'music';
switchTab(savedTab);

pollState();
loadCalendarEvents();
setInterval(loadCalendarEvents, 30000);

function scheduleAdaptivePoll(){
  const delay = (spotifyPlayback && spotifyPlayback.playing && spotifyPlayback.linked) ? 2000 : 7000;
  setTimeout(() => {
    pollState();
    scheduleAdaptivePoll();
  }, delay);
}
scheduleAdaptivePoll();

// SSE Stream
if (window.EventSource) {
  try {
    const sse = new EventSource('/api/v5/stream');
    sse.addEventListener('telemetry', () => { pollState(false); });
    sse.addEventListener('command', () => { pollState(false); });
    sse.addEventListener('state', () => { pollState(false); });
    sse.addEventListener('spotify', (e) => {
      try {
        const sp = JSON.parse(e.data);
        updateSpotifyUI(sp);
      } catch (_) {}
    });
  } catch (_) {}
}
</script>
</body>
</html>
"""
