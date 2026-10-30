// ⚡️ ZERO-COST DOM DIFFING & ELEMENT CACHING (Sovereign Engine v5.0)
const _domElCache = new Map();
const _domValCache = new Map();

function getEl(id) {
  let el = _domElCache.get(id);
  if (!el) {
    el = document.getElementById(id);
    if (el) _domElCache.set(id, el);
  }
  return el;
}

function setCachedText(id, val) {
  if (val === undefined || val === null) return;
  const s = String(val);
  if (_domValCache.get(id) === s) return;
  const el = getEl(id);
  if (el) {
    el.innerText = s;
    _domValCache.set(id, s);
  }
}

function setCachedClass(id, cls) {
  if (cls === undefined || cls === null) return;
  const k = id + ':c';
  if (_domValCache.get(k) === cls) return;
  const el = getEl(id);
  if (el) {
    el.className = cls;
    _domValCache.set(k, cls);
  }
}

function setCachedValue(id, val) {
  if (val === undefined || val === null) return;
  const k = id + ':v';
  if (_domValCache.get(k) === val) return;
  const el = getEl(id);
  if (el && (!el.matches || !el.matches(':active'))) {
    el.value = val;
    _domValCache.set(k, val);
  }
}

// ⚡️ NATIVE ZERO-DEPENDENCY CANVAS DUAL-AXIS CHART (0 KB external download, 100% offline, 0ms load)
function renderNativeClimateChart(ch) {
  const canvas = document.getElementById('climateCanvas');
  if (!canvas || !ch || ch.length < 2) return;
  try { localStorage.setItem('mb_climate_cache', JSON.stringify(ch)); } catch(_) {}
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  const w = rect.width;
  const h = rect.height;
  if (w <= 0 || h <= 0) return;
  canvas.width = w * dpr;
  canvas.height = h * dpr;
  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  // Background
  ctx.clearRect(0, 0, w, h);

  // Bounds
  let minT = 999, maxT = -999, minP = 9999, maxP = -9999;
  ch.forEach(pt => {
    if (pt.t < minT) minT = pt.t;
    if (pt.t > maxT) maxT = pt.t;
    if (pt.p < minP) minP = pt.p;
    if (pt.p > maxP) maxP = pt.p;
  });
  minT = Math.floor(minT - 1.5); maxT = Math.ceil(maxT + 1.5);
  minP = Math.floor(minP - 2); maxP = Math.ceil(maxP + 2);
  if (maxT === minT) maxT = minT + 2;
  if (maxP === minP) maxP = minP + 2;

  const padL = 36, padR = 36, padT = 24, padB = 26;
  const plotW = w - padL - padR;
  const plotH = h - padT - padB;

  // Grid & Axis labels
  ctx.strokeStyle = '#1e2536';
  ctx.lineWidth = 1;
  ctx.font = '10px JetBrains Mono, monospace';
  for (let i = 0; i <= 4; i++) {
    const y = padT + (plotH / 4) * i;
    ctx.beginPath();
    ctx.moveTo(padL, y);
    ctx.lineTo(w - padR, y);
    ctx.stroke();

    // Temp label left
    const valT = (maxT - (maxT - minT) * (i / 4)).toFixed(1);
    ctx.fillStyle = '#38bdf8';
    ctx.textAlign = 'right';
    ctx.fillText(valT + '°', padL - 4, y + 3);

    // Pres label right
    const valP = Math.round(maxP - (maxP - minP) * (i / 4));
    ctx.fillStyle = '#f59e0b';
    ctx.textAlign = 'left';
    ctx.fillText(valP, w - padR + 4, y + 3);
  }

  // Time labels
  ctx.fillStyle = '#94a3b8';
  ctx.textAlign = 'center';
  const timeStep = Math.max(1, Math.floor(ch.length / 5));
  for (let i = 0; i < ch.length; i += timeStep) {
    const x = padL + (plotW / (ch.length - 1)) * i;
    const timeStr = new Date(ch[i].ts * 1000).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'});
    ctx.fillText(timeStr, x, h - 8);
  }

  // Temperature Line (Blue)
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  ch.forEach((pt, idx) => {
    const x = padL + (plotW / (ch.length - 1)) * idx;
    const y = padT + plotH * (1 - (pt.t - minT) / (maxT - minT));
    if (idx === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();

  // Barometer Line (Amber Dashed)
  ctx.strokeStyle = '#f59e0b';
  ctx.lineWidth = 2;
  ctx.setLineDash([4, 4]);
  ctx.beginPath();
  ch.forEach((pt, idx) => {
    const x = padL + (plotW / (ch.length - 1)) * idx;
    const y = padT + plotH * (1 - (pt.p - minP) / (maxP - minP));
    if (idx === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
  ctx.setLineDash([]);

  // Legend
  ctx.font = '10px JetBrains Mono, monospace';
  ctx.fillStyle = '#38bdf8'; ctx.textAlign = 'left';
  ctx.fillText('● Темп (°C)', padL, 14);
  ctx.fillStyle = '#f59e0b'; ctx.textAlign = 'right';
  ctx.fillText('● Барометр (мм)', w - padR, 14);
}

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
  if (tabId === 'climate') {
    if (!window._lastClimateData || window._lastClimateData.length <= 1) {
      pollState(true);
    } else {
      setTimeout(() => { if (window._lastClimateData) renderNativeClimateChart(window._lastClimateData); }, 60);
    }
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
      alert('Невірний пароль');
    }
  });
}

function doLogout(){
  fetch('/api/v5/auth/logout', {method:'POST'}).then(()=>{
    location.reload();
  });
}

function sendCmd(cmdObj){
  if (cmdObj && cmdObj.set_mode !== undefined) {
    MODES.forEach((_, i) => {
      setCachedClass('mBtn' + i, (cmdObj.set_mode === i) ? 'btn active' : 'btn');
    });
  }
  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({action: 'cmd', payload: cmdObj}));
    toast('Команду передано ⚡️');
    return;
  }
  fetch('/api/v5/command',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(cmdObj)
  }).then(r => r.json()).then(d => {
    toast('Команду передано в чергу');
  }).catch(() => {
    toast('Помилка відправки команди');
    pollState();
  });
}

