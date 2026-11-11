#include "game.h"
#include <math.h>
#include "display.h"
#include "input.h"
#include "levels.h"
#include "records.h"

namespace Game {
namespace {

enum class State : uint8_t { Menu, LevelSelect, Settings, Records, Playing, Paused, Dead, Win };

constexpr int PLAYER_X = 40;   // экранная позиция игрока (фикс.), мир скроллится под ним
constexpr int PLAYER_SZ = 10;
constexpr float GRAVITY = 900.0f;    // px/s^2
constexpr float JUMP_VEL = -330.0f;  // px/s

struct Particle {
  float x, y, vx, vy, life;
  uint16_t color;
  bool active;
};
constexpr uint8_t MAX_PARTICLES = 24;
Particle parts[MAX_PARTICLES];

// --- общее состояние ---
State state = State::Menu;
uint8_t menuSel = 0;      // 0 PLAY,1 LEVELS,2 SETTINGS,3 RECORDS
uint8_t levelSel = 0;     // выбранный уровень 0..2
uint8_t recSel = 0;       // для экрана рекордов
uint8_t brightPresetIdx = 1;
const uint8_t kBrightPresets[5] = {40, 80, 130, 180, 255};

// --- игровое состояние ---
float scrollX = 0.0f;
float py = 0.0f, vy = 0.0f;
bool grounded = true;
float prevBottom = 0.0f;
uint8_t finalPercent = 0;
bool wasAPressedLastFrame = false;

void spawnParticle(float x, float y, float vx, float vy, uint16_t color, float life) {
  for (auto& p : parts) {
    if (!p.active) {
      p = {x, y, vx, vy, life, color, true};
      return;
    }
  }
}

void spawnJumpDust(float x, float y) {
  for (int i = 0; i < 5; i++) {
    float a = (float)i / 4.0f;
    spawnParticle(x, y, -40.0f - a * 60.0f, -20.0f + a * 60.0f, Color::Cyan, 0.35f);
  }
}

void spawnExplosion(float x, float y) {
  static const uint16_t cols[4] = {Color::Pink, Color::Orange, Color::Yellow, Color::Cyan};
  for (int i = 0; i < 16; i++) {
    float ang = (float)i / 16.0f * 6.2832f;
    float spd = 90.0f + (i % 3) * 40.0f;
    spawnParticle(x, y, cosf(ang) * spd, sinf(ang) * spd, cols[i % 4], 0.6f);
  }
}

void updateParticles(float dt) {
  for (auto& p : parts) {
    if (!p.active) continue;
    p.life -= dt;
    if (p.life <= 0) {
      p.active = false;
      continue;
    }
    p.vy += GRAVITY * 0.5f * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
  }
}

void drawParticles() {
  auto& g = Disp::g();
  for (auto& p : parts) {
    if (!p.active) continue;
    uint8_t a = (uint8_t)(p.life * 255.0f > 255.0f ? 255 : p.life * 255.0f);
    uint16_t c = Color::blend(p.color, Color::DimBg, a);
    g.fillRect((int)p.x, (int)p.y, 2, 2, c);
  }
}

void resetLevelState(uint8_t level) {
  scrollX = 0.0f;
  py = (float)(Layout::GROUND_Y - PLAYER_SZ);
  vy = 0.0f;
  grounded = true;
  prevBottom = py + PLAYER_SZ;
  for (auto& p : parts) p.active = false;
  Records::reportAttempt(level);
}

bool aabb(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool xInRange(float x, float w, float rx, float rw) { return x < rx + rw && x + w > rx; }

float currentSpeed(const Level& lv) {
  float s = lv.baseSpeed + (scrollX / 300.0f) * 10.0f;
  if (s > lv.maxSpeed) s = lv.maxSpeed;
  return s;
}

void updatePlaying(float dt) {
  const Level& lv = Levels::get(levelSel);
  float speed = currentSpeed(lv);
  scrollX += speed * dt;

  // игрок в мировых координатах
  float worldX = scrollX + PLAYER_X;

  // пауза - долгое A
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::A && ev.type == EvType::Long) {
      state = State::Paused;
      return;
    }
    if (ev.btn == Btn::A && ev.type == EvType::Click) {
      if (grounded) {
        vy = JUMP_VEL;
        grounded = false;
        spawnJumpDust(PLAYER_X + PLAYER_SZ / 2.0f, py + PLAYER_SZ);
      }
    }
  }

