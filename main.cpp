// GEOMETRY DASH for M5StickC PLUS2 - landscape 240x135
#include <M5Unified.h>
#include "display.h"
#include "game.h"
#include "input.h"

static uint32_t lastMs = 0;

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.clear_display = true;
  cfg.output_power = false;
  cfg.internal_imu = false;  // не используется в игре - экономим RAM/время инициализации
  cfg.internal_rtc = false;
  cfg.internal_spk = true;
  cfg.internal_mic = false;
  cfg.led_brightness = 0;
  M5.begin(cfg);
  M5.Display.setBrightness(0);

  Disp::begin();
  Input::begin();
  Game::begin();

  lastMs = millis();
}

void loop() {
  M5.update();
  Input::update();

  uint32_t now = millis();
  float dt = (float)(now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt > 0.05f) dt = 0.05f;  // защита от скачка после паузы/лага - максимум ~1 кадр при 20 FPS

  Game::update(dt);
  Game::render();

  delay(1);  // неблокирующий цикл: короткая пауза, millis()-таймеры делают всю работу
}
