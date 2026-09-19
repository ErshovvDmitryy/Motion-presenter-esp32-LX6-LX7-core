#pragma once

// ============================================================================
// UNIFIED MODEL / WINDOW / DATASET CONFIGURATION
// ============================================================================
// All window and inference settings reside here — a single source of truth.
// Changing the dataset = include the new gesture_model_data.h and update:
//   - MODEL_WINDOW      (must match the model's input[1]),
//   - MODEL_CLASSES     (number of rows in class_names.txt),
//   - enum ClassId      (order = order of training report rows),
//   - MODEL_MEAN/STDDEV (normalization parameters of the new dataset),
//   - gestureActionById (which class -> which device action).

// Window length (samples). Equal to the second element of the model's input tensor [1, WINDOW, 6].
#define MODEL_WINDOW   49

// Number of features per sample (ax, ay, az, gx, gy, gz).
#define MODEL_FEATURES 6

// Number of model classes (matches the number of rows from class_names.txt).
#define MODEL_CLASSES  8

// Ыtride sliding detection window in samples
// Corresponds to dataset slicing: windows 0-80, 20-100, 40-120...
#define MODEL_STRIDE   10

// Buffer margin beyond the window (allows the window to "slide" over fresh samples)
#define MODEL_BIAS     20

// Interval (ms) between inferences during continuous detection.
// 40 ms = ~25 detections/s.
#define DETECT_INTERVAL_MS 25

// Minimum swipe class confidence for detection.
#define DETECT_CONF_THRESH 0.80f

// How much need detected movement, for send HID command. if  1 = send HID at once.
#define DETECT_VOTE_N 2

// Cooldown (ms) afret send HID command.
#define HID_COOLDOWN_MS 600

// Size TFLM ().
#define MODEL_ARENA_BYTES (80 * 1024)

// ============================================================================
// CLASS REFERENCE.
// The order must match the training report (labels.txt / class_names.txt):
// value of each element = output model class index (0..MODEL_CLASSES-1).
// ============================================================================
enum ClassId {
    CLASS_CIRCLE_CCW           = 0,  // CircleCCW
    CLASS_CIRCLE_CW            = 1,  // CircleCW
    CLASS_NORMAL_HAND_MOVEMENT = 2,  // NormalHandMovement
    CLASS_SWIPE_DOWN           = 3,  // SwipeDown
    CLASS_SWIPE_LEFT           = 4,  // SwipeLeft
    CLASS_SWIPE_RIGHT          = 5,  // SwipeRight
    CLASS_SWIPE_UP             = 6,  // SwipeUp
    CLASS_UNKNOWN              = 7,  // Unknown
    CLASS_COUNT
};

static_assert(CLASS_COUNT == MODEL_CLASSES,
              "CLASS_COUNT (enum ClassId) must equal MODEL_CLASSES");

// Names class (mirror enum) — for logs and debug
static const char* const CLASS_NAMES[CLASS_COUNT] = {
    "CircleCCW",
    "CircleCW",
    "NormalHandMovement",
    "SwipeDown",
    "SwipeLeft",
    "SwipeRight",
    "SwipeUp",
    "Unknown",
};

// ============================================================================
// COMMANDS, uses where detected class motion
// ============================================================================
enum GestureAction {
    GEST_NONE,
    GEST_PREV_SLIDE,
    GEST_NEXT_SLIDE,
    GEST_SCROLL_UP,
    GEST_SCROLL_DOWN,
    GEST_REFRESH,
    GEST_CLOSE_TAB,
};

inline GestureAction gestureActionById(ClassId id) {
    switch (id) {
        case CLASS_SWIPE_LEFT:   return GEST_PREV_SLIDE;    // KEY_LEFT
        case CLASS_SWIPE_RIGHT:  return GEST_NEXT_SLIDE;    // KEY_RIGHT
        case CLASS_SWIPE_UP:     return GEST_SCROLL_UP;     // Page Up
        case CLASS_SWIPE_DOWN:   return GEST_SCROLL_DOWN;   // Page Down
        case CLASS_CIRCLE_CW:    return GEST_REFRESH;       // F5
        case CLASS_CIRCLE_CCW:   return GEST_CLOSE_TAB;     // Esc
        default:                 return GEST_NONE;
    }
}

// ax, ay, az, gx, gy, gz.
static const float MODEL_MEAN[MODEL_FEATURES] = {
    -0.00831f, 0.00165f, 0.00069f,
    0.00271f, 0.00001f, 0.00012f
};

static const float MODEL_STDDEV[MODEL_FEATURES] = {
    0.08723187f, 0.18882504f, 0.13767117f,
    0.11765759f, 0.14244981f, 0.21598291f
};

#define MOTION_BUFFER (MODEL_WINDOW + MODEL_BIAS)
#define NUM_INPUTS    (MODEL_WINDOW * MODEL_FEATURES)