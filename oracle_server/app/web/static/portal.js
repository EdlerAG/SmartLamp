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
    b.onclick = () => {
      currentLightMode = idx;
      renderLedRing(currentLightMode, currentBrtPct);
      sendCmd({set_mode: idx});
    };
    mg.appendChild(b);
  });
}

// ============================================================================
// 60-LED UNDERGLOW HALO & STUDIO LIGHT CONTROLS
// ============================================================================
let currentLightMode = 0;
let currentBrtPct = 65;
let currentAnimSpeed = 50;
let customR = 255, customG = 180, customB = 60, customW = 0;
let brtDebounceTimer = null;
let speedDebounceTimer = null;
let rgbwDebounceTimer = null;
// 🔒 Slider Input Lock-Out Guard (Zero Slider Jump / Telemetry Conflict Protection)
const _sliderLockUntil = {};

function lockSlider(id, ms = 1000) {
  _sliderLockUntil[id] = Date.now() + ms;
}

function isSliderLocked(id) {
  return Boolean(_sliderLockUntil[id] && Date.now() < _sliderLockUntil[id]);
}

function bindSliderLockGuards() {
  const ids = ['brtSlider', 'speedSlider', 'sliderR', 'sliderG', 'sliderB', 'sliderW', 'volSlider', 'scrAngleSlider', 'cfgMaxMa'];
  ids.forEach(id => {
    const el = document.getElementById(id);
    if (!el) return;
    const lock = () => lockSlider(id, 1200);
    el.addEventListener('input', lock);
    el.addEventListener('change', lock);
    el.addEventListener('pointerdown', () => lockSlider(id, 3000));
    el.addEventListener('pointerup', () => lockSlider(id, 1200));
    el.addEventListener('touchstart', () => lockSlider(id, 3000), { passive: true });
    el.addEventListener('touchend', () => lockSlider(id, 1200), { passive: true });
  });
}

// 🔧 LED Calibrator & Interactive Topology Controller
let ledRingNodes = [];
let calSelectedPixel = -1;
let calUntilTime = 0;
let currentLedReverse = false;

function onLedNodeClick(idx, evt) {
  if (evt) {
    evt.preventDefault();
    evt.stopPropagation();
  }
  if (window.Telegram?.WebApp?.HapticFeedback) {
    try { window.Telegram.WebApp.HapticFeedback.impactOccurred('medium'); } catch(_) {}
  }
  calSelectedPixel = idx;
  calUntilTime = Date.now() + 4200;

  const brtLabel = document.getElementById('ledRingBrt');
  if (brtLabel) brtLabel.innerText = `#${idx}`;
  const modeLabel = document.getElementById('ledRingMode');
  if (modeLabel) modeLabel.innerText = 'Калібратор';
  const badge = document.getElementById('calActiveBadge');
  if (badge) badge.innerText = `Обрано LED #${idx}`;

  sendCmd({ led_cal_px: idx });
  toast(`🔧 LED #${idx} світить бірюзовим маяком на шкатулці`);
  renderLedRing();
}

function toggleLedReverse() {
  currentLedReverse = !currentLedReverse;
  updateReverseBtnText();
  applyTopologyFromUi();
}

function updateReverseBtnText() {
  const btn = document.getElementById('btnLedReverse');
  if (btn) {
    btn.innerText = currentLedReverse ? '🔄 Проти стрілки (CCW)' : '🔄 За годинниковою (CW)';
    btn.style.color = currentLedReverse ? '#f59e0b' : '#38bdf8';
  }
}

function adjustOffset(delta) {
  const inp = document.getElementById('cfgFrontOffset') || document.getElementById('cfgLedOffset');
  if (!inp) return;
  let val = (parseInt(inp.value, 10) || 0) + delta;
  val = ((val % 60) + 60) % 60;
  inp.value = val;
  applyTopologyFromUi();
}

function onMaxMaInput(val) {
  lockSlider('cfgMaxMa', 1000);
  const lbl = document.getElementById('cfgMaxMaVal');
  if (lbl) lbl.innerText = `${val} мА`;
}

