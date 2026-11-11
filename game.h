// Geometry Dash - основной игровой модуль: меню, уровни, физика, рендер
#pragma once
#include <Arduino.h>

namespace Game {
void begin();
void update(float dt);  // dt в секундах
void render();
}  // namespace Game
