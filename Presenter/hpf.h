#pragma once

class HPF {
public:
  HPF(float alpha) : alpha(alpha), lowPass(0) {}
  
  float update(float input) {
    lowPass = alpha * input + (1.0f - alpha) * lowPass;
    return input - lowPass;
  }

private:
  float alpha;
  float lowPass;
};