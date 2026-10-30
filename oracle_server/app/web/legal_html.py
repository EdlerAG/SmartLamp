# Файл: oracle_server/app/web/legal_html.py

_LEGAL_STYLE = """
    :root {
        --bg-grad: radial-gradient(circle at 50% 10%, #1e1b4b 0%, #030712 100%);
        --card-bg: rgba(17, 24, 39, 0.75);
        --card-border: rgba(255, 255, 255, 0.1);
        --text-primary: #f9fafb;
        --text-secondary: #9ca3af;
        --accent: #818cf8;
        --accent-glow: rgba(129, 140, 248, 0.25);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        background: #030712;
        background-image: var(--bg-grad);
        color: var(--text-primary);
        min-height: 100vh;
        padding: 40px 20px;
        line-height: 1.6;
        display: flex;
        justify-content: center;
    }
    .container {
        max-width: 780px;
        width: 100%;
        background: var(--card-bg);
        border: 1px solid var(--card-border);
        border-radius: 24px;
        padding: 36px 32px;
        backdrop-filter: blur(16px);
        -webkit-backdrop-filter: blur(16px);
        box-shadow: 0 20px 50px rgba(0, 0, 0, 0.5), 0 0 40px var(--accent-glow);
    }
    .header {
        display: flex;
        align-items: center;
        gap: 16px;
        margin-bottom: 24px;
        padding-bottom: 20px;
        border-bottom: 1px solid var(--card-border);
    }
    .logo-badge {
        width: 48px;
        height: 48px;
        border-radius: 14px;
        background: linear-gradient(135deg, #6366f1, #a855f7);
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 24px;
        box-shadow: 0 4px 15px rgba(99, 102, 241, 0.4);
    }
    h1 {
        font-size: 24px;
        font-weight: 700;
        letter-spacing: -0.02em;
    }
    .meta {
        font-size: 13px;
        color: var(--text-secondary);
        margin-top: 2px;
    }
    h2 {
        font-size: 18px;
        font-weight: 600;
        margin-top: 28px;
        margin-bottom: 10px;
        color: #c7d2fe;
    }
    p, ul {
        font-size: 15px;
        color: #d1d5db;
        margin-bottom: 14px;
    }
    ul {
        padding-left: 24px;
    }
    li {
        margin-bottom: 6px;
    }
    a {
        color: var(--accent);
        text-decoration: none;
    }
    a:hover {
        text-decoration: underline;
    }
    .back-btn {
        display: inline-flex;
        align-items: center;
        gap: 8px;
        margin-top: 32px;
        padding: 10px 20px;
        border-radius: 12px;
        background: rgba(255, 255, 255, 0.06);
        border: 1px solid var(--card-border);
        color: var(--text-primary);
        font-size: 14px;
        font-weight: 500;
        transition: background 0.2s;
    }
    .back-btn:hover {
        background: rgba(255, 255, 255, 0.12);
        text-decoration: none;
    }
"""

