#pragma once
#include <Arduino.h>

class IService {
protected:
    uint32_t last_run_ms = 0;

public:
    virtual ~IService() = default;

    virtual const char* getName() const = 0;
    virtual void init() = 0;
    virtual void update() = 0;

    // Дозволяє сервісу спати, якщо він не налаштований або не потрібен (Zero-CPU)
    virtual bool isEnabled() const { return true; }

    // Мінімальний інтервал між викликами update() у мс (0 = кожен цикл)
    virtual uint32_t getIntervalMs() const { return 0; }

    void tick(uint32_t now) {
        if (!isEnabled()) return;
        uint32_t interval = getIntervalMs();
        if (interval == 0 || (now - last_run_ms >= interval)) {
            last_run_ms = now;
            update();
        }
    }
};