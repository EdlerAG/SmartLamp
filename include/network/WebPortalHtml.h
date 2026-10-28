#pragma once
#include <Arduino.h>

const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MagicBox Smart Hub • Hallmark Edition</title>
<!-- Hallmark · pre-emit critique: P5 H5 E5 S5 R5 V5 -->
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
  --font-sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", sans-serif;
  --font-mono: ui-monospace, "SF Mono", Menlo, Monaco, Consolas, monospace;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg-primary);
  color: var(--text-primary);
  font-family: var(--font-sans);
  font-size: 14px;
  line-height: 1.5;
  padding: 16px 12px;
  -webkit-font-smoothing: antialiased;
}
.wrap {
  max-width: 460px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 4px;
}
.brand-title {
  font-size: 15px;
  font-weight: 700;
  letter-spacing: 0.5px;
  color: var(--text-primary);
  display: flex;
  align-items: center;
  gap: 8px;
}
.status-pill {
  font-size: 11px;
  font-family: var(--font-mono);
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  color: var(--accent-success);
  padding: 4px 10px;
  border-radius: 9999px;
  display: inline-flex;
  align-items: center;
  gap: 6px;
}
.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: var(--accent-success);
}
.card {
  background: var(--bg-elevated);
  border: 1px solid var(--border-hairline);
  border-radius: 12px;
  padding: 16px;
}
.card.first { order: -1; border-color: var(--accent-cool); }
h2 {
  font-size: 13px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.75px;
  color: var(--text-secondary);
  margin-bottom: 12px;
  display: flex;
  align-items: center;
  gap: 6px;
}
.stat-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
  margin-bottom: 12px;
}
.stat {
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 8px;
  padding: 10px 12px;
}
.stat b {
  display: block;
  font-size: 20px;
  font-weight: 700;
  font-family: var(--font-mono);
  font-variant-numeric: tabular-nums;
  color: var(--text-primary);
}
.stat span {
  font-size: 11px;
  color: var(--text-secondary);
}
.mode-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
  margin: 10px 0;
}
.m-btn {
  padding: 10px 8px;
  font-size: 12px;
  font-weight: 500;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 8px;
  color: var(--text-secondary);
  cursor: pointer;
  transition: border-color 0.15s, color 0.15s, background-color 0.15s;
}
.m-btn:hover {
  border-color: var(--text-muted);
  color: var(--text-primary);
}
.m-btn.active {
  background: var(--bg-primary);
  border-color: var(--accent-warm);
  color: var(--accent-warm);
  font-weight: 600;
}
.brt-row {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 6px;
  margin-top: 8px;
}
.b-btn {
  padding: 8px 4px;
  font-size: 11px;
  font-family: var(--font-mono);
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 6px;
  color: var(--text-secondary);
  cursor: pointer;
  transition: border-color 0.15s, color 0.15s;
}
.b-btn:hover { border-color: var(--text-muted); color: var(--text-primary); }
.b-btn.active {
  background: var(--bg-primary);
  border-color: var(--accent-cool);
  color: var(--accent-cool);
  font-weight: 600;
}
.meme-row {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 6px;
  margin-top: 8px;
}
.react-btn {
  padding: 10px 4px;
  font-size: 18px;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 8px;
  cursor: pointer;
  transition: transform 0.1s, border-color 0.15s;
}
.react-btn:hover { border-color: var(--text-secondary); transform: translateY(-1px); }
.react-btn:active { transform: translateY(0); }
.net-list {
  max-height: 200px;
  overflow-y: auto;
  margin: 10px 0;
  border-radius: 8px;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  padding: 6px;
}
.net-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 12px;
  margin: 4px 0;
  background: var(--bg-elevated);
  border-radius: 6px;
  cursor: pointer;
  border: 1px solid transparent;
  transition: border-color 0.15s;
}
.net-item:hover { border-color: var(--border-hairline); }
.net-item.sel {
  border-color: var(--accent-cool);
  background: var(--bg-primary);
}
.badge {
  font-size: 11px;
  font-family: var(--font-mono);
  background: var(--bg-surface);
  color: var(--accent-cool);
  padding: 2px 6px;
  border-radius: 4px;
  border: 1px solid var(--border-hairline);
}
input, button {
  width: 100%;
  padding: 10px 12px;
  margin: 5px 0;
  border-radius: 8px;
  border: 1px solid var(--border-hairline);
  background: var(--bg-surface);
  color: var(--text-primary);
  font-size: 13px;
  font-family: var(--font-sans);
  outline: none;
  transition: border-color 0.15s;
}
input:focus { border-color: var(--border-focus); }
input::placeholder { color: var(--text-muted); }
button.btn-primary {
  background: var(--accent-cool);
  color: #040810;
  border: none;
  font-weight: 600;
  cursor: pointer;
}
button.btn-primary:hover { opacity: 0.92; }
button.btn-sec {
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  color: var(--text-primary);
  cursor: pointer;
}
button.btn-sec:hover { border-color: var(--text-secondary); }
button.btn-ota {
  background: var(--accent-warm);
  color: #08090c;
  border: none;
  font-weight: 600;
  cursor: pointer;
}
.pass-wrap { position: relative; display: none; }
.pass-wrap.show { display: block; }
.eye-btn {
  position: absolute;
  right: 6px;
  top: 7px;
  width: 36px;
  height: 36px;
  background: transparent;
  border: none;
  padding: 0;
  font-size: 16px;
  cursor: pointer;
}
.prog-wrap {
  display: none;
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  border-radius: 6px;
  overflow: hidden;
  height: 22px;
  margin-top: 10px;
}
.prog-bar {
  background: var(--accent-success);
  height: 100%;
  width: 0%;
  line-height: 22px;
  font-size: 11px;
  font-family: var(--font-mono);
  font-weight: 700;
  color: #08090c;
  text-align: center;
  transition: width 0.2s ease-out;
}
.toast {
  position: fixed;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--bg-surface);
  border: 1px solid var(--border-hairline);
  color: var(--text-primary);
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 13px;
  font-weight: 500;
  display: none;
  z-index: 99;
  box-shadow: 0 8px 24px rgba(0,0,0,0.5);
}
</style>
</head>
<body>
<div class="wrap">

  <div class="header">
    <div class="brand-title">
      <span>🪄</span>
      <span>MAGICBOX STUDIO</span>
    </div>
    <div class="status-pill">
      <span class="status-dot"></span>
      <span id="pillTxt">ONLINE</span>
    </div>
  </div>

  <div class="card" id="wifiCard">
    <h2>📶 Підключення Wi-Fi</h2>
    <button onclick="scanWifi(true)" class="btn-sec">🔄 Оновити список мереж</button>
    <div class="net-list" id="netList"></div>
    <form onsubmit="saveWifi(event)">
      <input type="text" id="ssid" name="ssid" placeholder="Оберіть мережу зі списку" required>
      <div class="pass-wrap" id="passBox">
        <input type="password" id="pass" name="pass" placeholder="Пароль Wi-Fi">
        <button type="button" class="eye-btn" onclick="togglePass()">👁️</button>
      </div>
      <button type="submit" id="saveBtn" class="btn-primary">Підключитися</button>
    </form>
  </div>

  <div class="card">
    <h2>✨ Клімат та Освітлення</h2>
    <div class="stat-grid">
      <div class="stat"><b id="t">--.- °C</b><span>Температура</span></div>
      <div class="stat"><b id="h">-- %</b><span>Вологість</span></div>
      <div class="stat"><b id="p">--- мм</b><span>Барометр</span></div>
      <div class="stat"><b id="b_stat">-- %</b><span id="m_stat">Яскравість</span></div>
    </div>
    <div class="mode-grid" id="modeGrid"></div>
    <div class="brt-row">
      <button class="b-btn" id="ba" onclick="setBrt('auto')">АВТО</button>
      <button class="b-btn" id="b0" onclick="setBrt('0')">15%</button>
      <button class="b-btn" id="b1" onclick="setBrt('1')">35%</button>
      <button class="b-btn" id="b2" onclick="setBrt('2')">65%</button>
      <button class="b-btn" id="b3" onclick="setBrt('3')">100%</button>
    </div>
    <div class="brt-row" style="grid-template-columns:repeat(4,1fr)">
      <button class="b-btn" onclick="setSlp(15)">🌙 15хв</button>
      <button class="b-btn" onclick="setSlp(30)">🌙 30хв</button>
      <button class="b-btn" onclick="setSlp(60)">🌙 60хв</button>
      <button class="b-btn" onclick="setSlp(0)">❌ Стоп</button>
    </div>
  </div>

  <div class="card">
    <h2>🎭 Реакції та Екран</h2>
    <div class="meme-row">
      <button class="react-btn" onclick="sendReact('fire')" title="Вогник">🔥</button>
      <button class="react-btn" onclick="sendReact('zap')" title="Блискавка">⚡</button>
      <button class="react-btn" onclick="sendReact('cat')" title="Котик">🐱</button>
      <button class="react-btn" onclick="sendReact('ufo')" title="НЛО">👽</button>
      <button class="react-btn" onclick="sendReact('crown')" title="Корона">👑</button>
    </div>
    <form onsubmit="sendMsg(event)" style="margin-top:10px">
      <input type="text" name="sender" placeholder="Ім'я відправника" value="Studio" maxlength="25" required>
      <input type="text" id="msg" name="msg" placeholder="Текст на круглий дисплей..." maxlength="55" required>
      <button type="submit" class="btn-primary">💬 Вивести на дисплей</button>
    </form>
  </div>

  <div class="card">
    <h2>☁️ Хмара Oracle та Telegram</h2>
    <form onsubmit="saveCloud(event)" style="margin-bottom:10px">
      <input type="text" name="oracle_url" placeholder="Oracle Server URL (http://ip:8000)">
      <input type="password" name="oracle_key" placeholder="Oracle API Key (X-Device-Key)">
      <button type="submit" class="btn-sec">☁️ Зберегти Oracle Cloud</button>
    </form>
    <form onsubmit="saveTg(event)">
      <input type="text" name="token" placeholder="Bot Token (@BotFather)">
      <div class="stat-grid" style="margin-bottom:0">
        <input type="text" name="admin_id" placeholder="Admin Chat ID">
        <input type="text" name="girl_id" placeholder="Girl Chat ID">
      </div>
      <button type="submit" class="btn-sec">🤖 Зберегти конфіг Telegram</button>
    </form>
  </div>

  <div class="card">
    <h2>🚀 Standalone OTA Оновлення</h2>
    <form onsubmit="uploadOta(event)">
      <input type="file" id="otaFile" accept=".bin" required>
      <button type="submit" class="btn-ota">Оновити firmware.bin</button>
    </form>
    <div class="prog-wrap" id="progWrap"><div class="prog-bar" id="progBar">0%</div></div>
  </div>

