#include "display.h"

namespace Disp {
namespace {
M5Canvas canvas(&M5.Display);
bool ready = false;
}  // namespace

bool begin() {
  M5.Display.setRotation(1);  // landscape: 240x135
  M5.Display.fillScreen(TFT_BLACK);
  if (M5.Display.width() != Layout::W || M5.Display.height() != Layout::H) {
    Serial.printf("[GD] unexpected display size %dx%d\n", M5.Display.width(), M5.Display.height());
  }
  canvas.setColorDepth(16);
  ready = (canvas.createSprite(Layout::W, Layout::H) != nullptr);
  if (!ready) Serial.println("[GD] ERROR: sprite alloc failed");
  return ready;
}

M5Canvas& g() { return canvas; }

void clear(uint16_t color) {
  if (ready) canvas.fillScreen(color);
}

void present() {
  if (ready) canvas.pushSprite(0, 0);
}

void centerText(const char* text, int cx, int cy, const lgfx::IFont* font, uint16_t color) {
  if (!ready) return;
  canvas.setFont(font);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(color);
  canvas.drawString(text, cx, cy);
}
}  // namespace Disp
