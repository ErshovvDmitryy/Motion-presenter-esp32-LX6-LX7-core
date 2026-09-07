#include "motionHistory.h"

MotionHistory motionHistory;

void MotionHistory::addSample(const IMU& imu){
  samples[head].gx = imu.gyroNotSm.x / 1000.0f;
  samples[head].gy = imu.gyroNotSm.y / 1000.0f;
  samples[head].gz = imu.gyroNotSm.z / 1000.0f;
  
  samples[head].ax = imu.accel.x / 4.0f;
  samples[head].ay = imu.accel.y / 4.0f;
  samples[head].az = imu.accel.z / 4.0f;

  samples[head].time = imu.lastReadTime;

  head++;

  if(head >= MOTION_BUFFER) { head = 0; }
  if(count < MOTION_BUFFER) { count++; }
}

void MotionHistory::clear() {
  head = 0;
  count = 0;
}

bool MotionHistory::isFull() const {
  return count >= MOTION_CAPTURE;
}

const MotionSample& MotionHistory::sample(uint8_t index) const {
  uint8_t start = (head + MOTION_BUFFER - count) % MOTION_BUFFER;
  return samples[(start + index) % MOTION_BUFFER];
}

bool MotionHistory::getWindow(uint8_t offset, MotionSample* dest, uint8_t len) const {
  if (offset + len > count) return false;
  uint8_t start = (head + MOTION_BUFFER - count) % MOTION_BUFFER;
  for (uint8_t i = 0; i < len; i++) {
    dest[i] = samples[(start + offset + i) % MOTION_BUFFER];
  }
  return true;
}

uint8_t MotionHistory::size() const {
  return count;
}