#pragma once

#include <Arduino.h>
#include <BleKeyboard.h>
#include "imu.h"
#include "motionHistory.h"

enum TransportLink {
  LINK_SERIAL,
  LINK_BLE_HID,
  LINK_BLE_GATT
};

void transportInit();
bool transportConnected();
void transportSetLink(TransportLink link);
TransportLink transportGetLink();

bool transportSendHID(uint8_t keycode);
bool transportSendMedia(const MediaKeyReport& key);

void transportStartRecording();
void transportStopRecording();
bool transportIsRecording();

void transportSendSample(const IMU& imu);
void transportSendInferenceResult(const float* probs, int numClasses, uint8_t maxIdx, float maxVal);