function pushScreenMsg(){
  const txt = document.getElementById('msgTxt').value.trim();
  if(!txt) return;
  sendCmd({sender:'Studio', msg: txt});
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
  setCachedText('volVal', `${v} / 21`);
}
function applyVolume(v){
  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({action: 'volume', volume: parseInt(v)}));
    toast(`Гучність: ${v} ⚡️`);
    return;
  }
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
  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({action: 'volume', muted: currentMuted}));
    toast(currentMuted ? 'Звук вимкнено (Mute) ⚡️' : 'Звук увімкнено ⚡️');
    return;
  }
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
  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({action: 'volume', touch_beep: currentTouchBeep}));
    toast(currentTouchBeep ? 'Кліки клавіш увімкнено ⚡️' : 'Кліки клавіш вимкнено ⚡️');
    return;
  }
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
  const voice_name = (provider === 'edge') ? edge_voice : (clone_voice_id || edge_voice);
  const voice_id = clone_voice_id;
  fetch('/api/v5/audio/voice-config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({provider, voice_name, voice_id})
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
  sendCmd({media_timer: minutes});
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
  fetch(`/api/v5/media/upload?save_name=${encodeURIComponent(name)}&reaction_slot=${slot}&set_as_saver=false`, {
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
  fetch(`/api/v5/media/trigger/${encodeURIComponent(name)}?saver=false&timer_min=0`, {method:'POST'})
    .then(r => r.json())
    .then(d => {
      toast(`Запущено медіа: ${name}`);
      pollState();
    });
}

function setAsSaver(name){
  fetch(`/api/v5/media/trigger/${encodeURIComponent(name)}?saver=true`, {method:'POST'})
    .then(r => r.json())
    .then(d => {
      toast(`Встановлено як заставку: ${name}`);
      pollState();
    });
}

function spotifyAction(act){
  if (act === 'toggle') {
    act = (spotifyPlayback && spotifyPlayback.playing) ? 'pause' : 'play';
  }
  if (act === 'pause') {
    if (spotifyPlayback.playing && spotifyPlayback.last_sync_ts > 0) {
      const elapsed = Date.now() - spotifyPlayback.last_sync_ts;
      const dur = spotifyPlayback.duration_ms || 0;
      const cur = (spotifyPlayback.progress_ms || 0) + elapsed;
      spotifyPlayback.progress_ms = dur > 0 ? Math.min(cur, dur) : cur;
    }
    spotifyPlayback.playing = false;
    spotifyPlayback.last_sync_ts = Date.now();
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

  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({action: 'spotify', action_name: act}));
    toast(`Spotify: ${act} ⚡️`);
    return;
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
  last_sync_ts: 0,
  lyrics: [],
  single_lyric: ''
};

function tickSpotifyProgress() {
  const spArtistEl = getEl('spArtist');
  const spBarEl = getEl('spProgressBar');
  if (!spArtistEl) return;

  if (!spotifyPlayback.linked || !spotifyPlayback.title) {
    if (spBarEl) spBarEl.style.width = '0%';
    const lb = getEl('spLyricBox');
    if (lb) lb.style.display = 'none';
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
  setCachedText('spArtist', `${spotifyPlayback.artist || 'Unknown'} · ${prog_m}:${prog_s} / ${dur_m}:${dur_s}`);

  if (spBarEl && spotifyPlayback.duration_ms > 0) {
    const pct = Math.min(100, Math.max(0, (currentMs / spotifyPlayback.duration_ms) * 100));
    spBarEl.style.width = pct.toFixed(1) + '%';
  }

  // 🎵 Real-time Synchronized Karaoke Lyrics
  const currentSec = currentMs / 1000.0;
  const lyrics = spotifyPlayback.lyrics;
  const lyricBox = getEl('spLyricBox');

  if (lyrics && lyrics.length > 0 && spotifyPlayback.playing) {
    let activeIdx = -1;
    for (let i = 0; i < lyrics.length; i++) {
      if (lyrics[i].sec <= currentSec) {
        activeIdx = i;
      } else {
        break;
      }
    }
    if (activeIdx >= 0) {
      if (lyricBox) lyricBox.style.display = 'block';
      const prevText = activeIdx > 0 ? lyrics[activeIdx - 1].text : '';
      const currText = lyrics[activeIdx].text;
      const nextText = activeIdx < lyrics.length - 1 ? lyrics[activeIdx + 1].text : '';

      setCachedText('spLyricPrev', prevText);
      setCachedText('spLyric', `🎤 ${currText}`);
      setCachedText('spLyricNext', nextText);
    } else {
      if (lyricBox) lyricBox.style.display = 'block';
      setCachedText('spLyricPrev', '');
      setCachedText('spLyric', '🎵 ...');
      setCachedText('spLyricNext', lyrics[0].text);
    }
  } else if (spotifyPlayback.single_lyric && spotifyPlayback.playing) {
    if (lyricBox) lyricBox.style.display = 'block';
    setCachedText('spLyricPrev', '');
    setCachedText('spLyric', `💬 "${spotifyPlayback.single_lyric}"`);
    setCachedText('spLyricNext', '');
  } else {
    if (lyricBox) lyricBox.style.display = 'none';
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
  const spLyricEl = document.getElementById('spLyric');

  if (sp.linked) {
    if (spLoginBtn) spLoginBtn.style.display = 'none';
    if (spDisconnectBtn) spDisconnectBtn.style.display = 'inline-flex';

    if (sp.title) {
      spotifyPlayback.linked = true;
      spotifyPlayback.title = sp.title;
      spotifyPlayback.artist = sp.artist || '';
      spotifyPlayback.playing = !!sp.playing;
      spotifyPlayback.progress_ms = sp.progress_ms || 0;
      spotifyPlayback.duration_ms = sp.duration_ms || 0;
      spotifyPlayback.last_sync_ts = Date.now();

      if (spTitleEl) spTitleEl.innerText = sp.title;
      if (spDisc) {
        if (sp.cover_url) {
          let discImg = spDisc.querySelector('img');
          if (!discImg) {
            spDisc.innerHTML = `<img src="${sp.cover_url}" alt="Cover" style="width:100%;height:100%;border-radius:50%;object-fit:cover;box-shadow:inset 0 0 10px rgba(0,0,0,0.8);">`;
          } else if (discImg.src !== sp.cover_url) {
            discImg.src = sp.cover_url;
          }
        }
        if (sp.playing) spDisc.classList.add('spinning');
        else spDisc.classList.remove('spinning');
      }

      spotifyPlayback.lyrics = (Array.isArray(sp.lyrics) && sp.lyrics.length > 0) ? sp.lyrics : [];
      spotifyPlayback.single_lyric = sp.lyric || '';

      if (spBadgeEl) {
        spBadgeEl.innerText = sp.playing ? '▶️ PLAYING' : '⏸ PAUSED';
        spBadgeEl.style.color = sp.playing ? 'var(--accent-success)' : 'var(--text-secondary)';
      }
      if (spPlayBtn) spPlayBtn.innerText = sp.playing ? '⏸ Пауза' : '▶️ Пуск';
      tickSpotifyProgress();
    } else {
      spotifyPlayback.linked = true;
      spotifyPlayback.title = '';
      if (spDisc) {
        spDisc.classList.remove('spinning');
        spDisc.innerHTML = '🎵';
      }
      spotifyPlayback.lyrics = []; spotifyPlayback.single_lyric = ''; const _lb = getEl('spLyricBox'); if (_lb) _lb.style.display = 'none';
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
    if (spDisc) {
      spDisc.classList.remove('spinning');
      spDisc.innerHTML = '🎵';
    }
    spotifyPlayback.lyrics = []; spotifyPlayback.single_lyric = ''; const _lb = getEl('spLyricBox'); if (_lb) _lb.style.display = 'none';
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
  if(!confirm('Ви дійсно хочете виконати передподарункове скидання?\n\n• Spotify та Google Calendar буде відключено\n• Тестові події, клімат та історію буде видалено\n• Власника буде закріплено за основним користувачем')) return;
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
  fetch(`/api/v5/media/${encodeURIComponent(name)}`, {method:'DELETE'})
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
    body: JSON.stringify({persona_name: name, system_prompt: prompt, temperature: 0.7})
  }).then(r => r.json()).then(d => {
    toast('Характер ШІ оновлено');
  });
}

function askAiBriefing(){
  toast('Генерую ШІ-брифінг...');
  fetch('/api/v1/ai/briefing', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({custom_note: "", speak: true})
  }).then(r => r.json()).then(d => {
    const box = document.getElementById('aiResponseBox');
    if(box){
      box.innerText = d.ai_forecast || d.screen_text || d.reply || 'Брифінг озвучено на шкатулці!';
      box.style.display = 'block';
    }
    toast('ШІ-брифінг відправлено на шкатулку ⚡️');
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
        box.innerText = d.brief || d.advice || 'Вдалого та світлого дня!';
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
  if(studioWs && studioWs.readyState === WebSocket.OPEN){
    studioWs.send(JSON.stringify({action:'pomo', sub_action:'start', duration_min: min}));
    toast(`Pomodoro: фокус на ${min} хв! ⚡️`);
    return;
  }
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
  if(studioWs && studioWs.readyState === WebSocket.OPEN){
    studioWs.send(JSON.stringify({action:'pomo', sub_action:'stop'}));
    toast('Pomodoro зупинено ⚡️');
    return;
  }
  fetch('/api/v5/routine/pomodoro/stop', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      toast('Pomodoro зупинено');
      updatePomodoroUI(d);
    });
}

function triggerBirthdayCelebration() {
  if (!confirm('Запустити святкову листівку на пристрої та привітання в Telegram?')) return;
  toast('Запуск привітання...');
  fetch('/api/v5/admin/routine/birthday-celebration', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
      alert(d.message || 'Свято активовано!');
    });
}

function sendSilentPush() {
  const input = document.getElementById('silentMsgInput');
  const txt = input.value.trim();
  if (!txt) return;
  if(studioWs && studioWs.readyState === WebSocket.OPEN){
    studioWs.send(JSON.stringify({action:'silent_push', message: txt}));
    toast('Тихе повідомлення надіслано в Inbox ⚡️');
    input.value = '';
    return;
  }
  fetch('/api/v5/admin/routine/silent-push', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ message: txt })
  }).then(r => r.json()).then(d => {
    toast('Тихе повідомлення надіслано в Inbox');
    input.value = '';
  });
}

