#pragma once

class LPF { // low pass filter 
public:
  LPF(float alpha) : alpha(alpha), value(0) {}
  float update(float input) {
    value = alpha * value + (1 - alpha) * input;
    return value;
  }
private:
  float alpha;
  float value;
};