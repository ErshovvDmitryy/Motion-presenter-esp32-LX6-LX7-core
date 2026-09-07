#pragma once

#include <Arduino.h>
#include "imu.h"
#include "modelConfig.h"

#define MOTION_CAPTURE MODEL_WINDOW

struct MotionSample {
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    unsigned long time;
};

class MotionHistory {
public:

  const MotionSample& sample(uint8_t index) const;

  bool getWindow(uint8_t offset, MotionSample* dest, uint8_t len) const;

  void addSample(const IMU& imu);
  void clear();
  bool isFull() const;
  uint8_t size() const;
  
private:
  MotionSample samples[MOTION_BUFFER];
  uint8_t count = 0;
  uint8_t head = 0;
};

extern MotionHistory motionHistory;