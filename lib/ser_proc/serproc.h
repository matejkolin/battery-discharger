// serialproc.h
// process commands coming from serial
#pragma once

#include "Arduino.h"
#include <string>
#include <vector>
#include <cstring>

#define SER_BUFFER_SIZE 256
#define cmdArSize 20
const char cmdArray[cmdArSize][7] = {"ccc", "ccv", "cfv", "ctc", "ccct", "ccvt", "cfvt", "wpwd", "wssid", "wifi"};

/// @brief Initialise with ProcessSerial(Serial)

class ProcessSerial
{
private:
    HardwareSerial &_ser;
    size_t serMessage_pos;
    bool receiving;

public:
    explicit ProcessSerial(HardwareSerial &ser);
    char *readSer();
    char serMessage[SER_BUFFER_SIZE];
    bool newInput;
    bool toggleBool(char *serMessage, const char *command, bool &variable);
    void allToFalse(char *serMessage, const char *command, const std::vector<bool *> &boolVariables);
    bool isCommand(char *serMessage, const char *command);

public:
    template <typename T>
    bool serSetVal(char *message, const char *command, T &variable)
    {
        char option[6];
        bool cmdTrue = false;

        if (std::sscanf(message, "set -%6s", option) == 1 && std::strcmp(option, command) == 0)
        {
            for (int i = 0; i < cmdArSize; i++)
            {
                if (std::strcmp(option, cmdArray[i]) == 0)
                {
                    cmdTrue = true;
                    break;
                }
            }

            if (cmdTrue)
            {
                T value;
                if constexpr (std::is_same<T, int>::value)
                {
                    if (std::sscanf(message, "set -%*s %d", &value) == 1)
                    {
                        variable = static_cast<T>(value);
                        return true;
                    }
                }

                 if constexpr (std::is_same<T, uint16_t>::value)
                {
                    if (std::sscanf(message, "set -%*s %hu", &value) == 1)
                    {
                        variable = static_cast<T>(value);
                        return true;
                    }
                }

                if constexpr (std::is_same<T, float>::value)
                {
                    if (std::sscanf(message, "set -%*s %f", &value) == 1)
                    {
                        variable = static_cast<T>(value);
                        return true;
                    }
                }

                if constexpr (std::is_same<T, bool>::value)
                {
                    char b[8];
                    if (std::sscanf(message, "set -%*s %[TtRrUuEe1]", b) == 1)
                    {
                        variable = true;
                        return true;
                    }
                    else
                    {
                        variable = false;
                        return true;
                    }
                }

                else if constexpr (std::is_same<T, std::string>::value)
                {
                    char stringValue[256];
                    if (std::sscanf(message, "set -%*s %s", stringValue) == 1)
                    {
                        variable = stringValue;
                    }
                    return true;
                }

                else
                {
                    Serial.println("Variable not matching any defined type");
                }
            }
            else
                Serial.printf("Invalid command %s\n", option);
        }
        return false;
    }
};

// END serproc.h