  prevBottom = py + PLAYER_SZ;
  vy += GRAVITY * dt;
  py += vy * dt;

  // проверка: находится ли игрок над ямой (по x диапазону)
  bool inGap = false;
  for (uint8_t i = 0; i < lv.gapCount; i++) {
    const Obstacle& ob = lv.gaps[i];
    if (xInRange(worldX, PLAYER_SZ, ob.x, ob.w)) {
      inGap = true;
      break;
    }
  }

  if (!inGap) {
    float groundTop = (float)(Layout::GROUND_Y - PLAYER_SZ);
    if (py >= groundTop) {
      py = groundTop;
      vy = 0.0f;
      grounded = true;
    }
  } else {
    grounded = false;
    if (py > Layout::H + 10) {
      finalPercent = (uint8_t)(scrollX * 100.0f / lv.length);
      if (finalPercent > 100) finalPercent = 100;
      Records::reportResult(levelSel, finalPercent);
      spawnExplosion(PLAYER_X + PLAYER_SZ / 2.0f, Layout::H - 10.0f);
      state = State::Dead;
      return;
    }
  }

  // столкновения с шипами (смерть при касании)
  for (uint8_t i = 0; i < lv.spikeCount; i++) {
    const Obstacle& ob = lv.spikes[i];
    float sx = (float)ob.x, sy = (float)(Layout::GROUND_Y - ob.h);
    if (aabb(worldX, py, PLAYER_SZ, PLAYER_SZ, sx, sy, (float)ob.w, (float)ob.h)) {
      finalPercent = (uint8_t)(scrollX * 100.0f / lv.length);
      if (finalPercent > 100) finalPercent = 100;
      Records::reportResult(levelSel, finalPercent);
      spawnExplosion(worldX - scrollX + PLAYER_SZ / 2.0f, py + PLAYER_SZ / 2.0f);
      state = State::Dead;
      return;
    }
  }

  // столкновения с блоками: приземление сверху - ок, удар сбоку/снизу - смерть
  for (uint8_t i = 0; i < lv.blockCount; i++) {
    const Obstacle& ob = lv.blocks[i];
    float bx = (float)ob.x, by = (float)(Layout::GROUND_Y - ob.h);
    if (aabb(worldX, py, PLAYER_SZ, PLAYER_SZ, bx, by, (float)ob.w, (float)ob.h)) {
      if (vy >= 0 && prevBottom <= by + 4.0f) {
        py = by - PLAYER_SZ;
        vy = 0.0f;
        grounded = true;
      } else {
        finalPercent = (uint8_t)(scrollX * 100.0f / lv.length);
        if (finalPercent > 100) finalPercent = 100;
        Records::reportResult(levelSel, finalPercent);
        spawnExplosion(worldX - scrollX + PLAYER_SZ / 2.0f, py + PLAYER_SZ / 2.0f);
        state = State::Dead;
        return;
      }
    }
  }

  if (scrollX >= (float)lv.length) {
    Records::reportResult(levelSel, 100);
    state = State::Win;
    return;
  }

