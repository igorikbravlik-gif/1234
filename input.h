// Geometry Dash - обработка кнопок A и B. Кнопка C (питание) намеренно не используется.
#pragma once
#include <Arduino.h>

enum class Btn : uint8_t { A, B };
enum class EvType : uint8_t { Click, Long };

struct InputEvent {
  Btn btn;
  EvType type;
};

namespace Input {
void begin();
void update();
bool poll(InputEvent& ev);
void flush();
// Быстрый флаг "кнопка A сейчас физически нажата" - нужен для мгновенного прыжка без задержки очереди
bool aHeld();
}  // namespace Input
