# Файл: oracle_server/app/web/portal_html.py
PORTAL_HTML: str = r"""<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MagicBox v5.0 Sovereign Cloud Studio</title>
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600;700&family=Plus+Jakarta+Sans:wght@400;600;700;800&display=swap" rel="stylesheet">
<script src="https://cdn.plot.ly/plotly-2.35.2.min.js"></script>
<style>
:root{
  --bg:#050811;--card:rgba(15,23,42,0.86);--border:rgba(56,189,248,0.2);
  --text:#f8fafc;--muted:#94a3b8;--sky:#38bdf8;--emerald:#10b981;--amber:#f59e0b;--rose:#ef4444;--violet:#8b5cf6;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:radial-gradient(circle at 15% 10%,#0f172a 0%,#050811 100%);color:var(--text);font-family:'Plus Jakarta Sans',sans-serif;min-height:100vh;padding:16px}
.mono{font-family:'JetBrains Mono',monospace}
.wrap{max-width:1440px;margin:0 auto}
.top-bar{display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px;background:var(--card);backdrop-filter:blur(16px);border:1px solid var(--border);border-radius:16px;padding:14px 20px;margin-bottom:16px}
.brand{display:flex;align-items:center;gap:12px}
.logo{width:40px;height:40px;border-radius:12px;background:linear-gradient(135deg,#38bdf8,#8b5cf6);display:flex;align-items:center;justify-content:center;font-size:20px;box-shadow:0 0 20px rgba(56,189,248,0.4)}
.pulse{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:6px}
.pulse.on{background:var(--emerald);box-shadow:0 0 10px var(--emerald)}
.pulse.off{background:var(--rose);box-shadow:0 0 10px var(--rose)}
.grid-4{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:16px}
@media(max-width:960px){.grid-4{grid-template-columns:repeat(2,1fr)}}
.kpi{background:var(--card);border:1px solid rgba(255,255,255,0.08);border-radius:14px;padding:14px 18px}
.kpi span{font-size:11.5px;color:var(--muted);text-transform:uppercase;letter-spacing:0.5px}
.kpi b{display:block;font-size:24px;font-family:'JetBrains Mono',monospace;margin-top:4px}
.split-2{display:grid;grid-template-columns:1.25fr 1fr;gap:16px;margin-bottom:16px}
@media(max-width:1080px){.split-2{grid-template-columns:1fr}}
.card{background:var(--card);border:1px solid var(--border);border-radius:16px;padding:18px}
.card h3{font-size:14px;text-transform:uppercase;letter-spacing:0.6px;color:var(--sky);margin-bottom:12px;display:flex;justify-content:space-between;align-items:center}
.mode-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:12px}
@media(max-width:600px){.mode-grid{grid-template-columns:repeat(2,1fr)}}
.btn{background:#1e293b;color:#f8fafc;border:1px solid rgba(255,255,255,0.1);padding:9px 12px;border-radius:10px;font-size:12.5px;font-weight:600;cursor:pointer;transition:0.15s}
.btn:hover{border-color:var(--sky);background:#26354d}
.btn.active{background:#0284c7;border-color:#7dd3fc;color:#fff;font-weight:700}
.btn-emerald{background:rgba(16,185,129,0.2);border-color:var(--emerald);color:#34d399}
input,select,textarea{width:100%;background:#090d16;color:#fff;border:1px solid #334155;padding:9px 12px;border-radius:10px;font-size:13px;outline:none;margin:4px 0}
input:focus,textarea:focus{border-color:var(--sky)}
.media-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(170px,1fr));gap:10px;max-height:320px;overflow-y:auto;padding-right:4px}
.media-item{background:#090d16;border:1px solid #1e293b;border-radius:12px;padding:10px;display:flex;flex-direction:column;justify-content:space-between;gap:8px}
.ambi-dot{width:14px;height:14px;border-radius:50%;display:inline-block;vertical-align:middle;border:1px solid rgba(255,255,255,0.4)}
#loginModal{position:fixed;inset:0;background:rgba(5,8,17,0.92);backdrop-filter:blur(12px);display:none;align-items:center;justify-content:center;z-index:999}
.login-box{background:#0f172a;border:1px solid var(--sky);border-radius:18px;padding:28px;width:350px;text-align:center}
.toast{position:fixed;bottom:20px;right:20px;background:var(--emerald);color:#04130d;padding:12px 20px;border-radius:12px;font-weight:800;display:none;z-index:1000}
</style>
</head>
<body>

<div id="loginModal">
  <div class="login-box">
    <div style="font-size:38px;margin-bottom:8px">🪄</div>
    <h2>MagicBox Studio v5.0</h2>
    <p style="font-size:12px;color:#94a3b8;margin:6px 0 16px">Введіть пароль або X-Device-Key</p>
    <input type="password" id="pwdInput" placeholder="Пароль...">
    <button class="btn active" style="width:100%;margin-top:10px" onclick="doLogin()">🔓 Увійти</button>
  </div>
</div>

<div class="wrap">
  <div class="top-bar">
    <div class="brand">
      <div class="logo">🪄</div>
      <div>
        <div style="display:flex;align-items:center;gap:8px">
          <span id="onlineDot" class="pulse on"></span>
          <b id="onlineTxt" style="font-size:15px">MAGICBOX v5.0 HYPERLIGHT</b>
          <span class="mono" style="font-size:12px;color:var(--sky)" id="fwBadge">FW: --</span>
        </div>
        <div class="mono" style="font-size:11.5px;color:var(--muted);margin-top:3px">
          Server RAM: <b id="ramTxt" style="color:var(--emerald)">-- MB</b> |
          Сховище: <b id="diskTxt">-- MB</b> |
          Черга: <b id="queueTxt">0</b>
        </div>
      </div>
    </div>
    <div id="quantBadge" class="mono" style="font-size:12px;background:rgba(16,185,129,0.12);border:1px solid rgba(16,185,129,0.35);padding:8px 14px;border-radius:10px;display:none"></div>
  </div>

  <div class="grid-4">
    <div class="kpi"><span>Температура в кімнаті</span><b id="kTemp" style="color:var(--sky)">-- °C</b></div>
    <div class="kpi"><span>Вологість повітря</span><b id="kHum" style="color:var(--emerald)">-- %</b></div>
    <div class="kpi"><span>Барометр (Тренд)</span><b id="kPres" style="color:var(--amber)">-- мм</b></div>
    <div class="kpi"><span>Режим та Яскравість</span><b id="kMode" style="font-size:17px">--</b></div>
  </div>

  <div class="split-2">
    <div class="card">
      <h3><span>🎨 Режими Світла, Яскравість та Реакції</span></h3>
      <div class="mode-grid" id="modeGrid"></div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:10px">
        <button class="btn" onclick="sendCmd({set_brt:'auto'})">🤖 Авто</button>
        <button class="btn" onclick="sendCmd({set_brt:'0'})">🌙 15%</button>
        <button class="btn" onclick="sendCmd({set_brt:'1'})">☕ 35%</button>
        <button class="btn" onclick="sendCmd({set_brt:'2'})">✨ 65%</button>
        <button class="btn" onclick="sendCmd({set_brt:'3'})">☀️ 100%</button>
      </div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-bottom:12px">
        <button class="btn" style="background:#ea580c" onclick="sendCmd({react:2,sender:'Studio',msg:'FIRE VIBE!'})">🔥 Вогник</button>
        <button class="btn" style="background:#0284c7" onclick="sendCmd({react:3,sender:'Studio',msg:'KUS-ZAP!'})">⚡ Розряд</button>
        <button class="btn" style="background:#059669" onclick="sendCmd({react:4,sender:'Studio',msg:'TYTS-KUS!'})">🐱 Котик</button>
        <button class="btn" style="background:#7c3aed" onclick="sendCmd({react:5,sender:'Studio',msg:'UFO BEAM!'})">👽 НЛО</button>
        <button class="btn" style="background:#ca8a04" onclick="sendCmd({react:6,sender:'Studio',msg:'QUEEN!'})">👑 Корона</button>
      </div>
      <div style="display:flex;gap:8px">
        <input type="text" id="msgTxt" placeholder="Повідомлення рідною кирилицею на екран або для озвучки...">
        <button class="btn active" style="white-space:nowrap" onclick="pushScreenMsg()">💬 На екран</button>
        <button class="btn btn-emerald" style="white-space:nowrap" onclick="speakTts()">🔊 Озвучити</button>
      </div>
    </div>

    <div class="card">
      <h3><span>🎞 Студія Кастомних Гіфок та Наліпок (.mbx AmbiLight)</span></h3>
      <form onsubmit="uploadMedia(event)" style="display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:center;margin-bottom:12px">
        <input type="file" id="mediaFile" accept=".gif,.webp,.png,.jpg,.jpeg" required>
        <div style="display:flex;gap:6px">
          <input type="text" id="mediaName" placeholder="Назва (напр. cat_vibe)">
          <select id="mediaSlot" style="width:130px">
            <option value="0">У галерею</option>
            <option value="2">На 🔥 Вогник</option>
            <option value="3">На ⚡ Розряд</option>
            <option value="4">На 🐱 Котика</option>
            <option value="5">На 👽 НЛО</option>
            <option value="6">На 👑 Корону</option>
          </select>
        </div>
        <button type="submit" class="btn btn-emerald">🚀 Залити</button>
      </form>
      <div class="media-grid" id="mediaGallery"></div>
    </div>
  </div>

  <div class="split-2">
    <div class="card">
      <h3><span>📈 Історія Клімату та Барометра (SQLite WAL)</span></h3>
      <div id="climateChart" style="width:100%;height:270px"></div>
    </div>

    <div class="card">
      <h3><span>🧠 Кастомна Персона ШІ-Духа Шкатулки</span></h3>
      <input type="text" id="aiName" placeholder="Ім'я Персони (напр. Кібер-Котик Мурчик)">
      <textarea id="aiPrompt" rows="4" placeholder="Системний промпт (характер, стиль звертання, гумор)..."></textarea>
      <div style="display:flex;gap:8px;margin-top:6px">
        <button class="btn active" style="flex:1" onclick="savePersona()">💾 Зберегти Характер</button>
        <button class="btn btn-emerald" style="flex:1" onclick="triggerAiBriefing(true)">🔊 Згенерувати та Озвучити Брифінг</button>
      </div>
    </div>
  </div>
</div>

<div class="toast" id="toast">Готово!</div>

<script>
const MODES=[
  "☀️ Теплий затишок","🕯 Жива свічка","🍯 Бурштинова","🔥 Камін",
  "🌅 Захід сонця","🌸 Сакура Неон","🌌 Аврора","🌊 Океан",
  "⚡ Кібер-комета","🌈 Веселка","🌑 Вимкнено"
];
const SLOT_LABELS={0:"Галерея",2:"🔥 Вогник",3:"⚡ Розряд",4:"🐱 Котик",5:"👽 НЛО",6:"👑 Корона"};

const mg=document.getElementById('modeGrid');
MODES.forEach((n,i)=>{
  const b=document.createElement('button');
  b.className='btn';b.id='mBtn'+i;b.textContent=n;
  b.onclick=()=>sendCmd({set_mode:i});
  mg.appendChild(b);
});

function toast(msg){
  const t=document.getElementById('toast');
  t.innerText=msg;t.style.display='block';
  setTimeout(()=>t.style.display='none',2400);
}

function getHeaders(){
  return {'X-Device-Key': localStorage.getItem('mb_key') || ''};
}

async function api(url, opts={}){
  opts.headers = Object.assign(getHeaders(), opts.headers || {});
  const r = await fetch(url, opts);
  if(r.status === 401){
    document.getElementById('loginModal').style.display='flex';
    throw new Error('Unauthorized');
  }
  return r.json();
}

async function doLogin(){
  const pwd = document.getElementById('pwdInput').value;
  const r = await fetch('/api/v5/auth/login',{
    method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({password:pwd})
  });
  if(r.ok){
    const d = await r.json();
    localStorage.setItem('mb_key', d.token);
    document.getElementById('loginModal').style.display='none';
    loadState();
  }else{
    toast('❌ Невірний пароль');
  }
}

async function sendCmd(payload){
  await api('/api/v5/command',{
    method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify(payload)
  });
  toast('⚡ Команду миттєво відправлено!');
  setTimeout(loadState, 300);
}

async function pushScreenMsg(){
  const m = document.getElementById('msgTxt').value.trim();
  if(!m) return;
  await sendCmd({sender:'Web Studio', msg:m});
  document.getElementById('msgTxt').value='';
}

async function speakTts(){
  const m = document.getElementById('msgTxt').value.trim();
  if(!m) return;
  await api('/api/v1/audio/tts?text='+encodeURIComponent(m),{method:'POST'});
  document.getElementById('msgTxt').value='';
  toast('🔊 Озвучення запущено!');
}

async function uploadMedia(e){
  e.preventDefault();
  const f = document.getElementById('mediaFile').files[0];
  if(!f) return;
  const name = document.getElementById('mediaName').value.trim();
  const slot = document.getElementById('mediaSlot').value;
  const fd = new FormData();
  fd.append('file', f);
  toast('⏳ Векторизована конвертація у .mbx...');
  const r = await fetch(`/api/v5/media/upload?save_name=${encodeURIComponent(name)}&reaction_slot=${slot}`,{
    method:'POST',headers:getHeaders(),body:fd
  });
  if(r.ok){
    const d = await r.json();
    toast(`✨ Готово: ${d.name} (${d.frames} кадрів @ ${d.fps} FPS)!`);
    document.getElementById('mediaFile').value='';
    loadState();
  }
}

async function triggerMedia(name){
  await api('/api/v5/media/trigger/'+encodeURIComponent(name),{method:'POST'});
  toast('▶️ Анімацію виведено на круглий екран!');
}

async function bindSlot(name, slot){
  await api(`/api/v5/media/bind?name=${encodeURIComponent(name)}&slot=${slot}`,{method:'POST'});
  toast('📌 Прив’язку кнопки оновлено!');
  loadState();
}

async function savePersona(){
  const name = document.getElementById('aiName').value;
  const prompt = document.getElementById('aiPrompt').value;
  await api('/api/v5/ai/persona',{
    method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({persona_name:name, system_prompt:prompt, temperature:0.75})
  });
  toast('🧠 Персону ШІ збережено!');
}

async function triggerAiBriefing(speak){
  toast('🌤 Генерація розумного брифінгу...');
  const d = await api('/api/v1/ai/briefing',{
    method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({custom_note:'', speak:speak})
  });
  if(d.ok) toast('✨ ' + d.ai_forecast.slice(0, 65) + '...');
}

function renderGallery(items){
  const g = document.getElementById('mediaGallery');
  g.innerHTML = '';
  items.filter(x=>x.media_type!=='sound').forEach(m=>{
    const rgb = `rgb(${m.ambi_r},${m.ambi_g},${m.ambi_b})`;
    const div = document.createElement('div');
    div.className = 'media-item';
    div.innerHTML = `
      <div>
        <div style="display:flex;justify-content:space-between;align-items:center">
          <b class="mono" style="font-size:12.5px">${m.name}</b>
          <span class="ambi-dot" style="background:${rgb}" title="AmbiLight: ${rgb}"></span>
        </div>
        <div class="mono" style="font-size:11px;color:#94a3b8;margin-top:3px">
          ${m.frames > 1 ? '🎞 GIF '+m.frames+'к @ '+m.fps+'fps' : '🖼 Стікер'} | ${(m.byte_size/1024).toFixed(0)} KB
        </div>
      </div>
      <div style="display:flex;gap:4px">
        <button class="btn active" style="flex:1;padding:5px" onclick="triggerMedia('${m.name}')">▶️ Пуск</button>
        <select onchange="bindSlot('${m.name}', this.value)" style="width:92px;padding:4px;margin:0;font-size:11px">
          ${Object.entries(SLOT_LABELS).map(([k,v])=>`<option value="${k}" ${m.reaction_slot==k?'selected':''}>${v}</option>`).join('')}
        </select>
      </div>
    `;
    g.appendChild(div);
  });
}

let personaLoaded = false;
async function loadState(){
  try{
    const d = await api('/api/v5/state?hours=24');
    if(!d.ok) return;
    document.getElementById('onlineDot').className = 'pulse ' + (d.is_online ? 'on' : 'off');
    document.getElementById('onlineTxt').innerText = d.is_online ? 'MAGICBOX ОНЛАЙН' : 'ОФЛАЙН (ЧЕРГА АКТИВНА)';
    document.getElementById('ramTxt').innerText = d.resources.ram_rss_mb + ' MB';
    document.getElementById('diskTxt').innerText = d.resources.storage_used_mb + ' / ' + d.resources.storage_limit_mb + ' MB';
    document.getElementById('queueTxt').innerText = d.pending_commands;

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
    }

    if(d.quantcore && d.quantcore.online){
      const qb = document.getElementById('quantBadge');
      qb.style.display = 'block';
      const pnl = d.quantcore.unrealized_pnl || 0;
      qb.innerHTML = `⚡ <b>QuantCore V23:</b> $${(d.quantcore.total_equity||0).toFixed(2)} (<span style="color:${pnl>=0?'#10b981':'#ef4444'}">${pnl>=0?'+':''}${pnl.toFixed(2)}$</span>) | Лідер: ${d.quantcore.top_leader}`;
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
        {x:times, y:ch.map(x=>x.t), name:'Темп (°C)', type:'scatter', mode:'lines', line:{color:'#38bdf8',width:2.5}},
        {x:times, y:ch.map(x=>x.p), name:'Барометр (мм)', type:'scatter', mode:'lines', line:{color:'#f59e0b',width:2,dash:'dot'}, yaxis:'y2'}
      ],{
        paper_bgcolor:'rgba(0,0,0,0)', plot_bgcolor:'rgba(9,13,22,0.5)',
        font:{color:'#cbd5e1', family:'JetBrains Mono', size:11},
        margin:{l:40,r:40,t:10,b:30}, legend:{orientation:'h',y:1.14},
        xaxis:{gridcolor:'#1e293b'},
        yaxis:{gridcolor:'#1e293b'},
        yaxis2:{overlaying:'y', side:'right', gridcolor:'rgba(0,0,0,0)'}
      },{displayModeBar:false, responsive:true});
    }
  }catch(e){}
}

loadState();
setInterval(loadState, 5000);
</script>
</body>
</html>
"""