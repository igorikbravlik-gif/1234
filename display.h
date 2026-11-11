// Geometry Dash - слой отрисовки: спрайт 240x135 (landscape), неоновая палитра
#pragma once
#include <M5Unified.h>

namespace Layout {
constexpr int W = 240;
constexpr int H = 135;
constexpr int GROUND_Y = H - 20;  // линия земли
}  // namespace Layout

namespace Color {
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
constexpr uint16_t Black  = 0x0000;
constexpr uint16_t White  = 0xFFFF;
constexpr uint16_t Cyan   = rgb(0, 230, 255);
constexpr uint16_t Pink   = rgb(255, 40, 170);
constexpr uint16_t Yellow = rgb(255, 220, 0);
constexpr uint16_t Green  = rgb(60, 255, 140);
constexpr uint16_t Orange = rgb(255, 130, 20);
constexpr uint16_t Purple = rgb(160, 70, 255);
constexpr uint16_t Gray   = rgb(90, 90, 110);
constexpr uint16_t DimBg  = rgb(8, 6, 20);

inline uint16_t blend(uint16_t fg, uint16_t bg, uint8_t a) {
  int fr = (fg >> 11) & 0x1F, fgc = (fg >> 5) & 0x3F, fb = fg & 0x1F;
  int br = (bg >> 11) & 0x1F, bgc = (bg >> 5) & 0x3F, bb = bg & 0x1F;
  int r = br + ((fr - br) * a) / 255;
  int g = bgc + ((fgc - bgc) * a) / 255;
  int b = bb + ((fb - bb) * a) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}
}  // namespace Color

namespace Disp {
bool begin();
M5Canvas& g();
void clear(uint16_t color = Color::DimBg);
void present();
void centerText(const char* text, int cx, int cy, const lgfx::IFont* font, uint16_t color);
}  // namespace Disp