function loadSchedule() {
  const cont = document.getElementById('scheduleContainer');
  if (!cont) return;
  cont.innerHTML = 'Завантаження розкладу...';
  fetch('/api/v5/admin/routine/calendar-view')
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
const loadNastyaSchedule = loadSchedule;

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
  const isDnd = btn ? btn.innerText.includes('ВКЛ') : false;
  const nextDnd = !isDnd;
  if (studioWs && studioWs.readyState === WebSocket.OPEN) {
    studioWs.send(JSON.stringify({ action: 'dnd', enabled: nextDnd }));
  } else {
    fetch('/api/v5/dnd/toggle', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ enabled: nextDnd })
    }).catch(() => {});
  }
  updateDndUI(nextDnd);
  toast(nextDnd ? 'Режим "Не турбувати" увімкнено' : 'Режим "Не турбувати" вимкнено');
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
    body: JSON.stringify({ summary, time_str: timeVal || '12:00', date_str: '' })
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

function applyState(d) {
  if (!d) return;
  // Cache fast slice to localStorage for 0ms instant startup next time
  try {
    localStorage.setItem('mb_cached_state', JSON.stringify({
      user_role: d.user_role,
      is_online: d.is_online,
      telemetry: d.telemetry,
      spotify: d.spotify,
      google_calendar_linked: d.google_calendar_linked,
      next_calendar_event: d.next_calendar_event,
      alarm: d.alarm,
      pomodoro: d.pomodoro
    }));
  } catch(_) {}

  const on = d.is_online !== undefined ? d.is_online : true;
  setCachedClass('onlineDot', 'pulse ' + (on ? 'on' : 'off'));
  setCachedText('onlineTxt', on ? 'MAGICBOX SOVEREIGN' : 'MAGICBOX OFFLINE');
  if (d.pending_commands !== undefined) setCachedText('queueTxt', d.pending_commands);

  const res = d.resources || {};
  if (res.ram_rss_mb !== undefined || res.rss_mb !== undefined) {
    setCachedText('ramTxt', (res.ram_rss_mb !== undefined ? res.ram_rss_mb : (res.rss_mb || 0)).toFixed(1) + ' MB');
  }
  if (res.storage_used_mb !== undefined || res.disk_used_mb !== undefined) {
    setCachedText('diskTxt', (res.storage_used_mb !== undefined ? res.storage_used_mb : (res.disk_used_mb || 0)).toFixed(1) + ' MB');
  }

  // Spotify Hub Update
  if (d.spotify) updateSpotifyUI(d.spotify);

  const t = d.telemetry || {};
  if (t.t !== undefined) {
    applyTelemetryInPlace(t);
    if (t.heap !== undefined) {
      setCachedText('diagHeap', Math.round(t.heap / 1024));
      setCachedText('diagPsram', Math.round((t.psram_free || 0) / 1024));
      setCachedText('diagRst', t.rst || 'NORMAL');
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
  if(d.google_calendar_linked !== undefined){
    updateGoogleUI(Boolean(d.google_calendar_linked), d.next_calendar_event);
  }

  if (d.user_role === 'admin') {
    const ap = document.getElementById('adminPanel');
    if (ap) ap.style.display = 'block';
    const rb = document.getElementById('roleBadge');
    if (rb) { rb.innerText = '🛡️ ADMIN'; rb.style.color = 'var(--accent-cool)'; }
  } else {
    const ap = document.getElementById('adminPanel');
    if (ap) ap.style.display = 'none';
    const rb = document.getElementById('roleBadge');
    if (rb) { rb.innerText = '✨ USER'; rb.style.color = 'var(--accent-pink)'; }
  }

  const ch = d.climate_history || [];
  if(ch.length > 1){
    window._lastClimateData = ch;
    renderNativeClimateChart(ch);
  }
}

function pollState(forceFull = false){
  pollTickCount++;
  // Never block initial render with heavy 24h SQL series; only load full series on climate tab
  const activeTab = localStorage.getItem('mb_active_tab') || 'music';
  const needFull = forceFull || (activeTab === 'climate' && pollTickCount % 5 === 1);
  const url = needFull ? '/api/v5/state?hours=24' : '/api/v5/state?fast=true';
  fetch(url).then(r => {
    if(r.status === 401) {
      if(!window.Telegram?.WebApp?.initData) {
        document.getElementById('loginModal').style.display = 'flex';
      }
      return null;
    }
    return r.json();
  }).then(d => {
    applyState(d);
  }).catch(()=>{});
}

// ⚡️ ZERO-LATENCY INSTANT HYDRATION (0.00s Paint from Local Cache)
const _cachedStateRaw = localStorage.getItem('mb_cached_state');
if (_cachedStateRaw) {
  try {
    applyState(JSON.parse(_cachedStateRaw));
  } catch(_) {}
}
const _cachedClimate = localStorage.getItem('mb_climate_cache');
if (_cachedClimate) {
  try {
    window._lastClimateData = JSON.parse(_cachedClimate);
  } catch(_) {}
}

// ⚡️ REAL-TIME WEBSOCKET SUITE (15ms latency, 0% CPU, Zero HTTP Polling)
let studioWs = null;

// Restore saved tab or default to 'music'
const savedTab = localStorage.getItem('mb_active_tab') || 'music';
switchTab(savedTab);

// Telegram Mini App Auto-Auth (Zero-Click, Single-Roundtrip Auth + State)
const isTelegramApp = Boolean(window.Telegram?.WebApp?.initData);
let isAuthReady = !isTelegramApp;

if (isTelegramApp) {
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
        isAuthReady = true;
        if (d.state) applyState(d.state);
        loadCalendarEvents();
        initStudioWebSocket();
      }
    })
    .catch(() => {});
  } catch (_) {}
} else {
  // Regular browser emergency access
  pollState();
  loadCalendarEvents();
}
setInterval(loadCalendarEvents, 30000);

