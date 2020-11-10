#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

typedef enum {
    START       = 0,
    FLAG_RCV    = 1,
    A_RCV       = 2,
    C_RCV       = 3,
    BCC_OK      = 4,
    STOP_ST     = 5,
    STOP_INFO   = 6,
    INFO        = 7      
} STATE;

typedef enum {
    RECEIVER_M,
    SENDER_M
} machine_type;

typedef enum {
    COMMAND,
    RESPONSE
} frame_type;

STATE machine(unsigned char input, machine_type type, frame_type frame_type);

void reset_state(machine_type type);

#endif