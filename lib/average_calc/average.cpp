#include "average.h"
/// @brief 
/// @param data: Data to calculate average 
/// @param numReadingsToAverage: Number of samples to average
/// @return 

float Average::average(float data, int numReadingsToAverage)
{
    // Allocate memory for the readings array based on numReadingsToAverage
    if (!readings)
    {
        readings = new float[numReadingsToAverage]();
    }

    measurements = data;

    // Subtract the last reading:
    runningTotal = runningTotal - readings[readIndex];
    // Read from the sensor and store the new reading in the array:
    readings[readIndex] = measurements;
    // Add the new reading to the running total:
    runningTotal = runningTotal + readings[readIndex];
    // Move to the next position in the array:
    readIndex = (readIndex + 1) % numReadingsToAverage;

    // If we have enough readings (numReadingsToAverage), calculate the average:
    if (readIndex == 0)
    {
        averageReading = static_cast<float>(runningTotal) / numReadingsToAverage;
    }

    return averageReading;
}
