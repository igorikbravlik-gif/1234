// Geometry Dash - описание препятствий трёх уровней
#pragma once
#include <Arduino.h>

enum class ObType : uint8_t { Spike, Block, Gap };

struct Obstacle {
  int16_t x;   // позиция в мировых координатах (px)
  uint8_t w;   // ширина
  uint8_t h;   // высота (для Gap - ширина ямы в w, h не используется)
};

struct Level {
  const char* name;
  const Obstacle* spikes;
  uint8_t spikeCount;
  const Obstacle* blocks;
  uint8_t blockCount;
  const Obstacle* gaps;
  uint8_t gapCount;
  int32_t length;     // длина уровня в px, при достижении - победа
  float baseSpeed;    // стартовая скорость px/сек
  float maxSpeed;      // максимальная скорость px/сек
};

namespace Levels {
const Level& get(uint8_t index);
const char* nameOf(uint8_t index);
}  // namespace Levels
