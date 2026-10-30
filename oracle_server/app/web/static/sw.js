// SmartLamp Sovereign Studio - Progressive Web App Service Worker
const CACHE_NAME = 'smartlamp-v5.6.0-shell';
const ASSETS_TO_CACHE = [
  '/',
  '/static/portal.css?v=5.6.0',
  '/static/portal.js?v=5.6.0',
  '/manifest.json',
  '/static/icon-192.svg',
  '/static/icon-512.svg'
];

self.addEventListener('install', (evt) => {
  evt.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      return cache.addAll(ASSETS_TO_CACHE);
    }).then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', (evt) => {
  evt.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) {
            return caches.delete(key);
          }
        })
      );
    }).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (evt) => {
  const url = new URL(evt.request.url);

  // 1. Пропускаємо API та WebSocket напряму через мережу без кешування
  if (url.pathname.startsWith('/api/') || url.pathname.startsWith('/ws/')) {
    return;
  }

  // 2. Для статичних файлів — Stale-While-Revalidate (швидкий запуск + фонове оновлення)
  if (url.pathname.startsWith('/static/') || url.pathname === '/manifest.json') {
    evt.respondWith(
      caches.match(evt.request).then((cachedResp) => {
        const fetchPromise = fetch(evt.request).then((networkResp) => {
          if (networkResp && networkResp.status === 200) {
            const respClone = networkResp.clone();
            caches.open(CACHE_NAME).then((cache) => cache.put(evt.request, respClone));
          }
          return networkResp;
        }).catch(() => cachedResp);
        return cachedResp || fetchPromise;
      })
    );
    return;
  }

  // 3. Для головної сторінки — Network-First з fallback на кеш для офлайну
  evt.respondWith(
    fetch(evt.request).then((networkResp) => {
      if (networkResp && networkResp.status === 200) {
        const respClone = networkResp.clone();
        caches.open(CACHE_NAME).then((cache) => cache.put(evt.request, respClone));
      }
      return networkResp;
    }).catch(() => caches.match(evt.request))
  );
});

// --- РІВЕНЬ 3: WEB PUSH NOTIFICATIONS ---
self.addEventListener('push', (evt) => {
  let data = { title: '✨ SmartLamp Sovereign', body: 'Нове сповіщення' };
  if (evt.data) {
    try {
      data = evt.data.json();
    } catch (_) {
      data.body = evt.data.text();
    }
  }
  const title = data.title || '✨ SmartLamp Sovereign';
  const options = {
    body: data.body || '',
    icon: '/static/icon-192.svg',
    badge: '/static/icon-192.svg',
    data: data.url || '/',
    tag: data.tag || 'smartlamp-alert',
    renotify: true,
  };
  evt.waitUntil(self.registration.showNotification(title, options));
});

self.addEventListener('notificationclick', (evt) => {
  evt.notification.close();
  const targetUrl = evt.notification.data || '/';
  evt.waitUntil(
    clients.matchAll({ type: 'window', includeUncontrolled: true }).then((winList) => {
      for (const client of winList) {
        if (client.url && 'focus' in client) return client.focus();
      }
      if (clients.openWindow) return clients.openWindow(targetUrl);
    })
  );
});