  updateParticles(dt);
}

void renderGround(const Level& lv) {
  auto& g = Disp::g();
  g.drawFastHLine(0, Layout::GROUND_Y, Layout::W, Color::Purple);
  g.fillRect(0, Layout::GROUND_Y + 1, Layout::W, Layout::H - Layout::GROUND_Y - 1, Color::Black);
  for (uint8_t i = 0; i < lv.gapCount; i++) {
    const Obstacle& ob = lv.gaps[i];
    int sx = (int)((float)ob.x - scrollX);
    if (sx + ob.w < 0 || sx > Layout::W) continue;
    g.fillRect(sx, Layout::GROUND_Y - 1, ob.w, 4, Color::DimBg);
  }
}

void renderObstacles(const Level& lv) {
  auto& g = Disp::g();
  for (uint8_t i = 0; i < lv.spikeCount; i++) {
    const Obstacle& ob = lv.spikes[i];
    int sx = (int)((float)ob.x - scrollX);
    if (sx + ob.w < -4 || sx > Layout::W + 4) continue;
    int baseY = Layout::GROUND_Y;
    g.fillTriangle(sx, baseY, sx + ob.w, baseY, sx + ob.w / 2, baseY - ob.h, Color::Pink);
    g.drawTriangle(sx, baseY, sx + ob.w, baseY, sx + ob.w / 2, baseY - ob.h, Color::White);
  }
  for (uint8_t i = 0; i < lv.blockCount; i++) {
    const Obstacle& ob = lv.blocks[i];
    int sx = (int)((float)ob.x - scrollX);
    if (sx + ob.w < -4 || sx > Layout::W + 4) continue;
    int by = Layout::GROUND_Y - ob.h;
    g.fillRect(sx, by, ob.w, ob.h, Color::rgb(30, 18, 60));
    g.drawRect(sx, by, ob.w, ob.h, Color::Purple);
    g.drawFastHLine(sx, by, ob.w, Color::Cyan);
  }
}

void renderPlayer() {
  auto& g = Disp::g();
  int squish = grounded ? 0 : (vy < 0 ? 2 : -1);
  int h = PLAYER_SZ - squish;
  int y = (int)py + squish;
  g.fillRoundRect(PLAYER_X, y, PLAYER_SZ, h, 2, Color::Yellow);
  g.drawRoundRect(PLAYER_X, y, PLAYER_SZ, h, 2, Color::White);
}

void renderHUD(const Level& lv) {
  auto& g = Disp::g();
  uint8_t pct = (uint8_t)(scrollX * 100.0f / lv.length);
  if (pct > 100) pct = 100;
  g.fillRect(0, 0, Layout::W, 3, Color::Gray);
  g.fillRect(0, 0, (int)(Layout::W * pct / 100), 3, Color::Green);
  g.setFont(&fonts::Font0);
  g.setTextColor(Color::White);
  g.setTextDatum(top_right);
  char buf[8];
  snprintf(buf, sizeof(buf), "%u%%", pct);
  g.drawString(buf, Layout::W - 3, 6);
}

void renderPlaying() {
  const Level& lv = Levels::get(levelSel);
  Disp::clear(Color::DimBg);
  renderGround(lv);
  renderObstacles(lv);
  renderPlayer();
  drawParticles();
  renderHUD(lv);
  Disp::present();
}

void renderPaused() {
  renderPlaying();  // фон - застывший кадр игры
  auto& g = Disp::g();
  g.fillRect(50, 40, 140, 55, Color::blend(Color::Black, Color::Black, 0));
  g.fillRoundRect(50, 40, 140, 55, 4, Color::rgb(10, 8, 24));
  g.drawRoundRect(50, 40, 140, 55, 4, Color::Cyan);
  Disp::centerText("PAUSE", Layout::W / 2, 58, &fonts::Font2, Color::Cyan);
  Disp::centerText("A long: resume  B: quit", Layout::W / 2, 80, &fonts::Font0, Color::Gray);
  Disp::present();
}

void updatePaused() {
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::A && ev.type == EvType::Long) state = State::Playing;
    if (ev.btn == Btn::B && ev.type == EvType::Click) state = State::Menu;
  }
}