function applyTopologyFromUi() {
  const bot = parseInt(document.getElementById('cfgBottomCount')?.value || 36, 10);
  const bak = parseInt(document.getElementById('cfgBackCount')?.value || 24, 10);
  const off = parseInt(document.getElementById('cfgFrontOffset')?.value || 0, 10);
  const ma  = parseInt(document.getElementById('cfgMaxMa')?.value || 2000, 10);

  const payload = {
    set_led_topology: true,
    bottom_count: Math.min(60, Math.max(1, bot)),
    back_count: Math.min(60, Math.max(0, bak)),
    front_offset: ((off % 60) + 60) % 60,
    reversed: currentLedReverse,
    max_ma: Math.min(2500, Math.max(500, ma)),
    // Застарілі поля для сумісності
    led_count: bot + bak,
    led_offset: off,
    led_reverse: currentLedReverse,
    led_max_ma: ma
  };

  sendCmd(payload);
  buildLedRing();
  toast(`Топологія: Дно ${payload.bottom_count} + Стіна ${payload.back_count} LED, зсув ${payload.front_offset}`);
}

function buildLedRing() {
  const svg = document.getElementById('ledRingSvg');
  if (!svg) return;
  svg.innerHTML = '';
  ledRingNodes = [];

  const cx = 110, cy = 110;
  const botCnt = parseInt(document.getElementById('cfgBottomCount')?.value || 36, 10);
  const bakCnt = parseInt(document.getElementById('cfgBackCount')?.value || 24, 10);

  // 1. Зона 1: Внутрішнє коло (Дно / Underglow на стіл)
  const rBot = 65;
  for (let i = 0; i < botCnt; i++) {
    // 0 index знаходиться під екраном (внизу: angle = PI/2)
    const angle = (i / botCnt) * 2 * Math.PI + (Math.PI / 2);
    const x = cx + rBot * Math.cos(angle);
    const y = cy + rBot * Math.sin(angle);

    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('cx', x.toFixed(2));
    circle.setAttribute('cy', y.toFixed(2));
    circle.setAttribute('r', '3.8');
    circle.setAttribute('class', 'led-ring-node led-zone-bot');
    circle.setAttribute('fill', '#38bdf8');
    circle.setAttribute('opacity', '0.7');
    circle.setAttribute('data-idx', String(i));
    circle.setAttribute('title', `Дно #${i}`);

    const hitCircle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    hitCircle.setAttribute('cx', x.toFixed(2));
    hitCircle.setAttribute('cy', y.toFixed(2));
    hitCircle.setAttribute('r', '9');
    hitCircle.setAttribute('class', 'led-ring-hit');
    hitCircle.addEventListener('click', (e) => onLedNodeClick(i, e));
    hitCircle.addEventListener('touchstart', (e) => onLedNodeClick(i, e), { passive: false });

    svg.appendChild(circle);
    svg.appendChild(hitCircle);
    ledRingNodes.push(circle);
  }

  // 2. Зона 2: Зовнішня дуга (Задня стінка / Wall-Wash на стіну)
  const rBak = 94;
  for (let k = 0; k < bakCnt; k++) {
    const globalIdx = botCnt + k;
    // Дуга зверху (від 135° до 45° через верх, тобто від -3*PI/4 до -PI/4 або розмах 180°)
    const spanFrac = bakCnt > 1 ? (k / (bakCnt - 1)) : 0.5;
    const arcAngle = (Math.PI * 0.85) - spanFrac * (Math.PI * 1.7); // верхня дуга
    const x = cx + rBak * Math.cos(arcAngle);
    const y = cy - rBak * Math.sin(arcAngle);

    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('cx', x.toFixed(2));
    circle.setAttribute('cy', y.toFixed(2));
    circle.setAttribute('r', '4.4');
    circle.setAttribute('class', 'led-ring-node led-zone-bak');
    circle.setAttribute('fill', '#f59e0b');
    circle.setAttribute('opacity', '0.7');
    circle.setAttribute('data-idx', String(globalIdx));
    circle.setAttribute('title', `Стіна #${globalIdx}`);

    const hitCircle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    hitCircle.setAttribute('cx', x.toFixed(2));
    hitCircle.setAttribute('cy', y.toFixed(2));
    hitCircle.setAttribute('r', '10');
    hitCircle.setAttribute('class', 'led-ring-hit');
    hitCircle.addEventListener('click', (e) => onLedNodeClick(globalIdx, e));
    hitCircle.addEventListener('touchstart', (e) => onLedNodeClick(globalIdx, e), { passive: false });

    svg.appendChild(circle);
    svg.appendChild(hitCircle);
    ledRingNodes.push(circle);
  }
}

