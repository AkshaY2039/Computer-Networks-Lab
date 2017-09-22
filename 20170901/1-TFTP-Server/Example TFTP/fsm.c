/* fsm.c 
   Copyright (c) 2013 James Northway
*/

#include "fsm.h"

void server_fsm(int *state, int *operation){

    switch (*state){
        case STATE_STANDBY:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_RECEIVE;
                break;
            }
            
        break;
        case STATE_RECEIVE:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_SEND;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_RESET;
                break;
            }
            
        break;
        case STATE_WAIT:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_RECEIVE;
                break;
                case OPERATION_FAILED:
                    *state = STATE_SEND;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_RESET;
                break;
            }
            
        break;
        case STATE_SEND:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_WAIT;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_RESET;
                break;
            }
            
        break;
        case STATE_RESET:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_STANDBY;
                break;
            }
            
        break;
    }
}

void client_fsm(int *state, int *operation){

    switch (*state){
        case STATE_SEND:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_WAIT;
                break;
                case OPERATION_FAILED:
                    *state = STATE_FINISHED;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_FINISHED;
                break;
            }
            
        break;
        case STATE_WAIT:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_RECEIVE;
                break;
                case OPERATION_FAILED:
                    *state = STATE_SEND;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_FINISHED;
                break;
            }
            
        break;
        case STATE_RECEIVE:
        
            switch(*operation){
                case OPERATION_DONE:
                    *state = STATE_SEND;
                break;
                case OPERATION_ABANDONED:
                    *state = STATE_FINISHED;
                break;
            }
            
        break;
    }
}
