# Файл: oracle_server/app/web/portal_html.py
PORTAL_HTML: str = r"""<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MagicBox Sovereign Studio • Hallmark v5.0 (Nastya Edition)</title>
<!-- Hallmark · pre-emit critique: P5 H5 E5 S5 R5 V5 -->
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600;700&family=Plus+Jakarta+Sans:wght@400;500;600;700&display=swap" rel="stylesheet">
<script src="https://cdn.plot.ly/plotly-2.35.2.min.js"></script>
<style>
:root {
  --bg-primary: #08090c;
  --bg-elevated: #12151e;
  --bg-surface: #181d29;
  --border-hairline: #222834;
  --border-focus: #4ea8de;
  --text-primary: #f4f3ef;
  --text-secondary: #8a94a6;
  --text-muted: #5a6478;
  --accent-warm: #e07a5f;
  --accent-cool: #4ea8de;
  --accent-success: #52b788;
  --accent-danger: #e63946;
  --font-sans: 'Plus Jakarta Sans', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  --font-mono: 'JetBrains Mono', ui-monospace, Menlo, Monaco, Consolas, monospace;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg-primary);
  color: var(--text-primary);
  font-family: var(--font-sans);
  min-height: 100vh;
  padding: 16px;
  -webkit-font-smoothing: antialiased;
}
.mono {
  font-family: var(--font-mono);
  font-variant-numeric: tabular-nums;
}
.wrap {
  max-width: 1400px;
  margin: 0 auto;
}
.top-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 14px 20px;
  margin-bottom: 16px;
}
.brand {
  display: flex;
  align-items: center;
  gap: 12px;
}
.logo {
  width: 38px;
  height: 38px;
  border-radius: 8px;
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
.pulse.on { background: var(--accent-success); }
.pulse.off { background: var(--accent-danger); }

.grid-4 {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
  margin-bottom: 16px;
}
@media(max-width:960px){.grid-4{grid-template-columns:repeat(2,1fr)}}

.kpi {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 10px;
  padding: 14px 18px;
}
.kpi span {
  font-size: 11px;
  font-weight: 600;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.kpi b {
  display: block;
  font-size: 24px;
  font-family: var(--font-mono);
  font-variant-numeric: tabular-nums;
  margin-top: 4px;
  color: var(--text-primary);
}

.split-2 {
  display: grid;
  grid-template-columns: 1.25fr 1fr;
  gap: 16px;
  margin-bottom: 16px;
}
@media(max-width:1080px){.split-2{grid-template-columns:1fr}}

.card {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 18px;
}
.card h3 {
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.75px;
  color: var(--text-secondary);
  margin-bottom: 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.mode-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  margin-bottom: 12px;
}
@media(max-width:600px){.mode-grid{grid-template-columns:repeat(2,1fr)}}

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
}
.btn:hover {
  border-color: var(--text-secondary);
  color: var(--text-primary);
}
.btn.active {
  background: var(--bg-primary);
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

.media-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(170px, 1fr));
  gap: 10px;
  max-height: 320px;
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

#loginModal {
  position: fixed;
  inset: 0;
  background: rgba(8,9,12,0.92);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 999;
}
.login-box {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 28px;
  width: 350px;
  text-align: center;
}
.toast {
  position: fixed;
  bottom: 20px;
  right: 20px;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  color: var(--accent-success);
  padding: 10px 18px;
  border-radius: 8px;
  font-family: var(--font-mono);
  font-size: 12px;
  font-weight: 600;
  display: none;
  z-index: 1000;
  box-shadow: 0 8px 24px rgba(0,0,0,0.5);
}
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
</style>
</head>
<body>

<div id="loginModal">
  <div class="login-box">
    <div style="font-size:32px;margin-bottom:8px">🪄</div>
    <h2 style="font-size:16px;margin-bottom:6px">MagicBox Studio • Hallmark v5.0</h2>
    <p style="font-size:12px;color:var(--text-secondary);margin-bottom:16px">Введіть PIN (0511) або пароль адміністратора</p>
    <input type="password" id="pwdInput" placeholder="PIN (0511)...">
    <button class="btn btn-cool" style="width:100%;margin-top:10px" onclick="doLogin()">Увійти</button>
  </div>
</div>

<div class="wrap">
  <div class="top-bar">
    <div class="brand">
      <div class="logo">🪄</div>
      <div>
        <div style="display:flex;align-items:center;gap:8px">
          <span id="onlineDot" class="pulse on"></span>
          <b id="onlineTxt" style="font-size:14px;letter-spacing:0.5px">MAGICBOX SOVEREIGN STUDIO v5.0 (NASTYA EDITION)</b>
          <span class="mono" style="font-size:11px;color:var(--accent-cool)" id="fwBadge">FW: --</span>
        </div>
        <div class="mono" style="font-size:11px;color:var(--text-secondary);margin-top:3px">
          Server RAM: <b id="ramTxt" style="color:var(--accent-success)">-- MB</b> |
          Сховище: <b id="diskTxt">-- MB</b> |
          Черга: <b id="queueTxt">0</b>
        </div>
      </div>
    </div>
    <div style="display:flex;gap:8px;align-items:center">
      <span id="roleBadge" class="mono" style="font-size:11px;padding:5px 10px;border-radius:6px;background:var(--bg-surface);border:1px solid var(--border-hairline);color:var(--accent-warm)">🌸 НАСТЯ</span>
      <button id="dndTopBtn" class="btn" style="padding:6px 12px;font-size:11px" onclick="toggleDnd()">🌙 DND: ВИКЛ</button>
      <a href="/api/v5/infographic/png" target="_blank" class="btn btn-cool" style="text-decoration:none;display:flex;align-items:center;gap:6px">
        📊 800x500 Інфографіка
      </a>
      <button onclick="doLogout()" class="btn" style="padding:6px 12px;font-size:11px">🚪 Вийти</button>
    </div>
  </div>

  <div class="grid-4">
    <div class="kpi"><span>Температура в кімнаті</span><b id="kTemp" style="color:var(--accent-cool)">--.- °C</b></div>
    <div class="kpi"><span>Вологість повітря</span><b id="kHum" style="color:var(--accent-success)">-- %</b></div>
    <div class="kpi"><span>Барометр (Тренд)</span><b id="kPres" style="color:var(--accent-warm)">--- мм</b></div>
    <div class="kpi"><span>Режим та Яскравість</span><b id="kMode" style="font-size:16px">--</b></div>
  </div>

  <!-- Spotify Vinyl Hub (v4.5) -->
  <div class="card" style="margin-bottom:12px;background:var(--bg-surface);border:1px solid var(--border-hairline)">
    <div style="display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px">
      <div style="display:flex;align-items:center;gap:12px">
        <div style="width:44px;height:44px;border-radius:50%;background:var(--bg-primary);border:2px solid var(--accent-warm);display:flex;align-items:center;justify-content:center;font-size:20px" id="spDisc">
          🎵
        </div>
        <div>
          <div style="display:flex;align-items:center;gap:8px">
            <b id="spTitle" style="font-size:14px;color:var(--text-primary)">Spotify не підключено</b>
            <span id="spBadge" class="mono" style="font-size:10px;padding:2px 6px;border-radius:10px;background:var(--border-hairline);color:var(--text-secondary)">IDLE</span>
          </div>
          <div id="spArtist" class="mono" style="font-size:11px;color:var(--text-secondary);margin-top:2px">
            Синхронізація обкладинки для вінілового плеєра шкатулки (Вікно 3)
          </div>
        </div>
      </div>
      <div style="display:flex;gap:6px;align-items:center" id="spControls">
        <button class="btn" style="padding:6px 12px" onclick="spotifyAction('previous')">⏮</button>
        <button class="btn btn-emerald" style="padding:6px 14px" id="spPlayBtn" onclick="spotifyAction('toggle')">⏯</button>
        <button class="btn" style="padding:6px 12px" onclick="spotifyAction('next')">⏭</button>
        <button class="btn btn-cool" style="padding:6px 12px" onclick="loginSpotify()">🔗 Підключити Spotify</button>
      </div>
    </div>
  </div>

  <!-- Nastya Daily Comfort & Routines (v5.0) -->
  <div class="split-2">
    <!-- Затишок та Режим Дня Насті -->
    <div class="card">
      <h3><span>🌸 Затишок та Режим Дня (Настя)</span></h3>
      
      <!-- Тихі Години -->
      <div style="margin-bottom:12px;background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
          <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">🌙 ТИХІ ГОДИНИ (QUIET HOURS):</span>
          <button class="btn" id="qhToggleBtn" onclick="toggleQuietHours()" style="padding:4px 10px;font-size:11px">🌙 ВИМКНЕНО</button>
        </div>
        <div style="display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:center">
          <div>
            <label style="font-size:10px;color:var(--text-muted);text-transform:uppercase">Початок:</label>
            <input type="time" id="qhStart" value="23:00" style="padding:6px 8px;font-size:12px">
          </div>
          <div>
            <label style="font-size:10px;color:var(--text-muted);text-transform:uppercase">Кінець:</label>
            <input type="time" id="qhEnd" value="08:00" style="padding:6px 8px;font-size:12px">
          </div>
          <button class="btn" style="margin-top:14px;padding:6px 12px;font-size:11px" onclick="saveQuietHours()">Зберегти</button>
        </div>
        <div style="font-size:11px;color:var(--text-muted);margin-top:6px">
          Авто-приглушення світла до 15% та повна тиша (Mute) для спокійного сну.
        </div>
      </div>

      <!-- Розумний Світанок -->
      <div style="margin-bottom:12px;background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
          <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">🌅 РОЗУМНИЙ СВІТАНОК:</span>
          <button class="btn" id="alarmToggleBtn" onclick="toggleAlarm()" style="padding:4px 10px;font-size:11px">🌅 ВИМКНЕНО</button>
        </div>
        <div style="display:grid;grid-template-columns:1fr auto;gap:8px;align-items:center">
          <div>
            <label style="font-size:10px;color:var(--text-muted);text-transform:uppercase">Час підйому:</label>
            <input type="time" id="alarmTime" value="07:30" style="padding:6px 8px;font-size:12px">
          </div>
          <button class="btn" style="margin-top:14px;padding:6px 12px;font-size:11px" onclick="saveAlarm()">Зберегти</button>
        </div>
        <div style="font-size:11px;color:var(--text-muted);margin-top:6px">
          За 20 хв до будильника лампа плавно вмикає сонячне світло (1% → 100%) та грає тихий акорд.
        </div>
      </div>

      <!-- Ранковий Гід Насті -->
      <div style="display:flex;gap:8px;align-items:center">
        <button class="btn btn-emerald" style="flex:1" onclick="triggerMorningBrief()">🌤 Отримати ранкову пораду Насті</button>
      </div>
      <div id="morningBriefBox" style="margin-top:8px;font-size:12px;color:var(--text-primary);background:var(--bg-surface);border:1px solid var(--border-hairline);padding:10px;border-radius:8px;display:none"></div>
    </div>

    <!-- Навчання Pomodoro & Особистий Google Календар -->
    <div class="card">
      <h3><span>📚 Фокус Pomodoro & 📅 Приватний Календар</span></h3>

      <!-- Pomodoro Таймер -->
      <div style="margin-bottom:14px;background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
          <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">СТАТУС: <b id="pomoStatus" class="mono" style="color:var(--text-muted)">НЕАКТИВНИЙ</b></span>
          <b id="pomoTimer" class="mono" style="font-size:18px;color:var(--accent-warm)">25:00</b>
        </div>
        <div style="display:flex;gap:6px;align-items:center">
          <button class="btn" style="padding:6px 10px" onclick="adjustPomoInput(-5)">-5 хв</button>
          <input type="number" id="pomoInput" value="25" min="5" max="120" style="width:60px;text-align:center;padding:6px" class="mono">
          <button class="btn" style="padding:6px 10px" onclick="adjustPomoInput(5)">+5 хв</button>
          <button class="btn active" style="flex:1;padding:8px" onclick="startPomodoro(parseInt(document.getElementById('pomoInput').value)||25)">▶️ Старт</button>
          <button class="btn btn-danger" style="padding:8px 12px" onclick="stopPomodoro()">🛑 Стоп</button>
        </div>
        <div style="font-size:11px;color:var(--text-muted);margin-top:6px">
          💡 Робота: теплий бурштин без сповіщень. Перерва: спокійний смарагдовий бриз. Кнопки на шкатулці: Ліва -хв, Права +хв, Кришка Старт/Пауза.
        </div>
      </div>

      <!-- Zero-Knowledge Google Календар & Розклад Подій -->
      <div style="background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
          <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">📅 РОЗКЛАД ТА КАЛЕНДАР:</span>
          <span id="calStatusBadge" class="mono" style="font-size:11px;color:var(--text-muted)">НЕ ПІДКЛЮЧЕНО</span>
        </div>
        <div id="calNextEvent" class="mono" style="font-size:12px;color:var(--accent-cool);margin-bottom:8px">
          📅 Найближча подія: --
        </div>

        <!-- Швидке додавання події -->
        <div style="display:grid;grid-template-columns:85px 1fr auto;gap:6px;align-items:center;margin-bottom:10px">
          <input type="text" id="calTimeInput" placeholder="14:30" class="mono" style="margin:0;padding:6px 8px;font-size:12px">
          <input type="text" id="calSummaryInput" placeholder="Назва події (напр. Семінар)..." style="margin:0;padding:6px 10px;font-size:12px">
          <button class="btn btn-emerald" style="padding:6px 12px;font-size:11px" onclick="addCalendarEvent()">+ Додати</button>
        </div>

        <!-- Список подій -->
        <div id="calEventList" style="max-height:150px;overflow-y:auto;margin-bottom:8px;font-size:12px">
          <div style="color:var(--text-muted);font-size:11px">Завантаження розкладу...</div>
        </div>

        <div style="display:flex;gap:8px;align-items:center">
          <button class="btn" style="padding:6px 12px;font-size:11px" onclick="loadCalendarEvents()">🔄 Оновити</button>
          <button class="btn btn-cool" id="calLoginBtn" style="padding:6px 12px;font-size:11px" onclick="loginGoogle()">🔗 Підключити Google</button>
          <button class="btn btn-danger" id="calDisconnectBtn" style="padding:6px 12px;font-size:11px;display:none" onclick="disconnectGoogle()">✕ Відключити</button>
        </div>
        <div style="font-size:11px;color:var(--text-muted);margin-top:6px">
          Шкатулка бере найближчу подію та виводить нагадування на дисплей Smart Hub (Вікно 3).
        </div>
      </div>
    </div>
  </div>

  <div class="split-2">
    <!-- Світло та Яскравість -->
    <div class="card">
      <h3><span>🎨 Режими Світла, Яскравість та Реакції</span></h3>
      <div class="mode-grid" id="modeGrid"></div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:10px">
        <button class="btn" onclick="sendCmd({set_brt:'auto'})">АВТО</button>
        <button class="btn" onclick="sendCmd({set_brt:'0'})">🌙 15%</button>
        <button class="btn" onclick="sendCmd({set_brt:'1'})">☕ 35%</button>
        <button class="btn" onclick="sendCmd({set_brt:'2'})">✨ 65%</button>
        <button class="btn" onclick="sendCmd({set_brt:'3'})">☀️ 100%</button>
      </div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:12px">
        <button class="btn" onclick="sendCmd({react:2,sender:'Studio',msg:'FIRE VIBE!'})">🔥 Вогник</button>
        <button class="btn" onclick="sendCmd({react:3,sender:'Studio',msg:'KUS-ZAP!'})">⚡ Розряд</button>
        <button class="btn" onclick="sendCmd({react:4,sender:'Studio',msg:'TYTS-KUS!'})">🐱 Котик</button>
        <button class="btn" onclick="sendCmd({react:5,sender:'Studio',msg:'UFO BEAM!'})">👽 НЛО</button>
        <button class="btn" onclick="sendCmd({react:6,sender:'Studio',msg:'QUEEN!'})">👑 Корона</button>
      </div>
      <div style="display:flex;gap:8px">
        <input type="text" id="msgTxt" placeholder="Повідомлення рідною кирилицею на екран або для озвучки...">
        <button class="btn active" style="white-space:nowrap" onclick="pushScreenMsg()">💬 На екран</button>
        <button class="btn btn-emerald" style="white-space:nowrap" onclick="speakTts()">🔊 Озвучити</button>
      </div>
    </div>

    <!-- Акустика, Гучність та Синтез Голосу (v4.5) -->
    <div class="card">
      <h3><span>🔊 Акустика та Синтез Голосу (v4.5)</span></h3>
      
      <!-- Гучність та Mute -->
      <div style="margin-bottom:14px;background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px">
          <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">ГУЧНІСТЬ КОЛОНКИ: <b id="volVal" class="mono" style="color:var(--accent-warm)">12 / 21</b></span>
          <div style="display:flex;gap:6px">
            <button class="btn" id="muteBtn" onclick="toggleMute()" style="padding:4px 10px;font-size:11px">🔊 ЗВУК УВІМК</button>
            <button class="btn" id="touchBeepBtn" onclick="toggleTouchBeep()" style="padding:4px 10px;font-size:11px">🔔 КЛІКИ: ТАК</button>
          </div>
        </div>
        <div class="slider-container">
          <span class="mono" style="font-size:11px;color:var(--text-muted)">0</span>
          <input type="range" id="volSlider" min="0" max="21" value="12" oninput="onVolSlider(this.value)" onchange="applyVolume(this.value)">
          <span class="mono" style="font-size:11px;color:var(--text-muted)">21</span>
        </div>
      </div>

      <!-- Вибір Голосу -->
      <form onsubmit="saveVoiceConfig(event)">
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:8px">
          <div>
            <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase">Рушій TTS:</label>
            <select id="ttsProviderSelect" onchange="onProviderChange()">
              <option value="edge-tts">Edge-TTS (Безкоштовно)</option>
              <option value="elevenlabs">ElevenLabs (Клонований)</option>
              <option value="fish_audio">Fish Audio (Нейро-голос)</option>
            </select>
          </div>
          <div>
            <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase">Голос Edge-TTS:</label>
            <select id="edgeVoiceSelect">
              <option value="uk-UA-OstapNeural">Остап 👨 (uk-UA-Ostap)</option>
              <option value="uk-UA-PolinaNeural">Поліна 👩 (uk-UA-Polina)</option>
            </select>
          </div>
        </div>
        <div id="cloneVoiceRow" style="display:none;margin-bottom:8px">
          <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase">Voice Clone ID:</label>
          <input type="text" id="cloneVoiceIdInput" placeholder="eleven_voice_id...">
        </div>
        <div style="display:flex;justify-content:space-between;align-items:center">
          <button type="submit" class="btn active">Зберегти налаштування голосу</button>
        </div>
      </form>

      <!-- Голосові пресети -->
      <div style="margin-top:12px">
        <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase;display:block;margin-bottom:6px">Збережені голосові фрази:</label>
        <div id="voicePresetsList" style="display:flex;flex-wrap:wrap;gap:6px;max-height:90px;overflow-y:auto"></div>
      </div>
    </div>
  </div>

  <div class="split-2">
    <!-- Студія Кастомних Гіфок (.mbx AmbiLight) та Таймер -->
    <div class="card">
      <h3>
        <span>🎞 Студія Кастомних Гіфок (.mbx)</span>
        <button class="btn btn-danger" style="padding:4px 10px;font-size:11px" onclick="stopMedia()">🛑 Зупинити показ</button>
      </h3>
      
      <!-- Панель швидкого таймера для поточного медіа -->
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:12px;background:var(--bg-surface);padding:8px 12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <span class="mono" style="font-size:11px;color:var(--text-secondary)">⏱ Таймер показу:</span>
        <button class="btn" style="padding:4px 8px;font-size:11px" onclick="setMediaTimer(15)">15 хв</button>
        <button class="btn" style="padding:4px 8px;font-size:11px" onclick="setMediaTimer(60)">1 год</button>
        <button class="btn" style="padding:4px 8px;font-size:11px" onclick="setMediaTimer(180)">3 год</button>
        <button class="btn" style="padding:4px 8px;font-size:11px" onclick="setMediaTimer(0)">♾ Безкінечно</button>
      </div>

      <form onsubmit="uploadMedia(event)" style="display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:center;margin-bottom:12px">
        <input type="file" id="mediaFile" accept=".gif,.webp,.png,.jpg,.jpeg" required>
        <div style="display:flex;gap:6px">
          <input type="text" id="mediaName" placeholder="Назва (cat_vibe)">
          <select id="mediaSlot" style="width:130px">
            <option value="0">Без прив'язки</option>
            <option value="2">🔥 Кнопка Вогник</option>
            <option value="3">⚡ Кнопка Блискавка</option>
            <option value="4">🐱 Кнопка Котик</option>
            <option value="5">👽 Кнопка НЛО</option>
            <option value="6">👑 Кнопка Корона</option>
          </select>
        </div>
        <button type="submit" class="btn btn-cool" style="padding:10px 14px">Завантажити</button>
      </form>
      <div class="media-grid" id="mediaGrid"></div>
    </div>

    <!-- Персона ШІ -->
    <div class="card">
      <h3><span>🧠 Налаштування Персони ШІ (Gemini)</span></h3>
      <form onsubmit="saveAiPersona(event)">
        <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase">Ім'я Духа:</label>
        <input type="text" id="aiName" value="Затишний Дух">
        <label style="font-size:11px;color:var(--text-secondary);text-transform:uppercase;margin-top:6px;display:block">Системний характер (Промпт):</label>
        <textarea id="aiPrompt" rows="3"></textarea>
        <div style="display:flex;justify-content:space-between;align-items:center;margin-top:8px">
          <button type="submit" class="btn active">Зберегти характер</button>
          <button type="button" class="btn btn-emerald" onclick="askAiBriefing()">Слухати свіжий брифінг</button>
        </div>
      </form>
      <div id="aiResponseBox" style="margin-top:12px;font-size:12.5px;color:var(--text-secondary);background:var(--bg-surface);border:1px solid var(--border-hairline);padding:10px;border-radius:8px;display:none"></div>
    </div>
  </div>

  <div class="card" style="margin-bottom:16px">
    <h3><span>📈 Історія Клімату та Барометра (SQLite WAL)</span></h3>
    <div id="climateChart" style="height:260px"></div>
  </div>

  <!-- 🛡️ Панель Архітектора (Ярослав) -->
  <div id="adminPanel" style="display:none;margin-top:16px" class="card">
    <h3><span>🛡️ Панель Архітектора (Ярослав)</span></h3>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px">
      <!-- Блок Привітання з Днем Народження -->
      <div style="background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="font-size:12px;font-weight:600;color:var(--accent-warm);margin-bottom:6px">🎂 ПРИВІТАННЯ З ДНЕМ НАРОДЖЕННЯ:</div>
        <p style="font-size:11px;color:var(--text-secondary);margin-bottom:10px">
          Миттєвий запуск святкової листівки на шкатулці, світлового шоу та надсилання привітання в Telegram (навіть якщо свято вже минуло або попереду).
        </p>
        <button class="btn btn-cool" style="width:100%;padding:8px" onclick="triggerBirthdayCelebration()">🎉 Привітати Настю з Днем Народження!</button>
      </div>

      <!-- Блок тихого повідомлення Silent Push -->
      <div style="background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline)">
        <div style="font-size:12px;font-weight:600;color:var(--accent-cool);margin-bottom:6px">🤫 ТИХЕ ПОВІДОМЛЕННЯ (SILENT PUSH):</div>
        <div style="display:flex;gap:6px;margin-bottom:6px">
          <input type="text" id="silentMsgInput" placeholder="Текст без дзвіночка на екран...">
          <button class="btn" style="padding:8px 12px" onclick="sendSilentPush()">Надіслати</button>
        </div>
        <div style="font-size:11px;color:var(--text-muted)">
          Виводиться в картку Inbox без звукового сигналу та без різких спалахів (якщо Настя вчиться чи спить).
        </div>
      </div>
    </div>

    <!-- Перегляд розкладу Google Календаря Насті -->
    <div style="background:var(--bg-surface);padding:12px;border-radius:8px;border:1px solid var(--border-hairline);margin-bottom:12px">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
        <span style="font-size:12px;font-weight:600;color:var(--text-secondary)">📅 РОЗКЛАД НАСТІ (GOOGLE КАЛЕНДАР):</span>
        <button class="btn" style="padding:4px 10px;font-size:11px" onclick="loadNastyaSchedule()">🔄 Оновити розклад</button>
      </div>
      <div id="scheduleContainer" class="mono" style="font-size:11.5px;color:var(--text-secondary);max-height:160px;overflow-y:auto">
        Натисніть «Оновити розклад» для перегляду подій...
      </div>
    </div>

    <!-- Системне здоров'я та бекап -->
    <div style="display:flex;gap:8px;flex-wrap:wrap">
      <button class="btn" onclick="triggerAdminBackup()">📦 Створити та надіслати бекап БД в Telegram</button>
      <button class="btn" onclick="loadHealthReport()">🩺 Повний звіт про здоров'я системи</button>
    </div>
    <div id="healthReportBox" style="margin-top:8px;font-size:11px;font-family:var(--font-mono);background:var(--bg-primary);border:1px solid var(--border-hairline);padding:10px;border-radius:6px;display:none;white-space:pre-wrap"></div>
  </div>
</div>

<div class="toast" id="toast">Виконано</div>

<script>
const MODES = [
  "☀️ Теплий затишок", "🕯 Жива свічка", "🍯 Бурштинова свічка", "🔥 Камін з іскрами",
  "🌅 Захід сонця", "🌸 Сакура Неон", "🌌 Північне сяйво", "🌊 Океанський бриз",
  "⚡ Кібер-комета", "🌈 Веселка", "🌑 Вимкнено", "🎵 Spotify Ambi"
];

const mg = document.getElementById('modeGrid');
MODES.forEach((name, idx) => {
  const b = document.createElement('button');
  b.className = 'btn';
  b.id = 'mBtn' + idx;
  b.innerText = name;
  b.onclick = () => sendCmd({set_mode: idx});
  mg.appendChild(b);
});

let currentMuted = false;
let currentTouchBeep = true;

function toast(msg){
  const t = document.getElementById('toast');
  t.innerText = msg;
  t.style.display = 'block';
  setTimeout(() => t.style.display = 'none', 2400);
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
      toast('Успішний вхід!');
      pollState();
    } else {
      alert('Невірний пароль!');
    }
  });
}

function doLogout(){
  fetch('/api/v5/auth/logout', {method:'POST'}).then(() => {
    document.getElementById('loginModal').style.display = 'flex';
    toast('Сесію завершено');
  });
}

function sendCmd(cmdObj){
  fetch('/api/v5/command',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(cmdObj)
  }).then(r => {
    if(r.status === 401) document.getElementById('loginModal').style.display = 'flex';
    else toast('Команду передано в чергу!');
  });
}

function pushScreenMsg(){
  const val = document.getElementById('msgTxt').value.trim();
  if(!val) return;
  sendCmd({sender:'Studio', msg: val});
  document.getElementById('msgTxt').value = '';
}

function speakTts(){
  const val = document.getElementById('msgTxt').value.trim();
  if(!val) return;
  fetch('/api/v1/audio/tts?text=' + encodeURIComponent(val), {method:'POST'}).then(r => {
    if(r.status === 401) document.getElementById('loginModal').style.display = 'flex';
    else toast('Озвучку згенеровано та передано!');
  });
  document.getElementById('msgTxt').value = '';
}

// Volume & Audio Controls (v4.5)
function onVolSlider(v){
  document.getElementById('volVal').innerText = `${v} / 21`;
}

function applyVolume(v){
  fetch('/api/v5/audio/volume', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({volume: parseInt(v), muted: currentMuted, touch_beep: currentTouchBeep})
  }).then(() => toast(`Гучність: ${v}/21`));
}

function toggleMute(){
  currentMuted = !currentMuted;
  const v = parseInt(document.getElementById('volSlider').value || '12');
  fetch('/api/v5/audio/volume', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({volume: v, muted: currentMuted, touch_beep: currentTouchBeep})
  }).then(() => {
    updateMuteUI(currentMuted);
    toast(currentMuted ? 'Звук вимкнено (MUTE)' : 'Звук увімкнено');
  });
}

function updateMuteUI(m){
  const btn = document.getElementById('muteBtn');
  if(m){
    btn.innerText = '🔇 MUTE (ТИША)';
    btn.className = 'btn btn-danger';
  } else {
    btn.innerText = '🔊 ЗВУК УВІМК';
    btn.className = 'btn btn-emerald';
  }
}

function toggleTouchBeep(){
  currentTouchBeep = !currentTouchBeep;
  const v = parseInt(document.getElementById('volSlider').value || '12');
  fetch('/api/v5/audio/volume', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({volume: v, muted: currentMuted, touch_beep: currentTouchBeep})
  }).then(() => {
    updateTouchBeepUI(currentTouchBeep);
    toast(currentTouchBeep ? 'Кліки кнопок увімкнено' : 'Кліки кнопок вимкнено');
  });
}

function updateTouchBeepUI(tb){
  const btn = document.getElementById('touchBeepBtn');
  btn.innerText = tb ? '🔔 КЛІКИ: ТАК' : '🔕 КЛІКИ: НІ';
  btn.className = tb ? 'btn btn-emerald' : 'btn';
}

function onProviderChange(){
  const prov = document.getElementById('ttsProviderSelect').value;
  document.getElementById('cloneVoiceRow').style.display = (prov === 'elevenlabs' || prov === 'fish_audio') ? 'block' : 'none';
}

function saveVoiceConfig(e){
  e.preventDefault();
  const provider = document.getElementById('ttsProviderSelect').value;
  const edgeVoice = document.getElementById('edgeVoiceSelect').value;
  const cloneId = document.getElementById('cloneVoiceIdInput').value.trim();
  fetch('/api/v5/audio/voice-config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({provider: provider, edge_voice: edgeVoice, clone_voice_id: cloneId})
  }).then(() => toast('Налаштування голосу збережено!'));
}

function renderVoicePresets(presets){
  const el = document.getElementById('voicePresetsList');
  el.innerHTML = '';
  if(!presets || presets.length === 0){
    el.innerHTML = '<span style="color:var(--text-muted);font-size:11px">Немає збережених фраз</span>';
    return;
  }
  presets.forEach(p => {
    const btn = document.createElement('button');
    btn.className = 'btn';
    btn.style.padding = '4px 8px';
    btn.style.fontSize = '11px';
    btn.innerText = `▶️ ${p.name}`;
    btn.title = p.text;
    btn.onclick = () => {
      fetch(`/api/v5/audio/preset/${encodeURIComponent(p.name)}/play`, {method: 'POST'})
        .then(() => toast(`Озвучено: «${p.name}»`));
    };
    el.appendChild(btn);
  });
}

// Media Loop & Timers (v4.5)
function stopMedia(){
  fetch('/api/v5/media/stop', {method:'POST'}).then(() => toast('Показ медіа зупинено'));
}

function setMediaTimer(minutes){
  fetch(`/api/v5/media/timer?minutes=${minutes}`, {method:'POST'}).then(() => {
    toast(minutes > 0 ? `Таймер встановлено на ${minutes} хв` : 'Медіа зациклено безкінечно');
  });
}

function uploadMedia(e){
  e.preventDefault();
  const file = document.getElementById('mediaFile').files[0];
  const name = document.getElementById('mediaName').value.trim();
  const slot = parseInt(document.getElementById('mediaSlot').value || '0');
  if(!file) return;

  const fd = new FormData();
  fd.append('file', file);
  let url = '/api/v5/media/upload?reaction_slot=' + slot;
  if(name) url += '&save_name=' + encodeURIComponent(name);

  toast('Обробка кадрів у RGB565...');
  fetch(url, {method:'POST', body: fd}).then(r => r.json()).then(d => {
    toast(`Завантажено «${d.name}» (${d.frames} кадрів)!`);
    document.getElementById('mediaFile').value = '';
    document.getElementById('mediaName').value = '';
    pollState();
  });
}

function renderGallery(list){
  const grid = document.getElementById('mediaGrid');
  grid.innerHTML = '';
  if(!list || list.length === 0){
    grid.innerHTML = '<div style="color:var(--text-muted);font-size:12px;padding:10px">Ще немає кастомних гіфок. Завантажте першу!</div>';
    return;
  }
  list.forEach(m => {
    const card = document.createElement('div');
    card.className = 'media-item';
    const rgb = `rgb(${m.ambi_r},${m.ambi_g},${m.ambi_b})`;
    card.innerHTML = `
      <div>
        <div style="display:flex;justify-content:space-between;align-items:center">
          <b style="font-size:13px">${m.name}</b>
          <span class="ambi-dot" style="background:${rgb}" title="AmbiLight RGB: ${rgb}"></span>
        </div>
        <div class="mono" style="font-size:11px;color:var(--text-secondary);margin-top:3px">
          ${m.frames} к. @ ${m.fps} FPS | ${m.reaction_slot ? 'Слот ' + m.reaction_slot : 'Вільна'}
        </div>
      </div>
      <div style="display:flex;gap:6px;margin-top:4px">
        <button class="btn btn-emerald" style="flex:1;padding:6px" onclick="triggerMedia('${m.name}')">Запустити</button>
        <button class="btn" style="flex:1;padding:6px;border:1px solid var(--accent-warm);color:var(--accent-warm)" onclick="setAsSaver('${m.name}')">🖼 На Заставку</button>
        <button class="btn" style="color:var(--accent-danger);padding:6px 10px" onclick="deleteMedia('${m.name}')">✕</button>
      </div>
    `;
    grid.appendChild(card);
  });
}

function triggerMedia(name){
  fetch('/api/v5/media/trigger/' + encodeURIComponent(name), {method:'POST'}).then(() => toast(`Анімацію «${name}» активовано!`));
}

function setAsSaver(name){
  fetch('/api/v5/media/trigger/' + encodeURIComponent(name) + '?saver=true', {method:'POST'})
    .then(() => toast(`«${name}» встановлено як постійну заставку (Вікно 2)!`));
}

function spotifyAction(act){
  if(act === 'toggle'){
    const isP = window.lastSpotifyPlaying || false;
    act = isP ? 'pause' : 'play';
  }
  fetch('/api/v5/spotify/action', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({action: act})
  }).then(r => r.json()).then(() => {
    toast(`Spotify: ${act}`);
    pollState();
  });
}

function loginSpotify(){
  fetch('/api/v5/spotify/login')
    .then(r => r.json())
    .then(d => {
      if(d.auth_url) window.open(d.auth_url, '_blank');
      else toast('Client ID не налаштований');
    });
}

function deleteMedia(name){
  if(confirm(`Видалити «${name}»?`)){
    fetch('/api/v5/media/' + encodeURIComponent(name), {method:'DELETE'}).then(() => {
      toast('Видалено');
      pollState();
    });
  }
}

function saveAiPersona(e){
  e.preventDefault();
  const name = document.getElementById('aiName').value;
  const prompt = document.getElementById('aiPrompt').value;
  fetch('/api/v5/ai/persona',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({persona_name: name, system_prompt: prompt, temperature: 0.75})
  }).then(() => toast('Персону ШІ оновлено!'));
}

function askAiBriefing(){
  toast('Генерація ШІ-брифінгу...');
  fetch('/api/v1/ai/briefing',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({speak: true})
  }).then(r => r.json()).then(d => {
    const box = document.getElementById('aiResponseBox');
    box.style.display = 'block';
    box.innerText = `[${d.persona}] ` + d.ai_forecast;
    toast('ШІ озвучено на шкатулці!');
  });
}

// Nastya Routine & Comfort (v5.0)
let quietHoursEnabled = false;
let alarmEnabled = false;

function toggleQuietHours() {
  quietHoursEnabled = !quietHoursEnabled;
  saveQuietHours();
}

function saveQuietHours() {
  const start = document.getElementById('qhStart').value || '23:00';
  const end = document.getElementById('qhEnd').value || '08:00';
  fetch(`/api/v5/nastya/quiet-hours?enabled=${quietHoursEnabled}&start=${encodeURIComponent(start)}&end=${encodeURIComponent(end)}`, {
    method: 'POST'
  }).then(r => r.json()).then(d => {
    updateQuietHoursUI(d.enabled, d.start, d.end);
    toast(d.enabled ? '🌙 Тихі години увімкнено!' : 'Тихі години вимкнено');
  });
}

function updateQuietHoursUI(enabled, start, end) {
  quietHoursEnabled = Boolean(enabled);
  const btn = document.getElementById('qhToggleBtn');
  if (btn) {
    btn.innerText = quietHoursEnabled ? '🌙 АКТИВНІ (ТИША)' : '🌙 ВИМКНЕНО';
    btn.className = quietHoursEnabled ? 'btn btn-emerald' : 'btn';
  }
  if (start && document.getElementById('qhStart')) document.getElementById('qhStart').value = start;
  if (end && document.getElementById('qhEnd')) document.getElementById('qhEnd').value = end;
}

function toggleAlarm() {
  alarmEnabled = !alarmEnabled;
  saveAlarm();
}

function saveAlarm() {
  const t = document.getElementById('alarmTime').value || '07:30';
  fetch(`/api/v5/nastya/alarm?enabled=${alarmEnabled}&time_str=${encodeURIComponent(t)}&days=0,1,2,3,4`, {
    method: 'POST'
  }).then(r => r.json()).then(d => {
    updateAlarmUI(d.enabled, d.time);
    toast(d.enabled ? `🌅 Будильник-світанок встановлено на ${d.time}` : 'Будильник вимкнено');
  });
}

function updateAlarmUI(enabled, timeVal) {
  alarmEnabled = Boolean(enabled);
  const btn = document.getElementById('alarmToggleBtn');
  if (btn) {
    btn.innerText = alarmEnabled ? `🌅 УВІМКНЕНО (${timeVal || '07:30'})` : '🌅 ВИМКНЕНО';
    btn.className = alarmEnabled ? 'btn btn-emerald' : 'btn';
  }
  if (timeVal && document.getElementById('alarmTime')) document.getElementById('alarmTime').value = timeVal;
}

function triggerMorningBrief() {
  toast('Запит ранкового гіда для Насті...');
  fetch('/api/v5/nastya/morning-brief/trigger', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      const box = document.getElementById('morningBriefBox');
      if (box) {
        box.style.display = 'block';
        box.innerText = d.brief || 'Гід сформовано!';
      }
      toast('Ранковий гід озвучено та відправлено!');
    });
}

function adjustPomoInput(delta) {
  const el = document.getElementById('pomoInput');
  if (!el) return;
  let val = parseInt(el.value) || 25;
  val = Math.max(5, Math.min(120, val + delta));
  el.value = val;
}

function startPomodoro(min) {
  fetch(`/api/v5/nastya/pomodoro/start?work_min=${min}`, { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast(`📚 Помодоро: ${min} хв фокусу (теплий бурштин)!`);
      updatePomodoroUI(d);
    });
}

function stopPomodoro() {
  fetch('/api/v5/nastya/pomodoro/stop', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast('Помодоро зупинено');
      updatePomodoroUI(d);
    });
}

function triggerBirthdayCelebration() {
  if (!confirm('Запустити святкову листівку на шкатулці та надіслати привітання Насті в Telegram?')) return;
  fetch('/api/v5/admin/birthday-trigger', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast(d.message || 'Привітання надіслано!');
    });
}

function sendSilentPush() {
  const txt = document.getElementById('silentMsgInput').value.trim();
  if (!txt) return;
  sendCmd({ msg: txt, sender: 'Ярослав', silent: true });
  document.getElementById('silentMsgInput').value = '';
  toast('🤫 Тихе повідомлення відправлено на екран!');
}

function loadNastyaSchedule() {
  const box = document.getElementById('scheduleContainer');
  box.innerText = 'Завантаження розкладу...';
  fetch('/api/v5/admin/nastya-schedule')
    .then(r => r.json())
    .then(d => {
      if (!d.ok || !d.events || d.events.length === 0) {
        box.innerText = 'Немає запланованих подій або календар не підключено.';
        return;
      }
      box.innerHTML = d.events.map(e => `<div style="margin-bottom:4px">🕒 <b>${e.start_time}</b>: ${e.summary} ${e.location ? '(' + e.location + ')' : ''}</div>`).join('');
    })
    .catch(() => {
      box.innerText = 'Помилка завантаження розкладу.';
    });
}

function triggerAdminBackup() {
  toast('⏳ Створення бекапу...');
  fetch('/api/v5/admin/backup', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast(d.message || 'Бекап виконано!');
    });
}

function loadHealthReport() {
  const box = document.getElementById('healthReportBox');
  box.style.display = 'block';
  box.innerText = 'Отримання звіту...';
  fetch('/api/v5/admin/health-report')
    .then(r => r.json())
    .then(d => {
      box.innerText = JSON.stringify(d.report, null, 2);
    });
}

function updatePomodoroUI(pomo) {
  if (!pomo) return;
  const statusEl = document.getElementById('pomoStatus');
  const timerEl = document.getElementById('pomoTimer');
  if (pomo.active) {
    const min = Math.floor(pomo.remaining_sec / 60);
    const sec = ('0' + (pomo.remaining_sec % 60)).slice(-2);
    if (timerEl) timerEl.innerText = `${min}:${sec}`;
    if (statusEl) {
      if (pomo.mode === 'work') {
        statusEl.innerText = '🔥 ФОКУС (БУРШТИН)';
        statusEl.style.color = 'var(--accent-warm)';
      } else {
        statusEl.innerText = '☕ ВІДПОЧИНОК (СМАРАГД)';
        statusEl.style.color = 'var(--accent-success)';
      }
    }
  } else {
    if (timerEl) timerEl.innerText = '25:00';
    if (statusEl) {
      statusEl.innerText = 'НЕАКТИВНИЙ';
      statusEl.style.color = 'var(--text-muted)';
    }
  }
}

function loginGoogle() {
  fetch('/api/v5/google/login')
    .then(r => r.json())
    .then(d => {
      if (d.auth_url) window.open(d.auth_url, '_blank');
      else toast('Google Client ID не налаштовано');
    });
}

function disconnectGoogle() {
  if (confirm('Відключити Google Календар?')) {
    fetch('/api/v5/google/disconnect', { method: 'POST' })
      .then(() => {
        toast('Календар відключено');
        pollState();
      });
  }
}

function updateGoogleUI(linked, nextEvt) {
  const badge = document.getElementById('calStatusBadge');
  const btnLogin = document.getElementById('calLoginBtn');
  const btnDisc = document.getElementById('calDisconnectBtn');
  const nextEvtEl = document.getElementById('calNextEvent');
  if (linked) {
    if (badge) { badge.innerText = '🔗 ПІДКЛЮЧЕНО (Zero-Knowledge)'; badge.style.color = 'var(--accent-success)'; }
    if (btnLogin) btnLogin.style.display = 'none';
    if (btnDisc) btnDisc.style.display = 'inline-block';
    if (nextEvtEl) {
      if (nextEvt && (nextEvt.summary || nextEvt.title)) {
        const s = nextEvt.summary || nextEvt.title;
        const t = nextEvt.time || nextEvt.start_time || '';
        nextEvtEl.innerText = `📅 ${s} (${t})`;
      } else {
        nextEvtEl.innerText = '📅 Немає подій найближчим часом';
      }
    }
  } else {
    if (badge) { badge.innerText = 'НЕ ПІДКЛЮЧЕНО'; badge.style.color = 'var(--text-muted)'; }
    if (btnLogin) btnLogin.style.display = 'inline-block';
    if (btnDisc) btnDisc.style.display = 'none';
    if (nextEvtEl) nextEvtEl.innerText = '📅 Найближча подія: --';
  }
}

function toggleDnd() {
  fetch('/api/v5/dnd/toggle', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      updateDndUI(d.dnd);
      toast(d.dnd ? '🌙 Режим DND увімкнено (Нічник + MUTE)' : '☀️ Режим DND вимкнено');
    });
}

function updateDndUI(isDnd) {
  const btn = document.getElementById('dndTopBtn');
  if (btn) {
    btn.innerText = isDnd ? '🌙 DND: АКТИВНО' : '🌙 DND: ВИКЛ';
    btn.className = isDnd ? 'btn btn-emerald' : 'btn';
  }
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/[&<>'"]/g, tag => ({
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    "'": '&#39;',
    '"': '&quot;'
  }[tag] || tag));
}

function loadCalendarEvents() {
  fetch('/api/v5/calendar/events')
    .then(r => r.json())
    .then(d => {
      const container = document.getElementById('calEventList');
      if (!container) return;
      if (!d.events || d.events.length === 0) {
        container.innerHTML = '<div style="color:var(--text-muted);font-size:11px;padding:4px 0">Подій немає. Додайте подію вище або через Telegram-бота.</div>';
        return;
      }
      container.innerHTML = d.events.map(ev => {
        const srcIcon = ev.source === 'google' ? '🇬' : '📝';
        const dStr = ev.date ? ` (${ev.date})` : '';
        return `
          <div style="display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px solid var(--border-hairline)">
            <div>
              <span>${srcIcon}</span>
              <b class="mono" style="color:var(--accent-warm);margin-left:4px">${ev.time}${dStr}</b>
              <span style="margin-left:6px">${escapeHtml(ev.summary)}</span>
            </div>
            <button class="btn btn-danger" style="padding:2px 8px;font-size:10px" onclick="deleteCalendarEvent('${ev.id}')">🗑</button>
          </div>
        `;
      }).join('');
    }).catch(() => {});
}

function addCalendarEvent() {
  const tEl = document.getElementById('calTimeInput');
  const sEl = document.getElementById('calSummaryInput');
  const timeStr = tEl.value.trim();
  const summary = sEl.value.trim();
  if (!timeStr || !summary) {
    toast('Вкажіть час та назву події');
    return;
  }
  fetch('/api/v5/calendar/events', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ time_str: timeStr, summary: summary, source: 'manual' })
  }).then(r => r.json()).then(d => {
    toast('📅 Подію додано та синхронізовано!');
    tEl.value = '';
    sEl.value = '';
    loadCalendarEvents();
    pollState();
  });
}

function deleteCalendarEvent(id) {
  if (!confirm('Видалити подію з календаря?')) return;
  fetch('/api/v5/calendar/events/' + encodeURIComponent(id), { method: 'DELETE' })
    .then(r => r.json())
    .then(() => {
      toast('Подію видалено');
      loadCalendarEvents();
      pollState();
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
    document.getElementById('onlineTxt').innerText = on ? 'MAGICBOX SOVEREIGN STUDIO v5.0 (NASTYA EDITION)' : 'MAGICBOX OFFLINE (BLEKAUT)';
    document.getElementById('queueTxt').innerText = d.pending_commands;

    const res = d.resources || {};
    document.getElementById('ramTxt').innerText = (res.ram_rss_mb !== undefined ? res.ram_rss_mb : (res.rss_mb || 0)).toFixed(1) + ' MB';
    document.getElementById('diskTxt').innerText = (res.storage_used_mb !== undefined ? res.storage_used_mb : (res.disk_used_mb || 0)).toFixed(1) + ' MB';

    // Spotify Hub Update
    const sp = d.spotify || {};
    window.lastSpotifyPlaying = sp.playing || false;
    const spTitleEl = document.getElementById('spTitle');
    const spArtistEl = document.getElementById('spArtist');
    const spBadgeEl = document.getElementById('spBadge');
    const spPlayBtn = document.getElementById('spPlayBtn');
    if(sp.title){
      spTitleEl.innerText = sp.title;
      const prog_m = Math.floor((sp.progress_ms || 0) / 60000);
      const prog_s = ('0' + Math.floor(((sp.progress_ms || 0) % 60000) / 1000)).slice(-2);
      const dur_m = Math.floor((sp.duration_ms || 0) / 60000);
      const dur_s = ('0' + Math.floor(((sp.duration_ms || 0) % 60000) / 1000)).slice(-2);
      spArtistEl.innerText = `${sp.artist || 'Unknown'} · ${prog_m}:${prog_s} / ${dur_m}:${dur_s}`;
      spBadgeEl.innerText = sp.playing ? '▶️ PLAYING' : '⏸ PAUSED';
      spBadgeEl.style.color = sp.playing ? 'var(--accent-success)' : 'var(--text-secondary)';
      spPlayBtn.innerText = sp.playing ? '⏸ Пауза' : '▶️ Пуск';
    }

    const t = d.telemetry || {};
    if(t.t !== undefined){
      document.getElementById('fwBadge').innerText = `${t.fw || 'v5.0'} | ${t.rssi || -60} dBm`;
      document.getElementById('kTemp').innerText = t.t.toFixed(1) + ' °C';
      document.getElementById('kHum').innerText = t.h.toFixed(0) + ' %';
      document.getElementById('kPres').innerText = `${t.p.toFixed(0)} мм (${t.p_diff>=0?'+':''}${(t.p_diff||0).toFixed(1)})`;
      document.getElementById('kMode').innerText = `${t.m_name || '--'} (${t.b_pct || 0}%)`;
      MODES.forEach((_,i)=>{
        const el = document.getElementById('mBtn'+i);
        if(el) el.className = (t.m === i) ? 'btn active' : 'btn';
      });

      // Update volume slider from telemetry if not actively dragging
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
      if (rb) { rb.innerText = '🛡️ АРХІТЕКТОР (ЯРОСЛАВ)'; rb.style.color = 'var(--accent-cool)'; }
    } else {
      const ap = document.getElementById('adminPanel');
      if (ap) ap.style.display = 'none';
      const rb = document.getElementById('roleBadge');
      if (rb) { rb.innerText = '🌸 ЗАТИШОК (НАСТЯ)'; rb.style.color = 'var(--accent-warm)'; }
    }

    const ch = d.climate_history || [];
    if(ch.length > 1){
      const times = ch.map(x => new Date(x.ts*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}));
      Plotly.react('climateChart',[
        {x:times, y:ch.map(x=>x.t), name:'Темп (°C)', type:'scatter', mode:'lines', line:{color:'#4ea8de',width:2.5}},
        {x:times, y:ch.map(x=>x.p), name:'Барометр (мм)', type:'scatter', mode:'lines', line:{color:'#e07a5f',width:2,dash:'dot'}, yaxis:'y2'}
      ],{
        paper_bgcolor:'rgba(0,0,0,0)', plot_bgcolor:'rgba(8,9,12,0.6)',
        font:{color:'#8a94a6', family:'JetBrains Mono', size:11},
        margin:{l:40,r:40,t:10,b:30}, legend:{orientation:'h',y:1.14},
        xaxis:{gridcolor:'#222834'},
        yaxis:{gridcolor:'#222834'},
        yaxis2:{overlaying:'y', side:'right', gridcolor:'rgba(0,0,0,0)'}
      },{responsive:true, displayModeBar:false});
    }
  }).catch(()=>{});
}

pollState();
loadCalendarEvents();
setInterval(pollState, 3000);
setInterval(loadCalendarEvents, 30000);
</script>
</body>
</html>
"""