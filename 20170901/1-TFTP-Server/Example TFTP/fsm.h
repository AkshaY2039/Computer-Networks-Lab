/* fsm.h 
   Copyright (c) 2013 James Northway
*/

#ifndef TFTP_FSM
#define TFTP_FSM

/*Finite state machine*/
#define STATE_STANDBY       1
#define STATE_RECEIVE       2
#define STATE_WAIT          3
#define STATE_SEND          4
#define STATE_RESET         5
#define STATE_FINISHED      9
#define OPERATION_DONE      0
#define OPERATION_FAILED    1
#define OPERATION_ABANDONED 2

void server_fsm(int *state, int *operation);
void client_fsm(int *state, int *operation);

#endif