function scheduleAdaptivePoll(){
  // If WebSocket is active, we don't need aggressive HTTP polling!
  const wsActive = Boolean(studioWs && studioWs.readyState === WebSocket.OPEN);
  const delay = wsActive ? 30000 : ((spotifyPlayback && spotifyPlayback.playing && spotifyPlayback.linked) ? 2000 : 7000);
  setTimeout(() => {
    if (!wsActive && isAuthReady) pollState();
    scheduleAdaptivePoll();
  }, delay);
}
scheduleAdaptivePoll();


// ⚡️ REAL-TIME WEBSOCKET SUITE (15ms latency, 0% CPU, Zero HTTP Polling)

function applyTelemetryInPlace(t) {
  if (!t) return;
  if (t.fw || t.rssi) {
    setCachedText('fwBadge', `${t.fw || 'v5.0'} (${t.rssi || -60} dBm)`);
  }
  const tStr = (t.t !== undefined ? t.t.toFixed(1) : '--') + ' °C';
  const hStr = (t.h !== undefined ? t.h.toFixed(0) : '--') + ' %';
  const pStr = `${(t.p || 0).toFixed(0)} мм (${(t.p_diff || 0) >= 0 ? '+' : ''}${(t.p_diff || 0).toFixed(1)})`;
  const mStr = `${t.m_name || '--'} (${t.b_pct || 0}%)`;

  setCachedText('kTemp', tStr);
  setCachedText('kHum', hStr);
  setCachedText('kPres', pStr);
  setCachedText('kMode', mStr);

  setCachedText('kTempCard', tStr);
  setCachedText('kHumCard', hStr);
  setCachedText('kPresCard', pStr);

  if (t.m !== undefined) {
    MODES.forEach((_, i) => {
      setCachedClass('mBtn' + i, (t.m === i) ? 'btn active' : 'btn');
    });
  }

  if (t.vol !== undefined) {
    setCachedValue('volSlider', t.vol);
    setCachedText('volVal', `${t.vol} / 21`);
  }
  if (t.muted !== undefined) {
    currentMuted = Boolean(t.muted);
    updateMuteUI(currentMuted);
  }
  if (t.t_beep !== undefined) {
    currentTouchBeep = Boolean(t.t_beep);
    updateTouchBeepUI(currentTouchBeep);
  }
  if (t.dnd !== undefined) {
    updateDndUI(Boolean(t.dnd));
  }
}

