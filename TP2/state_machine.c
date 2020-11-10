#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "state_machine.h"
#include "message_macros.h"

STATE receiver_state = START;
STATE sender_state = START;

STATE machine(unsigned char input, machine_type type, frame_type frame_type){

    STATE st;

    if(type == RECEIVER_M)
        st = receiver_state;
    else 
        st = sender_state;

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
            if(frame_type == COMMAND){
                if((type == RECEIVER_M && input == A_SENDER) ||
                    (type == SENDER_M && input == A_RECEIVER))
                    st = A_RCV;
                else if(input != FLAG)
                    st = START;
            } else {
                if((type == RECEIVER_M && input == A_RECEIVER) ||
                    (type == SENDER_M && input == A_SENDER))
                    st = A_RCV;
                else if(input != FLAG)
                    st = START;
            }
            break;

        case A_RCV:
            if(type == RECEIVER_M && frame_type == COMMAND && ((unsigned char)input == C_RR_1 || (unsigned char)input == C_RR_2 || 
                (unsigned char)input == C_REJ_1 || (unsigned char)input == C_REJ_2)) {
                st = START;
            } else if(input == FLAG)
                st = FLAG_RCV;
            else {
                st = C_RCV;
                c = input;
            }
            break;

        case C_RCV:
            if(input == FLAG)
                st = FLAG_RCV;
            else if(frame_type == COMMAND){
                if((type == RECEIVER_M && input == (A_SENDER ^ c)) ||
                (type == SENDER_M && input == (A_RECEIVER ^ c)))
                    st = BCC_OK;
                else
                    st = START;    
            
            } else if(frame_type == RESPONSE){
                if((type == RECEIVER_M && input == (A_RECEIVER ^ c)) ||
                (type == SENDER_M && input == (A_SENDER ^ c)))
                    st = BCC_OK;
                else 
                    st = START;
            }
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

    if(type == RECEIVER_M)
        receiver_state = st;
    else 
        sender_state = st;

    return st;
}

void reset_state(machine_type type){
    if(type == RECEIVER_M)
        receiver_state = START;
    else 
        sender_state = START;
}