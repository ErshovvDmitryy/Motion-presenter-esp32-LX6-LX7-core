#pragma once

enum WorkMode {
    WORK_BUTTONS,
    WORK_GESTURES,
    WORK_DRAW
};

// Prototypes class
extern WorkMode workMode;
extern bool debugMode;

// Prototypes func
void toggleMode();
WorkMode getMode();
void setMode(WorkMode mode);
void toggleGesture();
void toggleDraw();
void toggleDebug();
bool getDebug();