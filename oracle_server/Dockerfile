FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PYTHONOPTIMIZE=1 \
    MALLOC_TRIM_THRESHOLD_=65536 \
    MALLOC_ARENA_MAX=2

WORKDIR /srv/magicbox

COPY requirements.txt .
RUN pip install --no-cache-dir --compile -r requirements.txt

COPY . .
RUN mkdir -p storage/ota storage/media

EXPOSE 8000

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000", "--loop", "uvloop", "--http", "httptools", "--no-access-log", "--limit-concurrency", "32"]