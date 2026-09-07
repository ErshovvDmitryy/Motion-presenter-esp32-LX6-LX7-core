#include "transport.h"

static BleKeyboard bleKeyboard("Presenter", "Espressif", 100);

static TransportLink transportLink = LINK_BLE_HID;

void transportInit() {
  
  Serial.begin(115200);


  while (!Serial) {
    delay(10);
  }

  bleKeyboard.begin();

  Serial.println("Transport initialized (LINK_BLE_HID)");
}

bool transportConnected() {
  switch (transportLink) {
    case LINK_BLE_HID: return bleKeyboard.isConnected();
    case LINK_BLE_GATT: return false; // stub, see later
    case LINK_SERIAL:
    default: return true;
  }
}

void transportSetLink(TransportLink link) {
  transportLink = link;

  switch (link) {
    case LINK_SERIAL:    Serial.println("Transport link: LINK_SERIAL");    break;
    case LINK_BLE_HID:   Serial.println("Transport link: LINK_BLE_HID");   break;
    case LINK_BLE_GATT:  Serial.println("Transport link: LINK_BLE_GATT");  break; // stub, see later
  }
}

TransportLink transportGetLink() {
  return transportLink;
}

bool transportSendHID(uint8_t keycode) {
  if (transportLink != LINK_BLE_HID || !bleKeyboard.isConnected()) return false;
  bleKeyboard.write(keycode);
  return true;
}

bool transportSendMedia(const MediaKeyReport& key) {
  if (transportLink != LINK_BLE_HID || !bleKeyboard.isConnected()) return false;
  bleKeyboard.write(key);
  return true;
}

static bool recording = false;
static uint16_t segmentCrc = 0xFFFF;
static uint16_t segmentCount = 0;

static uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc) {
  for (size_t i = 0; i < len; i++) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      else
        crc = static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

void transportStartRecording() {
  recording = true;
  segmentCrc = 0xFFFF;
  segmentCount = 0;
}

void transportStopRecording() {
  recording = false;
  uint8_t packet[5];
  packet[0] = 107;
  memcpy(&packet[1], &segmentCount, sizeof(uint16_t));
  memcpy(&packet[3], &segmentCrc, sizeof(uint16_t));
  Serial.write(packet, sizeof(packet));
  segmentCrc = 0xFFFF;
  segmentCount = 0;
}

bool transportIsRecording() {
  return recording;
}

void transportSendSample(const IMU& imu) {
  if (transportLink != LINK_SERIAL) return; // GATT see later
  if (!recording) return;

  MotionSample s;
  s.ax = imu.accel.x;
  s.ay = imu.accel.y;
  s.az = imu.accel.z;
  s.gx = imu.gyroNotSm.x;
  s.gy = imu.gyroNotSm.y;
  s.gz = imu.gyroNotSm.z;
  s.time = imu.lastReadTime;

  if (recording) {
    segmentCrc = crc16_ccitt(reinterpret_cast<const uint8_t*>(&s), sizeof(MotionSample), segmentCrc);
    segmentCount++;
  }

  uint8_t packet[1 + sizeof(MotionSample) + 1];
  packet[0] = 106;
  memcpy(&packet[1], &s, sizeof(MotionSample));
  packet[1 + sizeof(MotionSample)] = recording ? 1 : 0;
  Serial.write(packet, sizeof(packet));
}

void transportSendInferenceResult(const float* probs, int numClasses, uint8_t maxIdx, float maxVal) {
  if (transportLink != LINK_SERIAL) return;

  const int PROBS_BYTES = numClasses * sizeof(float);
  const int PACKET_SIZE = 1 + PROBS_BYTES + 1 + sizeof(float);

  uint8_t packet[PACKET_SIZE];
  packet[0] = 108;
  memcpy(&packet[1], probs, PROBS_BYTES);
  packet[1 + PROBS_BYTES] = maxIdx;
  memcpy(&packet[1 + PROBS_BYTES + 1], &maxVal, sizeof(float));

  Serial.write(packet, PACKET_SIZE);
}
