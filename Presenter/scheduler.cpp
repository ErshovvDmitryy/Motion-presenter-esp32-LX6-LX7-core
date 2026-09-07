#include "system.h"
#include "scheduler.h"
#include <Arduino.h>

static TaskConfig tasks[TASK_COUNT] = {
  { 4000, 0, true, nullptr },     // IMU
  { 15000, 0, true, nullptr },     // Buttons
  { 4000, 0, true, nullptr },     // Gestures
  { 200000, 0, true, nullptr }     // Debug
};

// Регистрация функций
void registerTask(TaskID state, void (*callback)()) {
  if (state < TASK_COUNT) {
    tasks[state].callback = callback;
  }
}

bool shouldRunTask(TaskID state) {
  if (state >= TASK_COUNT || !tasks[state].enabled || tasks[state].callback == nullptr) {
    return false;
  }
  
  unsigned long now = micros();
  unsigned long elapsed = now - tasks[state].lastRun;
  
  if (elapsed >= tasks[state].interval) {
    tasks[state].lastRun = now;
    return true;
  }
  
  return false;
}

void runTask(TaskID state) {
  if (state < TASK_COUNT && tasks[state].callback != nullptr) {
    tasks[state].callback();
  }
}

void runScheduler() {
  for (int i = 0; i < TASK_COUNT; i++) {
    if (shouldRunTask((TaskID)i)) {
      runTask((TaskID)i);
    }
  }
}

void enableTask(TaskID state, bool enable) {
  if (state < TASK_COUNT) {
    tasks[state].enabled = enable;
  }
}

void setTaskInterval(TaskID state, unsigned long interval) {
  if (state < TASK_COUNT) {
    tasks[state].interval = interval;
  }
}