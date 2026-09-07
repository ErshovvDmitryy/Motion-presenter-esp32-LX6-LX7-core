#include <Arduino.h>
#include "system.h"

WorkMode workMode = WORK_BUTTONS;
bool debugMode = false;

WorkMode tempMode = WORK_GESTURES;

void setMode(WorkMode mode) {
    workMode = mode;
}

WorkMode getMode() {
    return workMode;
}

void toggleGesture() {
    if ( getMode() == WORK_GESTURES ) {
        setMode(WORK_BUTTONS);
        Serial.println("Switched to WORK_BUTTONS mode");
    }
    else if (getMode() == WORK_BUTTONS) {
        setMode(WORK_GESTURES);
        Serial.println("Switched to WORK_GESTURES mode");
    }
}

void toggleDraw() {
    tempMode = getMode();
    if ( getMode() != WORK_DRAW) {
        setMode(WORK_DRAW);
    }
    else if (getMode() == WORK_DRAW) {
        setMode(tempMode);
    }
}

void toggleDebug() {
    debugMode = !debugMode;
    if (debugMode) {
        Serial.println("Debug/dataset mode ON");
    } else {
        Serial.println("Debug/dataset mode OFF");
    }
}

bool getDebug() {
    return debugMode;
}