let wsReconnectDelay = 2000;
let wsReconnectTimer = null;
let wsConnecting = false;

async function initStudioWebSocket() {
  if (isTelegramApp && !isAuthReady) return;
  if (document.visibilityState === 'hidden') return;
  if (studioWs && (studioWs.readyState === WebSocket.OPEN || studioWs.readyState === WebSocket.CONNECTING)) return;
  if (wsConnecting) return;
  wsConnecting = true;

  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
  let wsUrl = `${proto}//${location.host}/ws/dashboard`;

  try {
    const res = await fetch('/api/v5/auth/ws-ticket', { method: 'POST', credentials: 'include' });
    if (res.status === 401) {
      wsConnecting = false;
      isAuthReady = false;
      setCachedClass('onlineDot', 'pulse');
      setCachedText('onlineTxt', 'ПОТРІБНА АВТОРИЗАЦІЯ 🔒');
      const lm = getEl('loginModal');
      if (lm) {
        lm.style.display = 'flex';
        const pwd = getEl('pwdInput');
        if (pwd) {
          pwd.value = '';
          setTimeout(() => pwd.focus(), 150);
        }
      }
      return;
    }
    if (res.ok) {
      const data = await res.json();
      if (data && data.ticket) {
        wsUrl += `?ticket=${encodeURIComponent(data.ticket)}`;
      }
    }
  } catch (_) {}

  try {
    studioWs = new WebSocket(wsUrl);
    wsConnecting = false;
    studioWs.onopen = () => {
      wsReconnectDelay = 2000;
      clearTimeout(wsReconnectTimer);
      setCachedClass('onlineDot', 'pulse on');
      setCachedText('onlineTxt', 'MAGICBOX ⚡️ REALTIME');
    };
    studioWs.onmessage = (evt) => {
      try {
        const d = JSON.parse(evt.data);
        if (d.type === 'init' || d.type === 'state') {
          applyState(d.state || d);
        } else if (d.type === 'telemetry') {
          if (d.telemetry) applyTelemetryInPlace(d.telemetry);
        } else if (d.type === 'spotify') {
          if (d.spotify) updateSpotifyUI(d.spotify);
        } else if (d.type === 'dnd_ack') {
          updateDndUI(Boolean(d.dnd));
        } else if (d.type === 'pomo_ack') {
          if (d.pomodoro) updatePomodoroUI(d.pomodoro);
        } else if (d.type === 'command_queued') {
          if (d.pending_commands !== undefined) setCachedText('queueTxt', d.pending_commands);
        }
      } catch(_) {}
    };
    studioWs.onclose = (evt) => {
      studioWs = null;
      wsConnecting = false;
      setCachedClass('onlineDot', 'pulse');
      setCachedText('onlineTxt', 'MAGICBOX · СИНХРОНІЗАЦІЯ');
      if (evt && evt.code === 1008) {
        isAuthReady = false;
        setCachedClass('onlineDot', 'pulse');
        setCachedText('onlineTxt', 'ПОТРІБНА АВТОРИЗАЦІЯ 🔒');
        toast('Сесію завершено (1008 Unauthorized). Авторизуйтесь знову 🔒');
        const lm = getEl('loginModal');
        if (lm) {
          lm.style.display = 'flex';
          const pwd = getEl('pwdInput');
          if (pwd) {
            pwd.value = '';
            setTimeout(() => pwd.focus(), 150);
          }
        }
        return;
      }
      if (document.visibilityState !== 'hidden') {
        clearTimeout(wsReconnectTimer);
        wsReconnectTimer = setTimeout(initStudioWebSocket, wsReconnectDelay);
        wsReconnectDelay = Math.min(wsReconnectDelay * 1.5, 30000);
      }
    };
    studioWs.onerror = () => {
      try { studioWs.close(); } catch(_) {}
    };
  } catch(_) {
    wsConnecting = false;
    if (document.visibilityState !== 'hidden') {
      clearTimeout(wsReconnectTimer);
      wsReconnectTimer = setTimeout(initStudioWebSocket, wsReconnectDelay);
      wsReconnectDelay = Math.min(wsReconnectDelay * 1.5, 30000);
    }
  }
}

