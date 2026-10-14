#pragma once

// Базовий інтерфейс для всіх мікросервісів шкатулки (SOLID: Open/Closed)
class IService {
public:
    virtual ~IService() = default;
    virtual void init() = 0;
    virtual void update() = 0;
};