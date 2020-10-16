#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "state_machine.h"
#include "message_macros.h"

STATE receiver_state = START;
STATE sender_state = START;

STATE machine(unsigned char input, machine_type type){

    STATE st;

    if(type == RECEIVER)
        st = receiver_state;
    else 
        st = sender_state;
    // printf("STATE: %d\n", st);

    static unsigned char c;

    if(st == STOP_ST || st == STOP_INFO){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;

        case FLAG_RCV:
            if((type == RECEIVER && input == A_SENDER) ||
                (type == SENDER && input == A_RECEIVER))
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == FLAG)
                st = FLAG_RCV;
            else {
                st = C_RCV;
                c = input;
            }
            break;

        case C_RCV:
            if(input == FLAG)
                st = FLAG_RCV;
            else if((type == RECEIVER && input == (A_SENDER ^ c)) ||
                (type == SENDER && input == (A_RECEIVER ^ c)))
                st = BCC_OK;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else {
                st = INFO;
            }
            break;
        
        case INFO:
            if(input == FLAG)
                st = STOP_INFO;
            break;

        case STOP_ST:
            break;

        case STOP_INFO:
            break;

        default:
            break;
    }

    if(type == RECEIVER)
        receiver_state = st;
    else 
        sender_state = st;

    return st;
}

void reset_state(machine_type type){
    if(type == RECEIVER)
        receiver_state = START;
    else 
        sender_state = START;
}