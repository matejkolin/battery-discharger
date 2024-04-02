#ifndef AVERAGE_H
#define AVERAGE_H

class Average
{
private:
    float runningTotal = 0;
    float *readings;
    
    float averageReading;
    float measurements;

public:
    float average(float data, int numReadingsToAverage); // Updated method signature
    int readIndex = 0;
};

#endif
