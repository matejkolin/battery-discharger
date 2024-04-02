// serpro.cpp
#include "serproc.h"

ProcessSerial::ProcessSerial(HardwareSerial &ser) : _ser(ser), serMessage_pos(0), newInput(false)
{
}

char *ProcessSerial::readSer()
{
    receiving = false;

    while (_ser.available() > 0 && newInput == false)
    {
        char inByte = _ser.read();

        if (!receiving)
        {
            if (inByte != '\n')
            {
                serMessage[serMessage_pos] = inByte;

                serMessage_pos++;

                if (serMessage_pos >= SER_BUFFER_SIZE)
                {
                    serMessage_pos = SER_BUFFER_SIZE - 1;
                }
            }

            else
            {
                serMessage[serMessage_pos] = '\0';
                receiving = false;
                serMessage_pos = 0;

                newInput = true;
            }
        }

        else if (inByte == '\n')
        {
            receiving = true;
        }
    }

    return serMessage;
}

bool ProcessSerial::toggleBool(char *serMessage, const char *command, bool &variable)
{
    if (strcmp(serMessage, command) == 0)
    {
        variable = !variable;
        return true;
    }

    return false;
}

bool ProcessSerial::isCommand(char *serMessage, const char *command)
{
    if (strcmp(serMessage, command) == 0)
    {
        return true;
    }

    return false;
}

void ProcessSerial::allToFalse(char *serMessage, const char *command, const std::vector<bool *> &boolVariables)
{
    if (strcmp(serMessage, command) == 0)
    {
        for (bool *var : boolVariables)
        {
            *var = false;
        }
    }
}

// END serpro.cpp