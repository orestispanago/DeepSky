#ifndef Measurement_h
#define Measurement_h

#include "Arduino.h"

class Measurement
{
public:
    Measurement();
    long getCount();
    float getCurrent();
    float getTotal();
    float getAverage();
    void update(float newVal);
    void reset();
    String toString();

private:
    long count;
    float current;
    float total;
    float average;
};

#endif