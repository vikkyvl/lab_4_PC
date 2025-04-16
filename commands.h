#ifndef COMMANDS_H
#define COMMANDS_H

#include <cstdint>

enum Type: uint8_t
{
    COMMAND = 1,
    MATRIX,
    THREADS
};

enum ServerState
{
    INITIAL = 4,
    CONFIGURED,
    CALCULATING
};

enum ServerRespond
{
    CONFIG_OK = 7,
    CALCULATION_STARTED,
    CURRENT_PROGRESS,
    COMPLETED,
    FAILED,
    BUSY,
    ACTIVE
};

enum ClientRequest
{
    CONFIG = 14,
    CALCULATE,
    GET_RESULT,
    IS_BUSY
};

#endif //COMMANDS_H
