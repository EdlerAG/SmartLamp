# Файл: oracle_server/app/web/portal_html.py
PORTAL_HTML: str = r"""<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MagicBox Sovereign Studio • Hallmark v3.8</title>
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
</style>
</head>
<body>

<div id="loginModal">
  <div class="login-box">
    <div style="font-size:32px;margin-bottom:8px">🪄</div>
    <h2 style="font-size:16px;margin-bottom:6px">MagicBox Studio • Hallmark v3.8</h2>
    <p style="font-size:12px;color:var(--text-secondary);margin-bottom:16px">Введіть пароль або X-Device-Key</p>
    <input type="password" id="pwdInput" placeholder="Пароль...">
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
          <b id="onlineTxt" style="font-size:14px;letter-spacing:0.5px">MAGICBOX SOVEREIGN STUDIO</b>
          <span class="mono" style="font-size:11px;color:var(--accent-cool)" id="fwBadge">FW: --</span>
        </div>
        <div class="mono" style="font-size:11px;color:var(--text-secondary);margin-top:3px">
          Server RAM: <b id="ramTxt" style="color:var(--accent-success)">-- MB</b> |
          Сховище: <b id="diskTxt">-- MB</b> |
          Черга: <b id="queueTxt">0</b>
        </div>
      </div>
    </div>
  </div>

  <div class="grid-4">
    <div class="kpi"><span>Температура в кімнаті</span><b id="kTemp" style="color:var(--accent-cool)">--.- °C</b></div>
    <div class="kpi"><span>Вологість повітря</span><b id="kHum" style="color:var(--accent-success)">-- %</b></div>
    <div class="kpi"><span>Барометр (Тренд)</span><b id="kPres" style="color:var(--accent-warm)">--- мм</b></div>
    <div class="kpi"><span>Режим та Яскравість</span><b id="kMode" style="font-size:16px">--</b></div>
  </div>

  <!-- Spotify Vinyl Hub (v4.0) -->
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

  <div class="split-2">
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

    <div class="card">
      <h3><span>🎞 Студія Кастомних Гіфок (.mbx AmbiLight)</span></h3>
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
  </div>

  <div class="split-2">
    <div class="card">
      <h3><span>📈 Історія Клімату та Барометра (SQLite WAL)</span></h3>
      <div id="climateChart" style="height:260px"></div>
    </div>

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

let personaLoaded = false;
function pollState(){
  fetch('/api/v5/state?hours=24').then(r => {
    if(r.status === 401) {
      document.getElementById('loginModal').style.display = 'flex';
      return null;
    }
    return r.json();
  }).then(d => {
    if(!d) return;

    const on = d.is_online;
    document.getElementById('onlineDot').className = 'pulse ' + (on ? 'on' : 'off');
    document.getElementById('onlineTxt').innerText = on ? 'MAGICBOX SOVEREIGN STUDIO' : 'MAGICBOX OFFLINE (BLEKAUT)';
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
      document.getElementById('fwBadge').innerText = `${t.fw || 'v3.8'} | ${t.rssi || -60} dBm`;
      document.getElementById('kTemp').innerText = t.t.toFixed(1) + ' °C';
      document.getElementById('kHum').innerText = t.h.toFixed(0) + ' %';
      document.getElementById('kPres').innerText = `${t.p.toFixed(0)} мм (${t.p_diff>=0?'+':''}${(t.p_diff||0).toFixed(1)})`;
      document.getElementById('kMode').innerText = `${t.m_name || '--'} (${t.b_pct || 0}%)`;
      MODES.forEach((_,i)=>{
        const el = document.getElementById('mBtn'+i);
        if(el) el.className = (t.m === i) ? 'btn active' : 'btn';
      });
    }

    if(!personaLoaded && d.ai_persona){
      personaLoaded = true;
      document.getElementById('aiName').value = d.ai_persona.name;
      document.getElementById('aiPrompt').value = d.ai_persona.prompt;
    }

    renderGallery(d.custom_media || []);

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
setInterval(pollState, 3000);
</script>
</body>
</html>
"""