document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'visible') {
    if (isAuthReady && (!studioWs || studioWs.readyState !== WebSocket.OPEN)) {
      wsReconnectDelay = 2000;
      initStudioWebSocket();
      pollState();
    }
  } else {
    clearTimeout(wsReconnectTimer);
    if (studioWs) {
      try { studioWs.close(); } catch(_) {}
      studioWs = null;
    }
  }
});

window.addEventListener('resize', () => {
  if (window._lastClimateData) renderNativeClimateChart(window._lastClimateData);
});

// У Telegram Mini App сокет запускається ПІСЛЯ завершення авторизації /api/v5/auth/tg-webapp
if (!isTelegramApp) {
  initStudioWebSocket();
}

// WebSocket Realtime Architecture Active

// 📦 PWA Service Worker Registration
if ('serviceWorker' in navigator && !isTelegramApp) {
  window.addEventListener('load', () => {
    navigator.serviceWorker.register('/sw.js').then((reg) => {
      console.log('SmartLamp PWA ServiceWorker active:', reg.scope);
    }).catch((err) => {
      console.warn('SmartLamp PWA SW registration skipped:', err);
    });
  });
}


// --- РІВЕНЬ 3: WEB PUSH NOTIFICATIONS (VAPID) ---
function urlBase64ToUint8Array(base64String) {
  const padding = '='.repeat((4 - base64String.length % 4) % 4);
  const base64 = (base64String + padding).replace(/\-/g, '+').replace(/_/g, '/');
  const rawData = window.atob(base64);
  const outputArray = new Uint8Array(rawData.length);
  for (let i = 0; i < rawData.length; ++i) {
    outputArray[i] = rawData.charCodeAt(i);
  }
  return outputArray;
}

