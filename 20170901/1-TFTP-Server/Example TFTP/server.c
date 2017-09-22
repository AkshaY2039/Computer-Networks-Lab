/* server.c 
   Copyright (c) 2013 James Northway
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "file.h"
#include "fsm.h"
#include "netudp.h"
#include "tftp.h"

/* From netudp.c */
extern struct sockaddr_in xfSrv, xfCli;
extern struct sockaddr_in ftCli, ftSrv; 

/* Sockets, Ports, Addresses */
int socket1;
int addrlen;
char *port;
int socket2;
int port2;

/* From tftp.c*/
extern packetbuffer_t packet_in_buffer[PACKETSIZE];
extern packetbuffer_t *packet_in;
extern int packet_in_length;
extern struct tftp_packet packet;
extern struct tftp_transaction transaction;
extern packetbuffer_t * packet_out;
extern int packet_out_length;

/* State Functions */
void state_standby(int *operation);
void state_receive(int *operation);
void state_send(int *operation);
void state_wait(int *operation);
void state_reset(int *operation);

/* State Helper Functions */
static void wait_alarm(int);

/* SERVER PROGRAM */
int main(int argc, char *argv[]){

    if (argc < 1) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    int state = STATE_STANDBY;
    int operation; 
    if(argv[1]==NULL)
    {   
        port="741";
        printf("port set to 741\n");
    }
    else
    port = argv[1];
    netudp_bind_server(&socket1, port);
    
    while (state != STATE_FINISHED){
    
        switch(state) {
            case STATE_STANDBY:
                state_standby(&operation);
            break;
            case STATE_RECEIVE:
                state_receive(&operation);
            break;
            case STATE_WAIT:
                state_wait(&operation);
            break;
            case STATE_SEND:
                state_send(&operation);
            break;
            case STATE_RESET:
                state_reset(&operation);
            break;
        }
        
        server_fsm(&state, &operation);
    }
    
    return 0;
}

/* STATE FUNCTIONS */
void state_standby(int *operation){
    int leave_standby = 0;
    int rbytes = 0;   
    transaction.timeout_count = 0;
    transaction.blocknum = 0;
    transaction.final_packet = 0;
    transaction.file_open = 0;
    transaction.complete = 0;
    transaction.ecode = ECODE_NONE;
    packet_in_length = 0;
    transaction.filepos = 0;
    printf("\n\n***TFTP Server***\n");
    
    while(leave_standby == 0){
        addrlen = sizeof(ftCli);
        rbytes = recvfrom(socket1, packet_in, PACKETSIZE, 0, 
                         (struct sockaddr *) &ftCli, 
                         (socklen_t *) &addrlen); 
        packet_in_length = rbytes;                            
        
        if (rbytes == -1){
            fprintf(stderr, "Could not read command from socket!:%s\n",
                strerror(errno));
            continue;
        }
        
        netudp_rebind_server(&socket2, &port2);
        leave_standby = 1;
    }
    
    *operation = OPERATION_DONE;
}

void state_receive(int *operation){
    packet_parse(&packet, packet_in, &packet_in_length);
    
    if (IS_RRQ(packet.opcode)){
        *operation = packet_receive_rrq();       
    }else if (IS_WRQ(packet.opcode)){
        *operation = packet_receive_wrq();      
    }else if (IS_ACK(packet.opcode)){
        *operation = packet_receive_ack();       
    }else if (IS_DATA(packet.opcode)){
        *operation = packet_receive_data();       
    }else if (IS_ERROR(packet.opcode)){
        *operation = packet_receive_error();       
    }else{    
        *operation = packet_receive_invalid();
    }
}

void state_send(int *operation){
    netudp_send_packet(&socket2, (struct sockaddr *) &ftCli, &packet_out, 
        &packet_out_length);
    
    if (transaction.complete == 1 || transaction.ecode != ECODE_NONE){
        printf("\nDone.\n");
        *operation = OPERATION_ABANDONED;       
    }else{
        *operation = OPERATION_DONE;
    }
}

void state_wait(int *operation){
    int rbytes = 0;
    transaction.timed_out = 0;
    struct sigaction sa;
    memset (&sa, '\0', sizeof(sa));
    sa.sa_handler = (void(*)(int)) wait_alarm;
    sigemptyset(&sa.sa_mask);
    int res = sigaction(SIGALRM, &sa, NULL);
    
    if (res == -1) {
        fprintf(stderr, "Unable to set SA_RESTART to false: %s\n",
                        strerror(errno));
        abort();
    }

    alarm(TIMEOUT);

    if((rbytes = recvfrom(socket2, packet_in, PACKETSIZE, 0,
            (struct sockaddr *)&ftCli, (socklen_t *) &addrlen)) < 0){  
            
        if ((errno == EINTR) && transaction.timed_out) {
            transaction.timeout_count++;           
        }else{
            fprintf(stderr, "%s\n", strerror(errno));
        }
        
        if (transaction.timeout_count == TIMEOUT_LIMIT){
            fprintf(stderr, "Timeout.\n");
            *operation = OPERATION_ABANDONED;           
        }else{
            *operation = OPERATION_FAILED;
        }
        
    }else{
        alarm(0);
        transaction.timeout_count = 0;
        packet_in_length = rbytes;
        *operation = OPERATION_DONE;
    } 
}

void state_reset(int *operation){

    if (transaction.file_open == 1){
        file_close(&transaction.filedata);
    }
    
    packet_free();
    close(socket2);
    *operation = OPERATION_DONE;
}

static void wait_alarm(int signo){
    transaction.timed_out = 1;
    signo++;
    return;
}
