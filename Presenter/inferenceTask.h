#pragma once

#include "motionHistory.h"
#include "modelConfig.h"

bool inferenceTaskInit();

bool inferenceTaskBusy();

void inferenceRequest(const MotionSample* window, int len);

bool takeInferenceResult(int& outClass);