#pragma once
#include <Arduino.h>

const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="uk"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MagicBox Smart Hub</title>
<style>
*{box-sizing:border-box}body{background:#090d16;color:#f8fafc;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;margin:0;padding:12px;text-align:center}
.wrap{max-width:440px;margin:0 auto;display:flex;flex-direction:column}
.card{background:#162032;width:100%;margin:6px 0;padding:16px;border-radius:18px;border:1px solid #28354a;box-shadow:0 10px 25px rgba(0,0,0,0.45)}
.card.first{order:-1;border-color:#38bdf8}
h2{color:#38bdf8;margin:0 0 10px 0;font-size:15px;text-transform:uppercase;letter-spacing:0.5px}
.stat-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px}
.stat{background:#0b1120;padding:10px;border-radius:12px;border:1px solid #1e293b}
.stat b{display:block;font-size:18px;color:#f8fafc}
.stat span{font-size:11px;color:#94a3b8}
.mode-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin:8px 0}
.m-btn{padding:10px 8px;font-size:13px;background:#0b1120;border:1px solid #334155;border-radius:10px;color:#e2e8f0}
.m-btn.active{background:#0284c7;border-color:#7dd3fc;color:#fff;font-weight:700}
.brt-row{display:grid;grid-template-columns:repeat(5,1fr);gap:5px;margin-top:8px}
.b-btn{padding:9px 4px;font-size:12px;background:#0b1120;border:1px solid #334155;border-radius:8px}
.b-btn.active{background:#10b981;border-color:#6ee7b7;color:#fff}
.meme-row{display:grid;grid-template-columns:repeat(5,1fr);gap:5px;margin-top:8px}
.net-list{max-height:200px;overflow-y:auto;text-align:left;margin:8px 0;border-radius:12px;background:#0b1120;padding:6px}
.net-item{display:flex;justify-content:space-between;align-items:center;padding:11px 12px;margin:4px 0;background:#1e293b;border-radius:9px;cursor:pointer;border:1px solid transparent}
.net-item.sel{background:#0284c7;border-color:#7dd3fc}
.badge{font-size:11px;background:#0b1120;color:#38bdf8;padding:3px 7px;border-radius:6px;font-weight:bold}
input,button{width:100%;padding:12px;margin:5px 0;border-radius:11px;border:1px solid #334155;background:#0b1120;color:#fff;font-size:14px;outline:none}
button{background:#0284c7;border:none;font-weight:600;cursor:pointer}
button:active{transform:scale(0.98)}
.btn-sec{background:#334155}
.btn-ota{background:#10b981}
.pass-wrap{position:relative;display:none}
.pass-wrap.show{display:block}
.eye-btn{position:absolute;right:6px;top:9px;width:40px;height:38px;background:transparent;padding:0;margin:0;font-size:18px}
.prog-wrap{display:none;background:#0b1120;border-radius:8px;overflow:hidden;height:22px;margin-top:8px}
.prog-bar{background:#10b981;height:100%;width:0%;line-height:22px;font-size:12px;font-weight:bold}
.toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:#10b981;color:#fff;padding:10px 20px;border-radius:20px;font-size:14px;font-weight:600;display:none;z-index:99}
</style></head><body>
<div class="wrap">

<div class="card" id="wifiCard">
  <h2>📶 Підключення до Wi-Fi</h2>
  <button onclick="scanWifi(true)" class="btn-sec">🔄 Оновити список мереж</button>
  <div class="net-list" id="netList"></div>
  <form onsubmit="saveWifi(event)">
    <input type="text" id="ssid" name="ssid" placeholder="Оберіть мережу зі списку вище" required>
    <div class="pass-wrap" id="passBox">
      <input type="password" id="pass" name="pass" placeholder="Введіть пароль від Wi-Fi">
      <button type="button" class="eye-btn" onclick="togglePass()">👁️</button>
    </div>
    <button type="submit" id="saveBtn">Підключитися</button>
  </form>
</div>

<div class="card">
  <h2>✨ Клімат та Освітлення</h2>
  <div class="stat-grid">
    <div class="stat"><b id="t">-- °C</b><span>Температура</span></div>
    <div class="stat"><b id="h">-- %</b><span>Вологість</span></div>
    <div class="stat"><b id="p">-- мм</b><span>Барометр</span></div>
    <div class="stat"><b id="b_stat">-- %</b><span id="m_stat">Яскравість лампи</span></div>
  </div>
  <div class="mode-grid" id="modeGrid"></div>
  <div class="brt-row">
    <button class="b-btn" id="ba" onclick="setBrt('auto')">🤖 Авто</button>
    <button class="b-btn" id="b0" onclick="setBrt('0')">15%</button>
    <button class="b-btn" id="b1" onclick="setBrt('1')">35%</button>
    <button class="b-btn" id="b2" onclick="setBrt('2')">65%</button>
    <button class="b-btn" id="b3" onclick="setBrt('3')">100%</button>
  </div>
  <div class="brt-row" style="grid-template-columns:repeat(4,1fr)">
    <button class="b-btn" onclick="setSlp(15)">🌙 15хв</button>
    <button class="b-btn" onclick="setSlp(30)">🌙 30хв</button>
    <button class="b-btn" onclick="setSlp(60)">🌙 60хв</button>
    <button class="b-btn" onclick="setSlp(0)">❌ Таймер</button>
  </div>
</div>

<div class="card">
  <h2>🎭 Мем-Реакції та Повідомлення</h2>
  <div class="meme-row">
    <button onclick="sendReact('fire')" style="background:#ea580c">🔥</button>
    <button onclick="sendReact('zap')" style="background:#0284c7">⚡</button>
    <button onclick="sendReact('cat')" style="background:#059669">🐱</button>
    <button onclick="sendReact('ufo')" style="background:#7c3aed">👽</button>
    <button onclick="sendReact('crown')" style="background:#ca8a04">👑</button>
  </div>
  <form onsubmit="sendMsg(event)" style="margin-top:10px">
    <input type="text" name="sender" placeholder="Ім'я" value="Yaroslav" maxlength="25" required>
    <input type="text" id="msg" name="msg" placeholder="Текст повідомлення на екран..." maxlength="55" required>
    <button type="submit">💬 Вивести текст на дисплей</button>
  </form>
</div>

<div class="card">
  <h2>☁️ Сервер Oracle Cloud & Telegram</h2>
  <form onsubmit="saveCloud(event)" style="margin-bottom:10px">
    <input type="text" name="oracle_url" placeholder="Oracle URL (напр. http://140.238.XX.XX:8000)">
    <input type="password" name="oracle_key" placeholder="Oracle API Key (X-Device-Key)">
    <button type="submit">☁️ Зберегти сервер Oracle</button>
  </form>
  <form onsubmit="saveTg(event)">
    <input type="text" name="token" placeholder="Bot Token (@BotFather)">
    <div class="stat-grid">
      <input type="text" name="admin_id" placeholder="Твій ID (Ярослав)">
      <input type="text" name="girl_id" placeholder="Її ID (Настя)">
    </div>
    <button type="submit" class="btn-sec">🤖 Зберегти конфіг TG</button>
  </form>
</div>

<div class="card">
  <h2>🚀 Оновлення прошивки (OTA)</h2>
  <form onsubmit="uploadOta(event)">
    <input type="file" id="otaFile" accept=".bin" required>
    <button type="submit" class="btn-ota">Оновити firmware.bin</button>
  </form>
  <div class="prog-wrap" id="progWrap"><div class="prog-bar" id="progBar">0%</div></div>
</div>

</div>
<div class="toast" id="toast">Готово!</div>
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
    document.getElementById('b_stat').innerText=d.b_pct+' %'+(d.sleep_m?' (🌙'+d.sleep_m+'m)':' (Кімн:'+d.l+'%)');
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
  fetch('/api/tg',{method:'POST',body:new FormData(e.target)}).then(()=>showToast('🤖 Конфіг TG збережено!'));
}
function saveCloud(e){
  e.preventDefault();
  fetch('/api/cloud',{method:'POST',body:new FormData(e.target)}).then(()=>showToast('☁️ Сервер Oracle збережено!'));
}
function renderNets(nets){
  const list=document.getElementById('netList');
  if(!nets||nets.length===0){list.innerHTML='<div style="padding:10px;text-align:center;color:#94a3b8">Натисніть «Оновити список»</div>';return;}
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
  if(force) list.innerHTML='<div style="padding:12px;text-align:center;color:#38bdf8">⚡ Сканування...</div>';
  fetch('/api/scan'+(force?'?refresh=1':'')).then(r=>r.json()).then(d=>{
    if(d.scanning){setTimeout(()=>scanWifi(false),350);return;}
    renderNets(d.networks);
  }).catch(()=>{});
}
function saveWifi(e){
  e.preventDefault();
  document.getElementById('saveBtn').innerText='⚡ Підключення...';
  fetch('/save',{method:'POST',body:new FormData(e.target)}).then(r=>r.text()).then(msg=>{
    document.body.innerHTML=`<div class="wrap"><div class="card"><h2>✅ ${msg}</h2><p>Підключення до роутера...</p></div></div>`;
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
  xhr.onload=()=>{document.body.innerHTML='<div class="wrap"><div class="card"><h2>🎉 Прошивку оновлено!</h2></div></div>';};
  xhr.open('POST','/update',true);xhr.send(fd);
}
scanWifi(false);updateStats();setInterval(updateStats,2500);
</script></body></html>
)rawliteral";