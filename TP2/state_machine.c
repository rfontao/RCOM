#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "state_machine.h"
#include "message_macros.h"

STATE receiver_state = START;
STATE sender_state = START;
STATE_INFO info_state = START;


STATE set_machine(char input){
    STATE st = receiver_state;
    // printf("STATE: %d\n", st);

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_SENDER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_SET)
                st = C_RCV;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_SENDER ^ C_SET))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    receiver_state = st;

    return st;
}

STATE ua_sender_machine(char input){
    STATE st = sender_state;
    // printf("STATE: %d\n", st);

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_RECEIVER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_UA)
                st = C_RCV;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_RECEIVER ^ C_UA))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    sender_state = st;

    return st;
}

STATE ua_receiver_machine(char input){
    STATE st = receiver_state;
    // printf("STATE: %d\n", st);

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_SENDER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_UA)
                st = C_RCV;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_SENDER ^ C_UA))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    receiver_state = st;

    return st;
}

STATE disc_sender_machine(char input){
    STATE st = sender_state;
    // printf("STATE: %d\n", st);

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_RECEIVER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_DISC)
                st = C_RCV;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_RECEIVER ^ C_DISC))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    sender_state = st;

    return st;
}

STATE disc_receiver_machine(char input){
    STATE st = receiver_state;
    // printf("STATE: %d\n", st);

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_SENDER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_DISC)
                st = C_RCV;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_SENDER ^ C_DISC))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    receiver_state = st;

    return st;
}

STATE_INFO info_machine(char input){

    STATE_INFO st = info_state;
    // printf("STATE: %d\n", st);

    static char c = C_INFO1;

    if(st == STOP_ST_I){
        st = START_I;
    }

    switch (st){
        case START_I:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_SENDER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV_I:
            if(input == C_INFO1 || input == C_INFO2) {
                st = C_RCV;
                c = input;
            }
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV_I:
            if(input == (A_SENDER ^ c))
                st = INFO;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;
        
        case INFO:
            if(input == FLAG)
                st = STOP_ST;
            break;

        case STOP_ST_I:
            break;

        default:
            break;
    }

    info_state = st;

    return st;
}

STATE info_response_machine(char input) {
    
    STATE st = sender_state;
    // printf("STATE: %d\n", st);

    static char c;

    if(st == STOP_ST){
        st = START;
    }

    switch (st){
        case START:
            if(input == FLAG)
                st = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(input == A_RECEIVER)
                st = A_RCV;
            else if(input != FLAG)
                st = START;
            break;

        case A_RCV:
            if(input == C_RR_1 || input == C_RR_2 || input == C_REJ_1 || input == C_REJ_2) {
                c = input;
                st = C_RCV;
            }
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case C_RCV:
            if(input == (A_SENDER ^ c))
                st = BCC_OK;
            else if(input == FLAG)
                st = FLAG_RCV;
            else
                st = START;
            break;

        case BCC_OK:
            if(input == FLAG)
                st = STOP_ST;
            else
                st = START;
            break;

        case STOP_ST:
            break;

        default:
            break;
    }

    sender_state = st;

    return st;
}