// 🌈 Live 20 FPS Client-Side Animation Loop
let ringAnimStep = 0;
function animateLedRingFrame() {
  if (!ledRingNodes.length) return;
  const now = Date.now();
  const isCalibrating = (now < calUntilTime && calSelectedPixel >= 0);

  if (isCalibrating) {
    const pulse = (Math.sin(ringAnimStep * 0.25) + 1.0) * 0.5;
    const beaconColor = `hsl(175, 100%, ${45 + Math.round(pulse * 30)}%)`;

    ledRingNodes.forEach((node, i) => {
      if (i === calSelectedPixel) {
        node.setAttribute('fill', beaconColor);
        node.setAttribute('opacity', '1.0');
        node.setAttribute('r', String(5 + pulse * 2.5));
      } else if (i % 10 === 0) {
        node.setAttribute('fill', '#38bdf8');
        node.setAttribute('opacity', '0.6');
        node.setAttribute('r', '4');
      } else {
        node.setAttribute('fill', '#1e2536');
        node.setAttribute('opacity', '0.2');
        node.setAttribute('r', '3');
      }
    });
    ringAnimStep++;
    return;
  }

  if (currentLightMode === 10 || currentBrtPct === 0) return;

  const baseOpacity = Math.max(0.4, currentBrtPct / 100);
  ringAnimStep++;

  if (currentLightMode === 9) {
    ledRingNodes.forEach((node, i) => {
      const hue = Math.floor(((i + ringAnimStep * 0.5) % 60) * 6);
      node.setAttribute('fill', `hsl(${hue}, 90%, 55%)`);
      node.setAttribute('opacity', String(baseOpacity));
    });
  } else if (currentLightMode === 8) {
    const cometHead = Math.floor((ringAnimStep * 0.6) % 60);
    ledRingNodes.forEach((node, i) => {
      const dist = (i - cometHead + 60) % 60;
      if (dist === 0) {
        node.setAttribute('fill', '#ffffff');
        node.setAttribute('opacity', '1.0');
        node.setAttribute('r', '5');
      } else if (dist < 8) {
        node.setAttribute('fill', '#38bdf8');
        node.setAttribute('opacity', String(Math.max(0.2, (1 - dist / 8) * baseOpacity)));
        node.setAttribute('r', '4');
      } else {
        node.setAttribute('fill', '#0c4a6e');
        node.setAttribute('opacity', '0.3');
        node.setAttribute('r', '3');
      }
    });
  } else if (currentLightMode === 3) {
    ledRingNodes.forEach((node, i) => {
      const flicker = Math.sin(ringAnimStep * 0.3 + i * 0.4) * 0.25;
      const hue = 15 + Math.floor((Math.sin(ringAnimStep * 0.15 + i * 0.8) + 1.0) * 15);
      node.setAttribute('fill', `hsl(${hue}, 100%, ${45 + flicker * 20}%)`);
      node.setAttribute('opacity', String(Math.min(1.0, baseOpacity + flicker)));
    });
  } else if (currentLightMode === 6) {
    ledRingNodes.forEach((node, i) => {
      const w = (Math.sin(ringAnimStep * 0.1 + i * 0.2) + 1.0) * 0.5;
      const hue = Math.floor(150 + w * 40);
      node.setAttribute('fill', `hsl(${hue}, 90%, ${45 + w * 20}%)`);
      node.setAttribute('opacity', String(baseOpacity));
    });
  }
}

