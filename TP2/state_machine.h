#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

typedef enum {
    START       = 0,
    FLAG_RCV    = 1,
    A_RCV       = 2,
    C_RCV       = 3,
    BCC_OK      = 4,      
    STOP_ST     = 5
} STATE;

STATE set_machine(char input);

STATE ua_sender_machine(char input);

STATE ua_receiver_machine(char input);

STATE disc_sender_machine(char input);

STATE disc_receiver_machine(char input);

#endif