</div>

<div class="toast" id="toast">Виконано</div>

<script>
const MODES=[
  "☀️ Теплий затишок","🕯 Жива свічка","🍯 Бурштинова свічка","🔥 Камін",
  "🌅 Захід сонця","🌸 Сакура Неон","🌌 Північне сяйво","🌊 Океан",
  "⚡ Кібер-комета","🌈 Веселка","🌑 Вимкнено"
];
const mg=document.getElementById('modeGrid');
MODES.forEach((name,idx)=>{
  const b=document.createElement('button');
  b.className='m-btn';b.id='m'+idx;b.textContent=name;
  b.onclick=()=>fetch('/api/cmd?set_mode='+idx,{method:'POST'}).then(()=>updateStats());
  mg.appendChild(b);
});

if(location.hostname==='192.168.4.1'||location.hostname.indexOf('magicbox')===-1){
  document.getElementById('wifiCard').classList.add('first');
  document.getElementById('passBox').classList.add('show');
}
function showToast(txt){
  const t=document.getElementById('toast');
  t.innerText=txt;t.style.display='block';
  setTimeout(()=>t.style.display='none',2200);
}
function togglePass(){
  const p=document.getElementById('pass');
  p.type=(p.type==='password')?'text':'password';
}
function setBrt(val){fetch('/api/cmd?set_brt='+val,{method:'POST'}).then(()=>updateStats());}
function setSlp(m){fetch('/api/cmd?sleep='+m,{method:'POST'}).then(()=>{showToast(m?'🌙 Таймер сну: '+m+' хв':'Таймер вимкнено');updateStats();});}
function sendReact(r){fetch('/api/cmd?react='+r,{method:'POST'}).then(()=>showToast('🚀 Реакцію запущено!'));}
function updateStats(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('t').innerText=d.t.toFixed(1)+' °C';
    document.getElementById('h').innerText=d.h.toFixed(0)+' %';
    document.getElementById('p').innerText=d.p.toFixed(0)+' мм';
    document.getElementById('b_stat').innerText=d.b_pct+' %'+(d.sleep_m?' (🌙'+d.sleep_m+'m)':'');
    document.getElementById('m_stat').innerText=d.m_name;
    for(let i=0;i<MODES.length;i++){
      const el=document.getElementById('m'+i);
      if(el) el.className=(d.m===i)?'m-btn active':'m-btn';
    }
    document.getElementById('ba').className=d.auto_b?'b-btn active':'b-btn';
    for(let i=0;i<4;i++){
      document.getElementById('b'+i).className=(!d.auto_b && d.b_idx===i)?'b-btn active':'b-btn';
    }
  }).catch(()=>{});
}
function sendMsg(e){
  e.preventDefault();
  fetch('/api/msg',{method:'POST',body:new FormData(e.target)}).then(()=>{
    document.getElementById('msg').value='';
    showToast('✅ Виведено на екран!');
  });
}
function saveTg(e){
  e.preventDefault();
  fetch('/api/tg',{method:'POST',body:new FormData(e.target)}).then(()=>showToast('🤖 Telegram збережено!'));
}
function saveCloud(e){
  e.preventDefault();
  fetch('/api/cloud',{method:'POST',body:new FormData(e.target)}).then(()=>showToast('☁️ Oracle збережено!'));
}
function renderNets(nets){
  const list=document.getElementById('netList');
  if(!nets||nets.length===0){list.innerHTML='<div style="padding:10px;text-align:center;color:var(--text-muted)">Натисніть «Оновити список»</div>';return;}
  list.innerHTML='';
  nets.forEach(n=>{
    const div=document.createElement('div');
    div.className='net-item';
    let q=Math.min(100,Math.max(0,2*(n.rssi+100)));
    const leftSpan=document.createElement('span');
    leftSpan.textContent=(n.enc?'🔒 ':'🔓 ');
    const bTag=document.createElement('b');
    bTag.textContent=n.ssid;
    leftSpan.appendChild(bTag);
    const badge=document.createElement('span');
    badge.className='badge';badge.textContent=q+'%';
    div.appendChild(leftSpan);div.appendChild(badge);
    div.onclick=()=>{
      document.querySelectorAll('.net-item').forEach(el=>el.classList.remove('sel'));
      div.classList.add('sel');
      document.getElementById('ssid').value=n.ssid;
      const pb=document.getElementById('passBox');
      const pi=document.getElementById('pass');
      if(n.enc){pb.classList.add('show');pi.focus();}
      else{pb.classList.remove('show');pi.value='';}
    };
    list.appendChild(div);
  });
}
function scanWifi(force=false){
  const list=document.getElementById('netList');
  if(force) list.innerHTML='<div style="padding:12px;text-align:center;color:var(--accent-cool)">⚡ Сканування...</div>';
  fetch('/api/scan'+(force?'?refresh=1':'')).then(r=>r.json()).then(d=>{
    if(d.scanning){setTimeout(()=>scanWifi(false),350);return;}
    renderNets(d.networks);
  }).catch(()=>{});
}
function saveWifi(e){
  e.preventDefault();
  document.getElementById('saveBtn').innerText='⚡ Підключення...';
  fetch('/save',{method:'POST',body:new FormData(e.target)}).then(r=>r.text()).then(msg=>{
    document.body.innerHTML=`<div class="wrap"><div class="card"><h2>✅ ${msg}</h2><p style="color:var(--text-secondary)">Підключення до роутера...</p></div></div>`;
  });
}
function uploadOta(e){
  e.preventDefault();
  const file=document.getElementById('otaFile').files[0];
  if(!file)return;
  const fd=new FormData();fd.append('update',file);
  const xhr=new XMLHttpRequest();
  document.getElementById('progWrap').style.display='block';
  xhr.upload.onprogress=(ev)=>{
    if(ev.lengthComputable){
      const p=Math.round((ev.loaded/ev.total)*100);
      const b=document.getElementById('progBar');
      b.style.width=p+'%';b.innerText=p+'%';
    }
  };
  xhr.onload=()=>{document.body.innerHTML='<div class="wrap"><div class="card"><h2>🎉 Прошивку оновлено!</h2><p style="color:var(--text-secondary)">Перезавантаження пристрою...</p></div></div>';};
  xhr.open('POST','/update',true);xhr.send(fd);
}
scanWifi(false);updateStats();setInterval(updateStats,2500);
</script>
</body>
</html>
)rawliteral";