function startLedRingLiveAnimation() {
  setInterval(animateLedRingFrame, 50); // 20 FPS
}

function computeRgbwCss(r, g, b, w) {
  const wr = Math.min(255, Math.floor(w * 1.0));
  const wg = Math.min(255, Math.floor(w * 0.84));
  const wb = Math.min(255, Math.floor(w * 0.67));
  const fr = Math.min(255, r + wr);
  const fg = Math.min(255, g + wg);
  const fb = Math.min(255, b + wb);
  return `rgb(${fr}, ${fg}, ${fb})`;
}

function renderLedRing(mode = currentLightMode, brt = currentBrtPct) {
  if (!ledRingNodes.length) buildLedRing();
  const isOff = (mode === 10 || brt === 0);
  const center = document.getElementById('ledRingCenter');
  const icon = document.getElementById('ledPwrIcon');
  const brtLabel = document.getElementById('ledRingBrt');
  const modeLabel = document.getElementById('ledRingMode');
  const haloBadge = document.getElementById('haloModeBadge');

  if (center) {
    if (isOff) center.classList.remove('lamp-on');
    else center.classList.add('lamp-on');
  }
  if (icon) icon.innerText = isOff ? '🌑' : '💡';
  if (brtLabel) brtLabel.innerText = isOff ? '0%' : `${brt}%`;
  if (modeLabel) modeLabel.innerText = MODES[mode] ? MODES[mode].replace(/^[^\w\sа-яіїєА-ЯІЇЄ]+/, '').trim() : 'Лампа';
  if (haloBadge) haloBadge.innerText = isOff ? 'ВИМКНЕНО' : (MODES[mode] ? MODES[mode].split(' ')[1] || 'СВІТЛО' : '60x SK6812');

  const baseOpacity = isOff ? 0.2 : Math.max(0.4, brt / 100);

  ledRingNodes.forEach((node, i) => {
    if (isOff) {
      node.setAttribute('fill', '#1e2536');
      node.setAttribute('opacity', '0.25');
      node.setAttribute('r', '3');
      return;
    }

    let color = '#f59e0b';
    switch (mode) {
      case 0: color = '#ff9820'; break;
      case 1: color = (i % 2 === 0) ? '#ff6600' : '#ff9900'; break;
      case 2: color = '#ffaa11'; break;
      case 3: color = (i % 3 === 0) ? '#ff2200' : ((i % 3 === 1) ? '#ff5500' : '#ff9900'); break;
      case 4: color = (i / 60 < 0.5) ? '#e11d48' : '#f59e0b'; break;
      case 5: color = (i % 2 === 0) ? '#f43f5e' : '#ec4899'; break;
      case 6: color = (i / 60 < 0.5) ? '#10b981' : '#06b6d4'; break;
      case 7: color = (i % 2 === 0) ? '#0284c7' : '#38bdf8'; break;
      case 8: color = (i >= 25 && i <= 35) ? '#e0f2fe' : '#0369a1'; break;
      case 9: color = `hsl(${Math.floor((i / 60) * 360)}, 90%, 55%)`; break;
      case 11:
        if (window._spotifyCol1) {
          color = (i >= 15 && i <= 45) ? window._spotifyCol1 : (window._spotifyCol2 || '#f59e0b');
        } else {
          color = '#1ed760';
        }
        break;
      default:
        color = computeRgbwCss(customR, customG, customB, customW);
        break;
    }
    node.setAttribute('fill', color);
    node.setAttribute('opacity', String(baseOpacity));
    node.setAttribute('r', '4');
  });
}

function onBrtSlider(val) {
  const pct = parseInt(val, 10) || 1;
  currentBrtPct = pct;
  const label = document.getElementById('brtVal');
  if (label) label.innerText = `${pct}%`;
  renderLedRing(currentLightMode, pct);

  clearTimeout(brtDebounceTimer);
  brtDebounceTimer = setTimeout(() => {
    sendCmd({set_brt: String(pct)});
    toast(`Яскравість: ${pct}%`);
  }, 60);
}

