#include "motionDetector.h"
#include "inference.h"
#include "inferenceTask.h"
#include "transport.h"
#include <BleKeyboard.h>

MotionDetector motionDetector;

// Перевод действия в HID-код (клавиши — аппаратная привязка, от модели не зависит).
static uint8_t hidKeyForAction(GestureAction action) {
    switch (action) {
        case GEST_PREV_SLIDE: return KEY_LEFT_ARROW;
        case GEST_NEXT_SLIDE: return KEY_RIGHT_ARROW;
        default:              return 0;
    }
}

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
                GestureAction action = (gesture >= 0 && gesture < CLASS_COUNT)
                                       ? gestureActionById((ClassId)gesture)
                                       : GEST_NONE;

                if (action != GEST_NONE) {
                    if (gesture == voteClass) {
                        voteCount = (voteCount < DETECT_VOTE_N) ? voteCount + 1 : voteCount;
                    } else {
                        voteClass = gesture;
                        voteCount = 1;
                    }

                    if (voteCount >= DETECT_VOTE_N && !hidSent) {
                        transportSendHID(hidKeyForAction(action));

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