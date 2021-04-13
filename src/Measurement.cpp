#include "Arduino.h"
#include "Measurement.h"

Measurement::Measurement()
{
}

float Measurement::getCurrent()
{
    return current;
}

float Measurement::getTotal()
{
    return total;
}

float Measurement::getAverage()
{
    return average;
}

long Measurement::getCount()
{
    return count;
}

void Measurement::update(float newVal)
{
    count++;
    current = newVal;
    total = total + current;
    average = (total / count);
}
void Measurement::reset()
{
    total = 0.0;
    count = 0.0;
}

String Measurement::toString()
{
    return "Measurement: { count: " + String(count) + "," +
           " current: " + String(current) + "," +
           " total: " + String(total) + "," +
           " average: " + String(average) + " }";
}