function setBrtPreset(val) {
  const slider = document.getElementById('brtSlider');
  if (slider) slider.value = val;
  onBrtSlider(val);
}

function onSpeedSlider(val) {
  const spd = parseInt(val, 10) || 50;
  currentAnimSpeed = spd;
  const label = document.getElementById('speedVal');
  if (label) label.innerText = String(spd);

  clearTimeout(speedDebounceTimer);
  speedDebounceTimer = setTimeout(() => {
    sendCmd({speed: spd});
    toast(`Швидкість: ${spd}`);
  }, 60);
}

function setSpeedPreset(val) {
  const slider = document.getElementById('speedSlider');
  if (slider) slider.value = val;
  onSpeedSlider(val);
}

function onRgbwInput() {
  const r = parseInt(document.getElementById('sliderR')?.value || 0, 10);
  const g = parseInt(document.getElementById('sliderG')?.value || 0, 10);
  const b = parseInt(document.getElementById('sliderB')?.value || 0, 10);
  const w = parseInt(document.getElementById('sliderW')?.value || 0, 10);

  customR = r; customG = g; customB = b; customW = w;
  setCachedText('valR', r);
  setCachedText('valG', g);
  setCachedText('valB', b);
  setCachedText('valW', w);

  const hex = `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`.toUpperCase();
  setCachedText('rgbwHex', `${hex} + W:${w}`);

  const preview = document.getElementById('rgbwPreview');
  if (preview) {
    const cssCol = computeRgbwCss(r, g, b, w);
    preview.style.backgroundColor = cssCol;
    preview.style.boxShadow = `0 0 16px ${cssCol}`;
  }
}

function applyCustomRgbw() {
  sendCmd({
    custom_r: customR,
    custom_g: customG,
    custom_b: customB,
    custom_w: customW
  });
  renderLedRing(currentLightMode, currentBrtPct);
  toast(`Колір SK6812: R${customR} G${customG} B${customB} W${customW}`);
}

function setRgbwPreset(r, g, b, w) {
  customR = r; customG = g; customB = b; customW = w;
  const sR = document.getElementById('sliderR'); if (sR) sR.value = r;
  const sG = document.getElementById('sliderG'); if (sG) sG.value = g;
  const sB = document.getElementById('sliderB'); if (sB) sB.value = b;
  const sW = document.getElementById('sliderW'); if (sW) sW.value = w;
  onRgbwInput();
  applyCustomRgbw();
}

