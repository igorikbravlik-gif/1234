// Geometry Dash - сохранение рекордов и попыток во Flash (NVS Preferences)
#pragma once
#include <Arduino.h>

constexpr uint8_t LEVEL_COUNT = 3;

namespace Records {
void begin();
uint8_t bestPercent(uint8_t level);      // 0..100
uint32_t attempts(uint8_t level);
void reportAttempt(uint8_t level);                // +1 попытка
void reportResult(uint8_t level, uint8_t percent);  // обновляет рекорд, если лучше
uint8_t brightness();
void setBrightness(uint8_t v);
void resetAll();
}  // namespace Records
