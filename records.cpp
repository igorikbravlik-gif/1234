#include "records.h"
#include <Preferences.h>

namespace {
Preferences prefs;
uint8_t best[LEVEL_COUNT];
uint32_t att[LEVEL_COUNT];
uint8_t bright = 130;

const char* keyBest(uint8_t i) {
  static char buf[8];
  snprintf(buf, sizeof(buf), "b%u", i);
  return buf;
}
const char* keyAtt(uint8_t i) {
  static char buf[8];
  snprintf(buf, sizeof(buf), "a%u", i);
  return buf;
}
}  // namespace

namespace Records {
void begin() {
  for (uint8_t i = 0; i < LEVEL_COUNT; i++) {
    best[i] = 0;
    att[i] = 0;
  }
  if (prefs.begin("gdash", false)) {
    for (uint8_t i = 0; i < LEVEL_COUNT; i++) {
      best[i] = prefs.getUChar(keyBest(i), 0);
      att[i] = prefs.getUInt(keyAtt(i), 0);
    }
    bright = prefs.getUChar("bright", bright);
    prefs.end();
  }
  if (bright < 16) bright = 16;
}

uint8_t bestPercent(uint8_t level) { return (level < LEVEL_COUNT) ? best[level] : 0; }
uint32_t attempts(uint8_t level) { return (level < LEVEL_COUNT) ? att[level] : 0; }

void reportAttempt(uint8_t level) {
  if (level >= LEVEL_COUNT) return;
  att[level]++;
  if (prefs.begin("gdash", false)) {
    prefs.putUInt(keyAtt(level), att[level]);
    prefs.end();
  }
}

void reportResult(uint8_t level, uint8_t percent) {
  if (level >= LEVEL_COUNT) return;
  if (percent > 100) percent = 100;
  if (percent > best[level]) {
    best[level] = percent;
    if (prefs.begin("gdash", false)) {
      prefs.putUChar(keyBest(level), best[level]);
      prefs.end();
    }
  }
}

uint8_t brightness() { return bright; }

void setBrightness(uint8_t v) {
  if (v < 16) v = 16;
  bright = v;
  if (prefs.begin("gdash", false)) {
    prefs.putUChar("bright", bright);
    prefs.end();
  }
}

void resetAll() {
  if (prefs.begin("gdash", false)) {
    prefs.clear();
    prefs.end();
  }
  for (uint8_t i = 0; i < LEVEL_COUNT; i++) {
    best[i] = 0;
    att[i] = 0;
  }
  bright = 130;
}
}  // namespace Records
