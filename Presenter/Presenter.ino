#include <Wire.h>
#include <esp_bt.h>
#include "system.h"
#include "imu.h"
#include "scheduler.h"
#include "input.h"
#include "pins.h"
#include "motionDetector.h"
#include "motionHistory.h"
#include "inference.h"
#include "inferenceTask.h"
#include "transport.h"

// ========== DECLARATION GLOBAL PARAMETRS ==========

void taskIMU() {
  updateIMU();
  transportSendSample(imu);
  
  /*static unsigned long imuCount = 0;
  static unsigned long lastFreqPrint = 0;
  imuCount++;
  unsigned long now = millis();
  if (now - lastFreqPrint >= 2000) {
    float hz = imuCount * 1000.0f / (float)(now - lastFreqPrint);
    if (!transportIsRecording()) {
        Serial.printf("IMU freq: %.1f Hz (%lu ticks / 2s)\n", hz, imuCount);
    }
    lastFreqPrint = now;
    imuCount = 0;
  }*/
}

void taskButtons(){
  handleButtons();
}

void taskGestures() {
    if (getMode() == WORK_BUTTONS) { 
        return; 
    }

    motionHistory.addSample(imu);
    
    motionDetector.updateState(imu);
}

void taskDebug() {
  static unsigned long lastPrint = 0;
  static bool firstRun = true;
  unsigned long now = millis();

  if (firstRun) {
    firstRun = false;
    lastPrint = now;
    return;
  }

  if (now - lastPrint >= 2000) {
    uint32_t n = motionDetector.getGestureCount();
    motionDetector.resetGestureCount();
    Serial.printf("Gestures in 2s: %u\n", n);
    lastPrint = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start setup");

  // for work, without not work :(
  disableCore0WDT();
  disableCore1WDT();

  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  delay(100);
  Serial.println("I2C start");
  
  transportInit();

  setupInput();

  setupIMU();

      if (!initInference()) {
        Serial.println("Failed to initialize inference!");
        while (1) delay(100);
    }

    if (!inferenceTaskInit()) {
        Serial.println("Failed to init inference task!");
        while (1) delay(100);
    }

  registerTask(TASK_IMU, taskIMU);
  registerTask(TASK_BUTTONS, taskButtons);
  registerTask(TASK_GESTURES, taskGestures);
  registerTask(TASK_SERIAL_DEBUG, taskDebug);

  enableCore0WDT();
  enableCore1WDT();

  Serial.println("Ready");
}

void loop(){
  runScheduler(); // RUN 
}