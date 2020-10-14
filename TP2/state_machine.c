#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include "state_machine.h"
#include "message_macros.h"

STATE state[2] = {START, START};

bool machine(char input, frameType type) {
    STATE st;
    if(type == SET)
        st = state[RECEPTOR];
    else if(type == UA_SENDER)
        st = state[EMITTER]; 
        
    switch (st)
    {
    case START:
        if(input == FLAG) 
            st = FLAG_RCV;
        break;

    case FLAG_RCV:
        if(type == SET) {
            if(input == A_SENDER) {
                st = A_RCV;
                break;
            }
        }
        else if(type == UA_SENDER) {
            if(input == A_RECEIVER) {
                st = A_RCV;
                break;
            }
        }
        if(input != FLAG) 
            st = START;
        break;

    case A_RCV:
        if(type == SET) {

            if(input == C_SET) {
                st = C_RCV;
                break;
            }

        } else if(type == UA_SENDER) {

            if(input == C_UA) {
                st = C_RCV;
                break;
            }
        }

        if(input == FLAG)
            st = FLAG_RCV;
        else 
            st = START;
        break;

    case C_RCV:
        if(type == SET) {
            if(input == A_SENDER ^ C_SET) {
                st = BCC_OK;
                break;
            }
        }
        else if(type == UA_SENDER) {
            if(input == A_RECEIVER ^ C_UA) {
                st = BCC_OK;
                break;
            }
        }
        if(input == FLAG)
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

    if(type == SET)
        state[RECEPTOR] = st;
    else if(type == UA_SENDER)
        state[EMITTER] = st; 

    if(st == STOP_ST)
        return true;
    return false;

}