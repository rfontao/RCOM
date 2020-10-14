#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include <stdbool.h>

#define RECEPTOR 1
#define EMITTER 0

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP_ST
} STATE;

bool machine(char input, frameType type);

#endif