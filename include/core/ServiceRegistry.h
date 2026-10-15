#pragma once
#include "core/IService.h"

template <size_t MAX_SERVICES = 8>
class ServiceRegistry {
private:
    IService* services[MAX_SERVICES] = {nullptr};
    size_t count = 0;

public:
    bool add(IService* srv) {
        if (!srv || count >= MAX_SERVICES) return false;
        services[count++] = srv;
        return true;
    }

    void initAll() {
        for (size_t i = 0; i < count; i++) {
            if (services[i]) {
                services[i]->init();
            }
        }
    }

    void tickAll() {
        uint32_t now = millis();
        for (size_t i = 0; i < count; i++) {
            if (services[i]) {
                services[i]->tick(now);
            }
        }
    }
};