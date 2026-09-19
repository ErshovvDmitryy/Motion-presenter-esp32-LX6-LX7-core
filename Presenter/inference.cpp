// inference.cpp
#include "inference.h"
#include "gesture_model_data.h"
#include "motionHistory.h"
#include "transport.h"
#include "system.h"

#include "tflm_esp32.h"

#define WINDOW_SIZE MOTION_WINDOW
#define NUM_FEATURES MODEL_FEATURES
#define NUM_CLASSES MODEL_CLASSES
#define NUM_INPUTS  (WINDOW_SIZE * NUM_FEATURES)

#define TF_NUM_OPS 11
#define ARENA_SIZE MODEL_ARENA_BYTES

// Параметры нормализации берутся из единой конфигурации модели (modelConfig.h).
const float (&mean)[MODEL_FEATURES]   = MODEL_MEAN;
const float (&stddev)[MODEL_FEATURES] = MODEL_STDDEV;

// ============================================================================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ TFLM (создаём один раз, используем повторно)
// ============================================================================

static tflite::ErrorReporter* error_reporter = nullptr;
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;
static uint8_t* tensor_arena = nullptr;

static tflite::MicroMutableOpResolver<TF_NUM_OPS> resolver;

bool initInference() {
    Serial.println("Loading model...");
    resolver.AddQuantize();
    resolver.AddExpandDims();
    resolver.AddConv2D();
    resolver.AddReshape();
    resolver.AddMaxPool2D();
    resolver.AddMean();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddDequantize();

    model = tflite::GetModel(gesture_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model version mismatch. Expected %d, got %d\n",
                      TFLITE_SCHEMA_VERSION, model->version());
        return false;
    }

    tensor_arena = (uint8_t*) malloc(ARENA_SIZE);
    if (tensor_arena == nullptr) {
        Serial.println("Couldn't allocate tensor arena");
        return false;
    }

    interpreter = new tflite::MicroInterpreter(model, resolver,
                                               tensor_arena, ARENA_SIZE);
    if (interpreter == nullptr) {
        Serial.println("Failed to create interpreter");
        return false;
    }

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("AllocateTensors() failed");
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("Inference initialized!");
    Serial.printf("   Model size: %d bytes\n", gesture_model_len);
    Serial.printf("   Input size: %d\n", NUM_INPUTS);
    Serial.printf("   Output size: %d\n", NUM_CLASSES);
    for (int i = 0; i < MODEL_CLASSES; i++) {
        Serial.printf("      class %2d: %s\n", i, CLASS_NAMES[i]);
    }

    return true;
}

// ==========================================================================
// НОРМАЛИЗАЦИЯ
// ==========================================================================

void normalizeWindow(MotionSample* window, int len, float* output) {
    for (int i = 0; i < len; i++) {
        output[i * 6 + 0] = (window[i].ax - mean[0]) / stddev[0];
        output[i * 6 + 1] = (window[i].ay - mean[1]) / stddev[1];
        output[i * 6 + 2] = (window[i].az - mean[2]) / stddev[2];
        output[i * 6 + 3] = (window[i].gx - mean[3]) / stddev[3];
        output[i * 6 + 4] = (window[i].gy - mean[4]) / stddev[4];
        output[i * 6 + 5] = (window[i].gz - mean[5]) / stddev[5];
    }
}

int runInference(MotionSample* window, int len) {
    if (len > WINDOW_SIZE) len = WINDOW_SIZE;

    static float input_data[NUM_INPUTS];
    static float probs[NUM_CLASSES];
    normalizeWindow(window, len, input_data);

    switch (input->type) {
        case kTfLiteFloat32: {
            float* dst = input->data.f;
            for (int i = 0; i < NUM_INPUTS; i++) dst[i] = input_data[i];
            break;
        }
        case kTfLiteInt8: {
            const float scale = input->params.scale;
            const int32_t zero_point = input->params.zero_point;
            int8_t* dst = input->data.int8;
            for (int i = 0; i < NUM_INPUTS; i++) {
                dst[i] = (int8_t) roundf(input_data[i] / scale) + zero_point;
            }
            break;
        }
        case kTfLiteUInt8: {
            const float scale = input->params.scale;
            const int32_t zero_point = input->params.zero_point;
            uint8_t* dst = input->data.uint8;
            for (int i = 0; i < NUM_INPUTS; i++) {
                dst[i] = (uint8_t) roundf(input_data[i] / scale) + zero_point;
            }
            break;
        }
        default: {
            Serial.printf("Unsupported input type: %d\n", input->type);
            return -1;
        }
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Invoke() failed");
        return -1;
    }

    switch (output->type) {
        case kTfLiteFloat32: {
            const float* src = output->data.f;
            for (int i = 0; i < NUM_CLASSES; i++) probs[i] = src[i];
            break;
        }
        case kTfLiteInt8: {
            const float scale = output->params.scale;
            const int32_t zero_point = output->params.zero_point;
            const int8_t* src = output->data.int8;
            for (int i = 0; i < NUM_CLASSES; i++) {
                probs[i] = ((float) src[i] - zero_point) * scale;
            }
            break;
        }
        case kTfLiteUInt8: {
            const float scale = output->params.scale;
            const int32_t zero_point = output->params.zero_point;
            const uint8_t* src = output->data.uint8;
            for (int i = 0; i < NUM_CLASSES; i++) {
                probs[i] = ((float) src[i] - zero_point) * scale;
            }
            break;
        }
        default: {
            Serial.printf("Unsupported output type: %d\n", output->type);
            return -1;
        }
    }

    int maxIdx = 0;
    float maxVal = probs[0];
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (probs[i] > maxVal) {
            maxVal = probs[i];
            maxIdx = i;
        }
    }

    if (getDebug() && !transportIsRecording()) {
        transportSendInferenceResult(probs, NUM_CLASSES, (uint8_t)maxIdx, maxVal);
    }

    if (maxVal > 0.8f) {
        return maxIdx;
    }

    return -1;
}

