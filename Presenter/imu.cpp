#include <Wire.h>
#include "imu.h"

static constexpr uint8_t MPU_ADDR = 0x68;

// ===== MPU-6050 full-scale ranges =====
// GYRO_FS_SEL:  0 = ±250, 1 = ±500, 2 = ±1000, 3 = ±2000 deg/s
#define GYRO_FS_SEL  2
// ACCEL_FS_SEL: 0 = ±2g,  1 = ±4g,  2 = ±8g,   3 = ±16g
#define ACCEL_FS_SEL 1

static constexpr float gyroScaleFactor() {
  return GYRO_FS_SEL == 1 ? 65.5f :
         GYRO_FS_SEL == 2 ? 32.8f :
         GYRO_FS_SEL == 3 ? 16.4f : 131.0f;
}

static constexpr float accelScaleFactor() {
  return ACCEL_FS_SEL == 1 ? 8192.0f :
         ACCEL_FS_SEL == 2 ? 4096.0f :
         ACCEL_FS_SEL == 3 ? 2048.0f : 16384.0f;
}

IMU imu;

int16_t readRegister16(uint8_t addr, uint8_t reg) {
  uint8_t buffer[2];
  
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  
  Wire.requestFrom((uint8_t)addr, (size_t)2, (bool)true);
  
  uint32_t timeout = millis() + 10;
  while(Wire.available() < 2) {
    if(millis() > timeout) return 0;
  }
  
  buffer[0] = Wire.read();
  buffer[1] = Wire.read();
  
  return (buffer[0] << 8) | buffer[1];
}

void setupIMU() {
  Serial.print("Initializing MPU-6050...");
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);              // GYRO_CONFIG
  Wire.write(GYRO_FS_SEL << 3);
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);              // ACCEL_CONFIG
  Wire.write(ACCEL_FS_SEL << 3);
  Wire.endTransmission();

  Serial.print("Gyro FS_SEL=");
  Serial.print(GYRO_FS_SEL);
  Serial.print(" Accel FS_SEL=");
  Serial.println(ACCEL_FS_SEL);

  delay(100);
}

void updateIMU() {
  unsigned long now = micros();
  //float dt = (now - imu.lastReadTime) / 1000000.0f;
  imu.lastReadTime = now;

  //if (dt <= 0 || dt > 0.1f) dt = 0.01f;
  
  uint8_t buffer[14];
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR, (size_t)14, (bool)true);
  
  for(int i = 0; i < 14; i++) buffer[i] = Wire.read();

  int16_t ax_raw = (buffer[0] << 8) | buffer[1];
  int16_t ay_raw = (buffer[2] << 8) | buffer[3];
  int16_t az_raw = (buffer[4] << 8) | buffer[5];
  int16_t gx_raw = (buffer[8] << 8) | buffer[9];
  int16_t gy_raw = (buffer[10] << 8) | buffer[11];
  int16_t gz_raw = (buffer[12] << 8) | buffer[13];

  float ax = ax_raw / accelScaleFactor();
  float ay = ay_raw / accelScaleFactor();
  float az = az_raw / accelScaleFactor();
  float gx = gx_raw / gyroScaleFactor();
  float gy = gy_raw / gyroScaleFactor();
  float gz = gz_raw / gyroScaleFactor();

  imu.accelNotSm.x = ax;
  imu.accelNotSm.y = ay;
  imu.accelNotSm.z = az;
  imu.gyroNotSm.x = gx;
  imu.gyroNotSm.y = gy;
  imu.gyroNotSm.z = gz;

  ax = imu.hpf_ax.update(ax);
  ay = imu.hpf_ay.update(ay);
  az = imu.hpf_az.update(az);
  /*gx = imu.hpf_ax.update(gx);
  gy = imu.hpf_ay.update(gy);
  gz = imu.hpf_az.update(gz);*/

  imu.accel.x = ax;
  imu.accel.y = ay;
  imu.accel.z = az;
  /*imu.gyro.x = gx;
  imu.gyro.y = gy;
  imu.gyro.z = gz;*/

  /*float pitchAcc = atan2(ay, sqrt(ax*ax + az*az)) * 180.0f / PI;
  float rollAcc  = atan2(-ax, az) * 180.0f / PI;

  const float alpha = 0.98f;
  imu.pitch = alpha * (imu.pitch + gx * dt) + (1.0f - alpha) * pitchAcc;
  imu.roll  = alpha * (imu.roll  + gy * dt) + (1.0f - alpha) * rollAcc;*/
}
