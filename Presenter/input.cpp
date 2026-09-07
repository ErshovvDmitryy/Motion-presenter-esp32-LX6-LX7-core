#include "input.h"
#include "system.h"
#include "transport.h"
#include "pins.h"
#include "scheduler.h"

#define GESTURE_BTN_DEBOUNCE 50
#define RECORD_HOLD_MS 500
#define DEBUG_HOLD_MS 5000

static int lastStates[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
static unsigned long lastPressTimes[6] = {0};

void setupInput(){
  Serial.println("Buttons start setup");
  pinMode(BTN_UP,       INPUT_PULLUP);
  pinMode(BTN_DOWN,     INPUT_PULLUP);
  pinMode(BTN_LEFT,     INPUT_PULLUP);
  pinMode(BTN_RIGHT,    INPUT_PULLUP);
  pinMode(BTN_PLAY,     INPUT_PULLUP);
  pinMode(BTN_GESTURE,  INPUT_PULLUP);
  
  Serial.println("Buttons initialized");
}

void handleButton(int pin, uint8_t keycode, int idx){
  int currentState = digitalRead(pin);
  unsigned long now = millis();

  if (currentState != lastStates[idx]) {
    lastStates[idx] = currentState;
    
    if (currentState == LOW && (now - lastPressTimes[idx]) > GESTURE_BTN_DEBOUNCE) {
      lastPressTimes[idx] = now;
      if (transportSendHID(keycode)) {
        Serial.print("Button pressed, sending: 0x");
        Serial.println(keycode, HEX);
      }
    }
  }
}

void handleMediaButton(int pin, const MediaKeyReport& keycode, int idx) {
  int currentState = digitalRead(pin);
  unsigned long now = millis();
  
  if (currentState == LOW && lastStates[idx] == HIGH && (now - lastPressTimes[idx]) > 10) {
    lastPressTimes[idx] = now;
    if (transportSendMedia(keycode)) {
      Serial.println("Media button pressed");
    } else {
      Serial.println("HID not sent (no BLE HID link)");
    }
  }
  lastStates[idx] = currentState;
}

void handleDebugMediaButton(int pin, const MediaKeyReport& keycode, int idx) {
  static bool pressActive = false;
  static unsigned long pressStart = 0;
  static bool debugTriggered = false;
  int currentState = digitalRead(pin);
  unsigned long now = millis();

  if (currentState == LOW && !pressActive && (now - lastPressTimes[idx]) > GESTURE_BTN_DEBOUNCE) {
    pressActive = true;
    pressStart = now;
    lastPressTimes[idx] = now;
    debugTriggered = false;
  }
  else if (currentState == LOW && pressActive && !debugTriggered && (now - pressStart) >= DEBUG_HOLD_MS) {
    debugTriggered = true;
    toggleDebug();

    transportSetLink(getDebug() ? LINK_SERIAL : LINK_BLE_HID);
    enableTask(TASK_SERIAL_DEBUG, getDebug());
    Serial.println(getDebug() ? "Debug serial streaming ON" : "Back to BLE HID");
  }
  else if (currentState == HIGH && pressActive) {
    pressActive = false;
    if (!debugTriggered && (now - pressStart) < DEBUG_HOLD_MS) {

      if (transportSendMedia(keycode)) {
        Serial.println("Media button pressed");
      } else {
        Serial.println("HID not sent (no BLE HID link)");
      }
    }
  }
  lastStates[idx] = currentState;
}

void handleRecordButton(uint8_t pin, unsigned long& lastPressTime) {
  static bool pressActive = false;
  static bool recordingState = false;
  static unsigned long pressStart = 0;
  int currentState = digitalRead(pin);
  unsigned long now = millis();

  if (currentState == LOW && !pressActive && (now - lastPressTime) > GESTURE_BTN_DEBOUNCE) {
    pressActive = true;
    pressStart = now;
    lastPressTime = now;
  }
  else if (currentState == LOW && pressActive && !recordingState && (now - pressStart) >= RECORD_HOLD_MS) {
    recordingState = true;
    transportStartRecording();
  }
  else if (currentState == HIGH && pressActive) {
    pressActive = false;
    if (recordingState) {
      recordingState = false;
      transportStopRecording();
    }
  }
}

void handleGestureButton(uint8_t pin, unsigned long& lastPressTime) {
  static int lastState = HIGH;
  int currentState = digitalRead(pin);
  unsigned long now = millis();

  if (currentState == LOW && lastState == HIGH && (now - lastPressTime) > GESTURE_BTN_DEBOUNCE) {
    toggleGesture();
    lastPressTime = now;
    Serial.println("Gesture mode switch");
  }
  lastState = currentState;
}

void handleButtons() {
  if (transportGetLink() == LINK_SERIAL) {
    handleRecordButton(BTN_LEFT, lastPressTimes[2]);
  } else {
    handleButton(BTN_LEFT, KEY_LEFT_ARROW, 2);
  }

  handleButton(BTN_UP,    KEY_UP_ARROW,    0);
  handleButton(BTN_DOWN,  KEY_DOWN_ARROW,  1);
  handleButton(BTN_RIGHT, KEY_RIGHT_ARROW, 3);

  handleDebugMediaButton(BTN_PLAY, KEY_MEDIA_PLAY_PAUSE, 4);

  handleGestureButton(BTN_GESTURE, lastPressTimes[5]);
}
