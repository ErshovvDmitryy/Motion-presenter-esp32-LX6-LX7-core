#pragma once

#include "imu.h"
#include "motionHistory.h"
#include "modelConfig.h"

enum MotionDetectorState {
  DET_IDLE,
  DET_RUNNING,
  DET_COOLDOWN
};

class MotionDetector {
public:

  void updateState(const IMU& imu);

  MotionDetectorState state() const;

  uint32_t getGestureCount() const;
  void resetGestureCount();

private:

  MotionDetectorState m_state = DET_IDLE;

  unsigned long lastDetect = 0;   // время последнего инференса
  unsigned long cooldownUntil = 0;

  int   voteClass = -1;   // класс, за который копим голоса
  int   voteCount = 0;    // число согласованных окон подряд

  bool  hidSent = false;
  
  uint32_t gestureCount = 0;
};

extern MotionDetector motionDetector;