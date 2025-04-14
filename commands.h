#ifndef COMMANDS_H
#define COMMANDS_H

enum ServerState
{
    INITIAL,
    CONFIGURED,
    CALCULATING
};

enum ServerRespond
{
    CONFIG_OK,
    CALCULATION_STARTED,
    CURRENT_PROGRESS,
    COMPLETED,
    FAILED
};

enum ClientRequest
{
    CONFIG = 1,
    CALCULATE,
    GET_RESULT
};

#endif //COMMANDS_H