void renderDeadWin(bool win) {
  Disp::clear(Color::DimBg);
  renderGround(Levels::get(levelSel));
  renderObstacles(Levels::get(levelSel));
  drawParticles();
  Disp::centerText(win ? "LEVEL COMPLETE!" : "YOU DIED", Layout::W / 2, 45, &fonts::Font2,
                    win ? Color::Green : Color::Pink);
  char buf[32];
  snprintf(buf, sizeof(buf), "Progress: %u%%  Best: %u%%", finalPercent, Records::bestPercent(levelSel));
  Disp::centerText(buf, Layout::W / 2, 68, &fonts::Font0, Color::White);
  snprintf(buf, sizeof(buf), "Attempts: %lu", (unsigned long)Records::attempts(levelSel));
  Disp::centerText(buf, Layout::W / 2, 84, &fonts::Font0, Color::Gray);
  Disp::centerText("B: restart   A long: menu", Layout::W / 2, 104, &fonts::Font0, Color::Cyan);
  Disp::present();
}

void updateDeadWin(float dt) {
  updateParticles(dt);
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::B && ev.type == EvType::Click) {
      resetLevelState(levelSel);
      state = State::Playing;
    }
    if (ev.btn == Btn::A && ev.type == EvType::Long) {
      state = State::Menu;
    }
  }
}

// ---------- МЕНЮ ----------
const char* kMenuItems[4] = {"PLAY", "LEVELS", "SETTINGS", "RECORDS"};

void renderMenu() {
  Disp::clear(Color::DimBg);
  auto& g = Disp::g();
  g.setFont(&fonts::Font4);
  g.setTextDatum(middle_center);
  g.setTextColor(Color::Cyan);
  g.drawString("GEOMETRY DASH", Layout::W / 2, 22);

  int startY = 55, step = 18;
  for (uint8_t i = 0; i < 4; i++) {
    bool sel = (i == menuSel);
    int y = startY + i * step;
    if (sel) {
      g.fillRoundRect(60, y - 9, 120, 18, 4, Color::rgb(30, 18, 60));
      g.drawRoundRect(60, y - 9, 120, 18, 4, Color::Yellow);
    }
    Disp::centerText(kMenuItems[i], Layout::W / 2, y, &fonts::Font2, sel ? Color::Yellow : Color::Gray);
  }
  Disp::centerText("B: next   A: select", Layout::W / 2, Layout::H - 8, &fonts::Font0, Color::Gray);
  Disp::present();
}

void updateMenu() {
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::B && ev.type == EvType::Click) {
      menuSel = (uint8_t)((menuSel + 1) % 4);
    }
    if (ev.btn == Btn::A && ev.type == EvType::Click) {
      switch (menuSel) {
        case 0:  // PLAY - сразу выбранный уровень
          resetLevelState(levelSel);
          state = State::Playing;
          break;
        case 1:
          state = State::LevelSelect;
          break;
        case 2:
          state = State::Settings;
          break;
        case 3:
          state = State::Records;
          break;
      }
    }
  }
}

// ---------- ВЫБОР УРОВНЯ ----------
void renderLevelSelect() {
  Disp::clear(Color::DimBg);
  Disp::centerText("SELECT LEVEL", Layout::W / 2, 18, &fonts::Font2, Color::Cyan);
  for (uint8_t i = 0; i < LEVEL_COUNT; i++) {
    bool sel = (i == levelSel);
    int y = 45 + i * 24;
    if (sel) {
      Disp::g().fillRoundRect(30, y - 10, 180, 20, 4, Color::rgb(30, 18, 60));
      Disp::g().drawRoundRect(30, y - 10, 180, 20, 4, Color::Yellow);
    }
    char buf[28];
    snprintf(buf, sizeof(buf), "%u. %s", i + 1, Levels::nameOf(i));
    Disp::centerText(buf, Layout::W / 2, y, &fonts::Font0, sel ? Color::Yellow : Color::Gray);
  }
  Disp::centerText("B: next   A: play   A long: back", Layout::W / 2, Layout::H - 8, &fonts::Font0, Color::Gray);
  Disp::present();
}

