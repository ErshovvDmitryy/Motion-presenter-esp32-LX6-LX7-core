#include "motionDetector.h"
#include "inference.h"
#include "inferenceTask.h"
#include "transport.h"
#include <BleKeyboard.h>

MotionDetector motionDetector;

MotionDetectorState MotionDetector::state() const {
  return m_state;
}

uint32_t MotionDetector::getGestureCount() const {
  return gestureCount;
}

void MotionDetector::resetGestureCount() {
  gestureCount = 0;
}

void MotionDetector::updateState(const IMU& imu) {
    switch (m_state) {
        case DET_IDLE:
            if (motionHistory.size() >= MODEL_WINDOW) {
                m_state = DET_RUNNING;
                lastDetect = 0;
                voteClass = -1;
                voteCount = 0;
                hidSent = false;
            }
            break;

        case DET_COOLDOWN:
            if (millis() >= cooldownUntil) {
                voteClass = -1;
                voteCount = 0;
                hidSent = false;
                m_state = DET_RUNNING;
            }
            break;

        case DET_RUNNING: {
            if (millis() - lastDetect < DETECT_INTERVAL_MS) break;
            lastDetect = millis();

            static MotionSample window[MODEL_WINDOW];
            uint8_t start = motionHistory.size() - MODEL_WINDOW;
            if (!motionHistory.getWindow(start, window, MODEL_WINDOW)) break;

            inferenceRequest(window, MODEL_WINDOW);

            int gesture = -1;
            while (takeInferenceResult(gesture)) {
                if (gesture == 4 || gesture == 5) {

                    if (gesture == voteClass) {
                        voteCount = (voteCount < DETECT_VOTE_N) ? voteCount + 1 : voteCount;
                    } else {
                        voteClass = gesture;
                        voteCount = 1;
                    }

                    if (voteCount >= DETECT_VOTE_N && !hidSent) {
                        if (gesture == 4) transportSendHID(KEY_LEFT_ARROW);
                        else              transportSendHID(KEY_RIGHT_ARROW);

                    gestureCount++;
                    hidSent = true;
                    cooldownUntil = millis() + HID_COOLDOWN_MS;
                    m_state = DET_COOLDOWN;
                    if (!transportIsRecording()) {
                        Serial.printf("Gesture: %d (voted %d) -> HID\n", gesture, voteCount);
                        }
                    }
                } else {
                    
                    voteClass = -1;
                    voteCount = 0;
                }
            }
            break;
        }
    }
}