function toggleLampPower() {
  if (currentLightMode === 10) {
    currentLightMode = 0;
    sendCmd({set_mode: 0});
    toast('Підсвітка УВІМКНЕНА ☀️');
  } else {
    currentLightMode = 10;
    sendCmd({set_mode: 10});
    toast('Підсвітка ВИМКНЕНА 🌑');
  }
  renderLedRing(currentLightMode, currentBrtPct);
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
  if (Boolean(window.Telegram?.WebApp?.initData)) return;
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
        const delBtn = window._currentUserRole === 'admin'
          ? ''
          : `<button class="btn btn-danger" style="padding:2px 6px;font-size:10px" onclick="deleteCalendarEvent('${ev.id}')">✕</button>`;
        html += `
          <div style="display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px solid var(--border-hairline);${isPast}">
            <div>
              <b class="mono" style="color:var(--accent-cool);margin-right:6px">${escapeHtml(timeStr)}</b>
              <span>${escapeHtml(ev.summary || 'Без назви')}</span>
            </div>
            ${delBtn}
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

// --- ATMOSPHERIC MOOD THEMES & ONBOARDING SUITE ---
const MOOD_TO_MODE = {
  cozy: 0,       // ☀️ Теплий затишок
  sakura: 5,     // 🌸 Сакура Неон
  ocean: 7,      // 🌊 Океанський бриз
  nordic: 6,     // 🌌 Північне сяйво
  chornobyl: 8,  // ⚡ Кібер-комета (Смарагдове сяйво)
  chernobyl: 8
};

function selectMoodTheme(mood, autoSyncLamp = true) {
  if (mood === 'chernobyl') mood = 'chornobyl';
  if (!MOOD_TO_MODE.hasOwnProperty(mood)) mood = 'cozy';
  const allMoods = ['cozy', 'sakura', 'ocean', 'nordic', 'chornobyl', 'chernobyl'];
  allMoods.forEach(m => {
    document.body.classList.remove('theme-' + m);
    const btn = document.getElementById('moodBtn-' + m);
    if (btn) btn.classList.remove('active');
  });
  document.body.classList.add('theme-' + mood);
  const activeBtn = document.getElementById('moodBtn-' + mood);
  if (activeBtn) activeBtn.classList.add('active');

  try {
    localStorage.setItem('mb_mood', mood);
  } catch (_) {}

  if (autoSyncLamp) {
    const lampMode = MOOD_TO_MODE[mood];
    if (lampMode !== undefined) {
      setMode(lampMode);
    }
    fetch('/api/v5/setup/mood', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ mood: mood })
    }).then(r => r.json()).then(d => {
      if (d && d.ok) {
        toast('Атмосферу змінено');
        loadOnboardingState();
      }
    }).catch(() => {});
  }
}

function toggleOnboardingWidget() {
  const list = document.getElementById('onboardStepsList');
  const btn = document.getElementById('onboardToggleBtn');
  if (!list || !btn) return;
  const isHidden = list.style.display === 'none';
  if (isHidden) {
    list.style.display = 'grid';
    btn.innerText = 'Згорнути ▲';
    try { localStorage.setItem('mb_onboard_open', '1'); } catch (_) {}
  } else {
    list.style.display = 'none';
    btn.innerText = 'Розгорнути ▼';
    try { localStorage.setItem('mb_onboard_open', '0'); } catch (_) {}
  }
}

function loadOnboardingState() {
  fetch('/api/v5/setup/state')
    .then(r => {
      if (!r.ok) return null;
      return r.json();
    })
    .then(d => {
      if (!d || !d.steps) return;
      const s = d.steps;
      
      const stepKeys = ['wifi', 'pairing', 'mood', 'spotify', 'calendar', 'quiet_hours'];
      let doneCount = 0;
      stepKeys.forEach(k => {
        if (s[k] && s[k].status === 'done') doneCount++;
      });

      const badge = document.getElementById('onboardProgressBadge');
      if (badge) {
        badge.innerText = `${doneCount}/6 Готово`;
        if (doneCount === 6) {
          badge.style.background = 'rgba(16,185,129,0.25)';
          badge.style.color = 'var(--accent-success)';
        }
      }

      function updateStepUI(id, isDone, text) {
        const item = document.getElementById(id);
        const val = document.getElementById(id + '-val');
        if (!item || !val) return;
        val.innerText = text;
        if (isDone) {
          item.classList.add('done');
          item.classList.remove('pending');
          val.style.color = 'var(--accent-success)';
        } else {
          item.classList.remove('done');
          item.classList.add('pending');
          val.style.color = 'var(--text-secondary)';
        }
      }

      updateStepUI('step-wifi', Boolean(s.wifi && s.wifi.status === 'done'), (s.wifi && s.wifi.status === 'done') ? 'Підключено' : 'Очікування...');
      updateStepUI('step-pairing', Boolean(d.claimed || (s.pairing && s.pairing.status === 'done')), (d.claimed || (s.pairing && s.pairing.status === 'done')) ? 'Прив\'язано' : 'Очікування...');

      const moodNames = {
        cozy: '🕯️ Затишок',
        sakura: '🌸 Сакура',
        ocean: '🌊 Океан',
        nordic: '❄️ Північ',
        chornobyl: '☢️ Чорнобиль',
        chernobyl: '☢️ Чорнобиль'
      };
      const curMood = d.theme_mood || 'cozy';
      updateStepUI('step-mood', Boolean(s.mood && s.mood.status === 'done'), moodNames[curMood] || curMood);

      const spDone = Boolean(d.spotify_linked || (s.spotify && s.spotify.status === 'done'));
      const spSkipped = Boolean(s.spotify && s.spotify.status === 'skipped');
      const spText = spDone ? 'Підключено' : (spSkipped ? 'Пропущено' : 'Не підключено');
      updateStepUI('step-spotify', spDone || spSkipped, spText);

      const calDone = Boolean(d.calendar_linked || (s.calendar && s.calendar.status === 'done'));
      const calSkipped = Boolean(s.calendar && s.calendar.status === 'skipped');
      const calText = calDone ? 'Підключено' : (calSkipped ? 'Пропущено' : 'Не підключено');
      updateStepUI('step-calendar', calDone || calSkipped, calText);

      const qhDone = Boolean(s.quiet_hours && s.quiet_hours.status === 'done');
      updateStepUI('step-quiet_hours', qhDone, '22:00 - 08:00');
    })
    .catch(() => {});
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
      pomodoro: d.pomodoro,
      mood: d.mood
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

  window._currentUserRole = d.user_role;
  if (d.user_role === 'admin') {
    const ap = document.getElementById('adminPanel');
    if (ap) ap.style.display = 'block';
    const rb = document.getElementById('roleBadge');
    if (rb) { rb.innerText = '🛡️ VIEWER'; rb.style.color = 'var(--accent-cool)'; }
    const cvb = document.getElementById('calViewerBadge');
    if (cvb) cvb.style.display = 'inline-block';
    const car = document.getElementById('calAddEventRow');
    if (car) car.style.display = 'none';
    const cdb = document.getElementById('calDisconnectBtn');
    if (cdb) cdb.style.display = 'none';
  } else {
    const ap = document.getElementById('adminPanel');
    if (ap) ap.style.display = 'none';
    const rb = document.getElementById('roleBadge');
    if (rb) { rb.innerText = '👑 OWNER'; rb.style.color = 'var(--accent-pink)'; }
    const cvb = document.getElementById('calViewerBadge');
    if (cvb) cvb.style.display = 'none';
    const car = document.getElementById('calAddEventRow');
    if (car) car.style.display = 'grid';
    const cdb = document.getElementById('calDisconnectBtn');
    if (cdb) cdb.style.display = 'inline-block';
  }

  if (d.mood) {
    selectMoodTheme(d.mood, false);
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

// Restore saved mood or default to 'cozy'
try {
  const savedMood = localStorage.getItem('mb_mood') || 'cozy';
  selectMoodTheme(savedMood, false);
} catch (_) {}

// Restore onboarding open/close state
try {
  if (localStorage.getItem('mb_onboard_open') === '0') {
    const list = document.getElementById('onboardStepsList');
    const btn = document.getElementById('onboardToggleBtn');
    if (list) list.style.display = 'none';
    if (btn) btn.innerText = 'Розгорнути ▼';
  }
} catch (_) {}

// Telegram Mini App Auto-Auth (Zero-Click, Single-Roundtrip Auth + State)
const isTelegramApp = Boolean(window.Telegram?.WebApp?.initData);
let isAuthReady = !isTelegramApp;

if (isTelegramApp) {
  try {
    window.Telegram.WebApp.ready();
    window.Telegram.WebApp.expand();
    try {
      if (window.Telegram.WebApp.setHeaderColor) window.Telegram.WebApp.setHeaderColor('#07090e');
      if (window.Telegram.WebApp.setBackgroundColor) window.Telegram.WebApp.setBackgroundColor('#07090e');
    } catch (_) {}
    const qrBtn = document.querySelector('button[title="QR Код"]');
    if (qrBtn) qrBtn.style.display = 'none';
    const onboardList = document.getElementById('onboardStepsList');
    const onboardBtn = document.getElementById('onboardToggleBtn');
    if (onboardList && localStorage.getItem('mb_onboard_open') !== '1') {
      onboardList.style.display = 'none';
      if (onboardBtn) onboardBtn.innerText = 'Розгорнути ▼';
    }
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
        loadOnboardingState();
        initStudioWebSocket();
      }
    })
    .catch(() => {});
  } catch (_) {}
} else {
  // Regular browser emergency access
  pollState();
  loadCalendarEvents();
  loadOnboardingState();
}
setInterval(loadCalendarEvents, 30000);
setInterval(loadOnboardingState, 30000);

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
    currentLightMode = t.m;
    MODES.forEach((_, i) => {
      setCachedClass('mBtn' + i, (t.m === i) ? 'btn active' : 'btn');
    });
    renderLedRing(currentLightMode, currentBrtPct);
  }

  if (t.b_pct !== undefined && !isSliderLocked('brtSlider')) {
    currentBrtPct = t.b_pct;
    const brtSlider = document.getElementById('brtSlider');
    if (brtSlider) {
      brtSlider.value = t.b_pct;
      setCachedText('brtVal', `${t.b_pct}%`);
    }
    renderLedRing(currentLightMode, currentBrtPct);
  }

  if (t.vol !== undefined && !isSliderLocked('volSlider')) {
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
  if (t.scr_ang !== undefined && !isSliderLocked('scrAngleSlider')) {
    currentScreenAngle = t.scr_ang;
    const slider = document.getElementById('scrAngleSlider');
    if (slider) slider.value = t.scr_ang;
    setCachedText('scrAngleVal', `${t.scr_ang}°`);
  }

  // Синхронізація дводіапазонної топології стрічки від плати
  let rebuildRing = false;
  if (t.led_bot !== undefined && !isSliderLocked('cfgBottomCount')) {
    const curVal = document.getElementById('cfgBottomCount')?.value;
    if (curVal != t.led_bot) rebuildRing = true;
    setCachedValue('cfgBottomCount', t.led_bot);
  }
  if (t.led_bak !== undefined && !isSliderLocked('cfgBackCount')) {
    const curVal = document.getElementById('cfgBackCount')?.value;
    if (curVal != t.led_bak) rebuildRing = true;
    setCachedValue('cfgBackCount', t.led_bak);
  }
  if (t.led_off !== undefined && !isSliderLocked('cfgFrontOffset')) {
    setCachedValue('cfgFrontOffset', t.led_off);
  }
  if (t.led_rev !== undefined) {
    currentLedReverse = Boolean(t.led_rev === true || t.led_rev === 'true');
    updateReverseBtnText();
  }
  if (t.led_ma !== undefined && !isSliderLocked('cfgMaxMa')) {
    setCachedValue('cfgMaxMa', t.led_ma);
    setCachedText('cfgMaxMaVal', `${t.led_ma} мА`);
  }
  if (rebuildRing) {
    buildLedRing();
  }
}

let currentScreenAngle = 0;
function onAngleSlider(val) {
  setCachedText('scrAngleVal', `${val}°`);
}
function applyAngleSlider(val) {
  const ang = parseInt(val, 10) || 0;
  currentScreenAngle = ang;
  sendCmd({screen_angle: ang});
  toast(`Кут екрана: ${ang}°`);
}
function setScreenAngle(ang) {
  let a = parseInt(ang, 10) || 0;
  a = ((a % 360) + 360) % 360;
  currentScreenAngle = a;
  const slider = document.getElementById('scrAngleSlider');
  if (slider) slider.value = a;
  setCachedText('scrAngleVal', `${a}°`);
  sendCmd({screen_angle: a});
  toast(`Кут екрана: ${a}°`);
}
function adjustScreenAngle(delta) {
  let a = (currentScreenAngle + delta) % 360;
  if (a < 0) a += 360;
  setScreenAngle(a);
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
        const jitter = Math.floor(Math.random() * 400);
        const nextDelay = Math.min(wsReconnectDelay + jitter, 10000);
        wsReconnectTimer = setTimeout(initStudioWebSocket, nextDelay);
        wsReconnectDelay = Math.min(wsReconnectDelay * 2, 10000);
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

// Startup LED Ring, Slider Guards and Animation Loop
setTimeout(() => {
  buildLedRing();
  onRgbwInput();
  renderLedRing();
  bindSliderLockGuards();
  startLedRingLiveAnimation();
}, 200);