async function toggleWebPush() {
  if (!('serviceWorker' in navigator) || !('PushManager' in window)) {
    alert('Ваш браузер не підтримує Web Push сповіщення');
    return;
  }
  const perm = await Notification.requestPermission();
  if (perm !== 'granted') {
    toast('Дозвіл на сповіщення не надано');
    return;
  }
  try {
    const reg = await navigator.serviceWorker.ready;
    const keyRes = await fetch('/api/v5/push/vapid-key');
    const keyData = await keyRes.json();
    if (!keyData.ok || !keyData.publicKey) {
      toast('VAPID ключ наразі генерується сервером...');
      return;
    }
    const appKey = urlBase64ToUint8Array(keyData.publicKey);
    const sub = await reg.pushManager.subscribe({
      userVisibleOnly: true,
      applicationServerKey: appKey
    });
    const saveRes = await fetch('/api/v5/push/subscribe', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(sub)
    });
    const saveJson = await saveRes.json();
    if (saveJson.ok) {
      toast('🔔 Web Push сповіщення успішно активовано!');
      const btn = document.getElementById('pushSubscribeBtn');
      if (btn) btn.innerText = '🔔 Push: АКТИВНО';
      const testBtn = document.getElementById('pushTestBtn');
      if (testBtn) testBtn.style.display = 'inline-block';
    }
  } catch (e) {
    console.error(e);
    toast('Помилка підписки: ' + e.message);
  }
}

async function testWebPush() {
  toast('Відправка тестового Push...');
  try {
    const res = await fetch('/api/v5/push/test', { method: 'POST' });
    const d = await res.json();
    toast(`Тестовий push відправлено (${d.delivered_count || 0} клієнтів)`);
  } catch (_) {
    toast('Помилка тестування Push');
  }
}
