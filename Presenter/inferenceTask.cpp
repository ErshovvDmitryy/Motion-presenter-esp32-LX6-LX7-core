#include "inferenceTask.h"
#include "inference.h"
#include "transport.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ==========================================================================
// СООБЩЕНИЕ (окно входа + результат) между main (core 1) и infer (core 0)
// ==========================================================================

static MotionSample s_window[MODEL_WINDOW];
static int    s_resultClass = -1;
static bool   s_resultFresh = false;
static bool   s_busy = false;

static SemaphoreHandle_t s_winReady = nullptr;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static void inferenceTask(void* param) {
    (void)param;

    Serial.println("Inference task started");

    for (;;) {
        if (xSemaphoreTake(s_winReady, portMAX_DELAY) != pdPASS) continue;

        MotionSample localWindow[MODEL_WINDOW];
        portENTER_CRITICAL(&s_mux);
        memcpy(localWindow, s_window, sizeof(s_window));
        portEXIT_CRITICAL(&s_mux);

        //unsigned long t0 = micros();
        int cls = runInference(localWindow, MODEL_WINDOW);
        /*unsigned long dt = micros() - t0;
        if (!transportIsRecording()) {
            Serial.printf("Infer: %lu us\n", dt);
        }*/

        portENTER_CRITICAL(&s_mux);
        s_resultClass = cls;
        s_resultFresh = true;
        s_busy = false;
        portEXIT_CRITICAL(&s_mux);
    }
}

bool inferenceTaskInit() {
    s_winReady = xSemaphoreCreateBinary();
    if (s_winReady == nullptr) return false;

    BaseType_t ok = xTaskCreatePinnedToCore(
        inferenceTask,
        "tflmInfer",
        16384,
        nullptr,
        1 | tskIDLE_PRIORITY,
        nullptr,
        0
    );

    return (ok == pdPASS);
}

bool inferenceTaskBusy() {
    bool b;
    portENTER_CRITICAL(&s_mux);
    b = s_busy;
    portEXIT_CRITICAL(&s_mux);
    return b;
}

void inferenceRequest(const MotionSample* window, int len) {
    if (len > MODEL_WINDOW) len = MODEL_WINDOW;

    portENTER_CRITICAL(&s_mux);
    if (s_busy) {
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    memcpy(s_window, window, sizeof(MotionSample) * len);
    s_busy = true;
    portEXIT_CRITICAL(&s_mux);
    
    xSemaphoreGive(s_winReady);
}

bool takeInferenceResult(int& outClass) {
    bool got = false;
    portENTER_CRITICAL(&s_mux);
    if (s_resultFresh) {
        outClass = s_resultClass;
        s_resultFresh = false;
        got = true;
    }
    portEXIT_CRITICAL(&s_mux);
    return got;
}