PRIVACY_POLICY_HTML = f"""<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Політика конфіденційності | MagicBox</title>
    <style>{_LEGAL_STYLE}</style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo-badge">✨</div>
            <div>
                <h1>Політика конфіденційності</h1>
                <div class="meta">MagicBox Companion • Останнє оновлення: 31 жовтня 2026 р.</div>
            </div>
        </div>

        <h2>1. Загальні положення</h2>
        <p>Ця Політика конфіденційності регулює порядок збору, використання та захисту інформації додатком <strong>MagicBox Companion</strong> (розумний світильник-компаньйон та персональний хаб). Додаток є суто некомерційним приватним сервісом, створеним для домашнього затишку та персонального використання.</p>

        <h2>2. Які дані обробляються</h2>
        <p>MagicBox обробляє мінімально необхідний обсяг даних виключно для забезпечення роботи своїх функцій:</p>
        <ul>
            <li><strong>Кліматична телеметрія:</strong> температура, вологість та атмосферний тиск із вбудованих датчиків пристрою для відображення на екрані.</li>
            <li><strong>Spotify API:</strong> назва треку, автор, обкладинка альбому та статус відтворення для відображення медіа-статусу на екрані пристрою та керування відтворенням з дозволу користувача.</li>
            <li><strong>Google Calendar API:</strong> назви та час найближчих запланованих подій користувача для виведення нагадувань на екран шкатулки.</li>
            <li><strong>Технічна діагностика:</strong> рівень сигналу Wi-Fi, вільна пам'ять пристрою, статус зв'язку та стабільність електроживлення (фіксація відключень світла).</li>
        </ul>

        <h2>3. Мета використання даних</h2>
        <p>Всі отримані дані використовуються виключно для взаємодії між пристроєм MagicBox, персональним Telegram-ботом та веб-інтерфейсом власника. Ми <strong>ніколи не передаємо</strong>, не продаємо, не орендуємо та не розкриваємо ваші персональні дані третім особам, рекламним мережам чи аналітичним платформам.</p>

        <h2>4. Інтеграції з Google та Spotify</h2>
        <p>Підключення сторонніх сервісів є повністю добровільним і здійснюється через стандартні безпечні протоколи <em>OAuth 2.0</em>. Токени доступу зберігаються в зашифрованому вигляді на приватному сервері. MagicBox запитує доступ тільки до необхідних областей (read-only для календаря та керування відтворенням для Spotify).</p>

        <h2>5. Керування та видалення даних</h2>
        <p>Власник пристрою зберігає повний контроль над своїми даними:</p>
        <ul>
            <li>Ви можете в будь-який момент відключити Spotify або Google Calendar одним натисканням у меню налаштувань.</li>
            <li>Ви можете відкликати доступ безпосередньо в налаштуваннях безпеки Google (<a href="https://myaccount.google.com/permissions" target="_blank">Google Security</a>) та Spotify (<a href="https://www.spotify.com/account/apps/" target="_blank">Spotify Apps</a>).</li>
            <li>Ви можете повністю очистити всі збережені дані та розірвати зв'язок із пристроєм за допомогою команди скидання.</li>
        </ul>

        <h2>6. Безпека даних</h2>
        <p>Для захисту зв'язку використовується сучасне шифрування TLS/HTTPS та WSS, криптографічні хеші SHA-256 для токенів сесій та апаратна автентифікація.</p>

        <a href="/" class="back-btn">← Повернутися до MagicBox</a>
    </div>
</body>
</html>
"""

TERMS_OF_SERVICE_HTML = f"""<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Умови використання | MagicBox</title>
    <style>{_LEGAL_STYLE}</style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo-badge">📜</div>
            <div>
                <h1>Умови використання</h1>
                <div class="meta">MagicBox Companion • Останнє оновлення: 31 жовтня 2026 р.</div>
            </div>
        </div>

        <h2>1. Прийняття умов</h2>
        <p>Використовуючи розумний пристрій <strong>MagicBox</strong>, його веб-панель керування або супутнього Telegram-бота, ви погоджуєтесь із цими Умовами використання.</p>

        <h2>2. Опис сервісу</h2>
        <p>MagicBox — це персональний затишний пристрій-компаньйон, що забезпечує атмосферне підсвічування, відображення корисного віджета на екрані (погода, розклад дня, поточна музика), таймер фокусування та приватне спілкування.</p>

        <h2>3. Права та обов'язки користувача</h2>
        <ul>
            <li>Сервіс надається для некомерційного, персонального використання.</li>
            <li>Користувач має право самостійно обирати, які інтеграції (Spotify, Google Calendar) підключати, змінювати теми оформлення та налаштовувати тихі години.</li>
            <li>Користувач несе відповідальність за збереження свого доступу до Telegram-акаунту, з якого здійснюється керування пристроєм.</li>
        </ul>

        <h2>4. Інтеграції зі сторонніми платформами</h2>
        <p>При підключенні Google Calendar або Spotify ви також приймаєте умови надання послуг відповідних провайдерів. MagicBox використовує офіційні API виключно в межах наданих вами дозволів і не несе відповідальності за можливі збої чи зміни правил на стороні сторонніх сервісів.</p>

        <h2>5. Доступність сервісу</h2>
        <p>Сервіс надається на умовах «як є» (as is). Оскільки пристрій працює з локальними датчиками та мережевими інтерфейсами, періодично можуть проводитися планові оновлення програмного забезпечення для підвищення стабільності та додавання нових можливостей.</p>

        <h2>6. Зміни до умов</h2>
        <p>Ці умови можуть час від часу оновлюватися. Актуальна редакція завжди доступна на цій сторінці.</p>

        <a href="/" class="back-btn">← Повернутися до MagicBox</a>
    </div>
</body>
</html>
"""
