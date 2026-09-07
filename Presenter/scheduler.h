#pragma once

enum TaskID {
  TASK_IMU = 0,
  TASK_BUTTONS,
  TASK_GESTURES,
  TASK_SERIAL_DEBUG,
  TASK_COUNT  // count of task, always of the last one
};

struct TaskConfig {
  unsigned long interval;     // time to delay task
  unsigned long lastRun;      // last time run
  bool enabled;               // task is work?
  void (*callback)();         // index func
};

void registerTask(TaskID state, void (*callback)());
bool shouldRunTask(TaskID state);
void runTask(TaskID state);
void runScheduler();
void enableTask(TaskID state, bool enable);
void setTaskInterval(TaskID state, unsigned long interval);