// inference.h
#pragma once

#include <Arduino.h>
#include "motionHistory.h"
#include "modelConfig.h"


#ifndef MOTION_WINDOW
#define MOTION_WINDOW MODEL_WINDOW
#endif

bool initInference();
int runInference(MotionSample* window, int len);