void updateLevelSelect() {
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::B && ev.type == EvType::Click) {
      levelSel = (uint8_t)((levelSel + 1) % LEVEL_COUNT);
    }
    if (ev.btn == Btn::A && ev.type == EvType::Click) {
      resetLevelState(levelSel);
      state = State::Playing;
    }
    if (ev.btn == Btn::A && ev.type == EvType::Long) {
      state = State::Menu;
    }
  }
}

// ---------- НАСТРОЙКИ ----------
void renderSettings() {
  Disp::clear(Color::DimBg);
  Disp::centerText("SETTINGS", Layout::W / 2, 18, &fonts::Font2, Color::Cyan);
  Disp::centerText("Brightness", Layout::W / 2, 50, &fonts::Font0, Color::Gray);
  char buf[16];
  snprintf(buf, sizeof(buf), "%u / 255", kBrightPresets[brightPresetIdx]);
  Disp::centerText(buf, Layout::W / 2, 68, &fonts::Font2, Color::Yellow);
  Disp::centerText("B: change   A long: back", Layout::W / 2, Layout::H - 8, &fonts::Font0, Color::Gray);
  Disp::present();
}

void updateSettings() {
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.btn == Btn::B && ev.type == EvType::Click) {
      brightPresetIdx = (uint8_t)((brightPresetIdx + 1) % 5);
      Records::setBrightness(kBrightPresets[brightPresetIdx]);
      M5.Display.setBrightness(kBrightPresets[brightPresetIdx]);
    }
    if (ev.btn == Btn::A && ev.type == EvType::Long) {
      state = State::Menu;
    }
  }
}

// ---------- РЕКОРДЫ ----------
void renderRecords() {
  Disp::clear(Color::DimBg);
  Disp::centerText("RECORDS", Layout::W / 2, 16, &fonts::Font2, Color::Cyan);
  for (uint8_t i = 0; i < LEVEL_COUNT; i++) {
    int y = 42 + i * 24;
    char buf[36];
    snprintf(buf, sizeof(buf), "%s", Levels::nameOf(i));
    Disp::centerText(buf, Layout::W / 2, y, &fonts::Font0, Color::White);
    snprintf(buf, sizeof(buf), "Best %u%%   Tries %lu", Records::bestPercent(i),
              (unsigned long)Records::attempts(i));
    Disp::centerText(buf, Layout::W / 2, y + 11, &fonts::Font0, Color::Gray);
  }
  Disp::centerText("A or B: back", Layout::W / 2, Layout::H - 8, &fonts::Font0, Color::Gray);
  Disp::present();
}

void updateRecords() {
  InputEvent ev;
  while (Input::poll(ev)) {
    if (ev.type == EvType::Click) state = State::Menu;
  }
}

}  // namespace

void begin() {
  Records::begin();
  brightPresetIdx = 2;
  for (uint8_t i = 0; i < 5; i++) {
    if (kBrightPresets[i] == Records::brightness()) brightPresetIdx = i;
  }
  M5.Display.setBrightness(kBrightPresets[brightPresetIdx]);
  for (auto& p : parts) p.active = false;
}

void update(float dt) {
  switch (state) {
    case State::Menu: updateMenu(); break;
    case State::LevelSelect: updateLevelSelect(); break;
    case State::Settings: updateSettings(); break;
    case State::Records: updateRecords(); break;
    case State::Playing: updatePlaying(dt); break;
    case State::Paused: updatePaused(); break;
    case State::Dead: updateDeadWin(dt); break;
    case State::Win: updateDeadWin(dt); break;
  }
}

void render() {
  switch (state) {
    case State::Menu: renderMenu(); break;
    case State::LevelSelect: renderLevelSelect(); break;
    case State::Settings: renderSettings(); break;
    case State::Records: renderRecords(); break;
    case State::Playing: renderPlaying(); break;
    case State::Paused: renderPaused(); break;
    case State::Dead: renderDeadWin(false); break;
    case State::Win: renderDeadWin(true); break;
  }
}

}  // namespace Game
