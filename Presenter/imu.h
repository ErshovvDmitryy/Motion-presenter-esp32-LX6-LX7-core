#pragma once

#include <Arduino.h>
#include "math_types.h"
#include "hpf.h"

#define IMU_HPF_ALPHA 0.03f

struct IMU { 
  Vec3 gyro, accel, accelNotSm, gyroNotSm;
  unsigned long lastReadTime = 0;
  HPF hpf_ax{ IMU_HPF_ALPHA }, hpf_ay{ IMU_HPF_ALPHA }, hpf_az{ IMU_HPF_ALPHA }/*, hpf_gx{ IMU_HPF_ALPHA }, hpf_gy{ IMU_HPF_ALPHA }, hpf_gz{ IMU_HPF_ALPHA }*/; // smoothed parametrs along 3 axis 
};

extern IMU imu;

void setupIMU();
void updateIMU();