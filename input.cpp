#include "input.h"

// Распиновка M5StickC PLUS2: A = GPIO37, B = GPIO39 (нажатие = LOW)
namespace {
constexpr uint8_t PIN_A = 37;
constexpr uint8_t PIN_B = 39;
constexpr uint32_t DEBOUNCE_MS = 20;
constexpr uint32_t LONG_MS = 550;

struct Button {
  uint8_t pin;
  Btn id;
  bool raw, stable, longFired;
  uint32_t changedAt, pressedAt;
};

Button btns[2] = {
    {PIN_A, Btn::A, false, false, false, 0, 0},
    {PIN_B, Btn::B, false, false, false, 0, 0},
};

constexpr uint8_t QN = 8;
InputEvent q[QN];
uint8_t qh = 0, qt = 0;

void push(Btn b, EvType t) {
  uint8_t n = (uint8_t)((qt + 1) % QN);
  if (n == qh) return;
  q[qt] = {b, t};
  qt = n;
}
}  // namespace

namespace Input {
void begin() {
  uint32_t now = millis();
  for (auto& b : btns) {
    pinMode(b.pin, INPUT);
    b.raw = b.stable = (digitalRead(b.pin) == LOW);
    b.changedAt = now;
    b.longFired = b.stable;  // если зажата при старте - не даём ложный клик
  }
}

void update() {
  uint32_t now = millis();
  for (auto& b : btns) {
    bool r = (digitalRead(b.pin) == LOW);
    if (r != b.raw) {
      b.raw = r;
      b.changedAt = now;
    }
    if (b.raw != b.stable && (now - b.changedAt) >= DEBOUNCE_MS) {
      b.stable = b.raw;
      if (b.stable) {
        b.pressedAt = now;
        b.longFired = false;
      } else if (!b.longFired) {
        push(b.id, EvType::Click);
      }
    }
    if (b.stable && !b.longFired && (now - b.pressedAt) >= LONG_MS) {
      b.longFired = true;
      push(b.id, EvType::Long);
    }
  }
}

bool poll(InputEvent& ev) {
  if (qh == qt) return false;
  ev = q[qh];
  qh = (uint8_t)((qh + 1) % QN);
  return true;
}

void flush() { qh = qt; }

bool aHeld() { return btns[0].stable; }
}  // namespace Input
