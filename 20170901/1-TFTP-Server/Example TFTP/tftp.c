/* tftp.c 
   Copyright (c) 2013 James Northway
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include "tftp.h"
#include "fsm.h"
#include "file.h"

packetbuffer_t packet_in_buffer[PACKETSIZE];
packetbuffer_t *packet_in = packet_in_buffer;
int packet_in_length;
packetbuffer_t *packet_out;
int packet_out_length;
struct tftp_packet packet;
struct tftp_transaction transaction;

void packet_extract_opcode(packet_t *packet, const packetbuffer_t *pbuf){ 
    memmove(&packet->opcode,pbuf,sizeof(opcode_t));
    packet->opcode = ntohs(packet->opcode);
}

void packet_parse_rq(packet_t *packet, const packetbuffer_t *pbuf){  
    strcpy(packet->filename, pbuf+sizeof(opcode_t));
    strcpy(packet->mode, pbuf+sizeof(opcode_t)+strlen(packet->filename)+1);
}

void packet_parse_data(packet_t *packet, const packetbuffer_t *pbuf, 
        int * data_length) {       
    memcpy(&packet->blocknum,pbuf+sizeof(opcode_t),sizeof(bnum_t));
    packet->blocknum = ntohs(packet->blocknum);
    packet->data_length = *data_length - sizeof(opcode_t) - sizeof(bnum_t);
    memcpy(packet->data, pbuf+sizeof(opcode_t)+sizeof(bnum_t), 
        packet->data_length);
}

void packet_parse_ack(packet_t *packet, const packetbuffer_t *pbuf){
    memcpy(&packet->blocknum,pbuf+sizeof(opcode_t),sizeof(bnum_t));
    packet->blocknum = ntohs(packet->blocknum);
}

void packet_parse_error(packet_t *packet, const packetbuffer_t *pbuf, 
        int * data_length){       
    memcpy(&packet->ecode,pbuf+sizeof(opcode_t),sizeof(ecode_t));
    packet->ecode = ntohs(packet->ecode);
    packet->estring_length = 
        *data_length - sizeof(opcode_t) - sizeof(ecode_t);
    memcpy(packet->estring, pbuf+sizeof(opcode_t)+sizeof(ecode_t),
        packet->estring_length);
}

packetbuffer_t *append_to_packet(const void *data, const int data_size){
    packet_out = realloc(packet_out, 
        sizeof(packetbuffer_t)*packet_out_length + data_size);
    memmove(packet_out + packet_out_length, data, data_size);
    packet_out_length += data_size;
    return packet_out;
}

void packet_form_rrq(char * filename){
    opcode_t opcode_out = htons(OPCODE_RRQ);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(filename, strlen(filename)+1);
    append_to_packet(transaction.mode, strlen(transaction.mode)+1);
}

void packet_form_wrq(char * outfilename){
    opcode_t opcode_out = htons(OPCODE_WRQ);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(outfilename, strlen(outfilename)+1);
    append_to_packet(transaction.mode, strlen(transaction.mode)+1);
}

void packet_form_data(){ 
    opcode_t opcode_out = htons(OPCODE_DATA);
    bnum_t new_blocknum = htons(transaction.blocknum);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(&new_blocknum, sizeof(bnum_t));
    append_to_packet(&transaction.filebuffer, transaction.filebuffer_length);
    printf("\rTransferred block: %i", transaction.blocknum);
}

void packet_form_error(){
    opcode_t opcode_out = htons(OPCODE_ERROR);
    ecode_t new_ecode = htons(transaction.ecode);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(&new_ecode, sizeof(ecode_t));
    append_to_packet(transaction.estring, strlen(transaction.estring)+1); 
}

void packet_form_ack(){
    opcode_t opcode_out = htons(OPCODE_ACK);
    printf("\rReceived block: %i", transaction.blocknum);
    bnum_t new_blocknum = htons(transaction.blocknum);
    append_to_packet(&opcode_out,sizeof(opcode_t));
    append_to_packet(&new_blocknum, sizeof(bnum_t));
}

int packet_parse(packet_t *packet, const packetbuffer_t *pbuf, 
        int *packet_in_length){   
    packet_extract_opcode(packet,pbuf);
    
    if (IS_RRQ(packet->opcode) || IS_WRQ(packet->opcode)){
        packet_parse_rq(packet, pbuf);
    }else if (IS_ACK(packet->opcode)){
        packet_parse_ack(packet, pbuf);
    }else if (IS_DATA(packet->opcode)){
        packet_parse_data(packet, pbuf, packet_in_length);
    }else if (IS_ERROR(packet->opcode)){
        packet_parse_error(packet, pbuf, packet_in_length);
    }else{
        return -1;
    }
    
    return 0;
}

void packet_free(){
    if (packet_out_length > 0){
        free(packet_out);
        packet_out_length = 0;
        packet_out = NULL;
    }
}

int packet_receive_rrq(){
    printf("Read request for %s\n",packet.filename);
    packet_free();
        
    if (transaction.file_open == 1){
        file_close(&transaction.filedata);
    }
    
    if ((file_open_read(packet.filename,&transaction.filedata))== -1){
        strcpy(transaction.estring, ESTRING_1);
        transaction.ecode = ECODE_1;
        packet_form_error();       
    }else{
        transaction.file_open = 1;
        transaction.blocknum = 1;
        transaction.filepos = ((transaction.blocknum * BLIMIT) - BLIMIT);
        transaction.filebuffer_length = file_buffer_from_pos(&transaction);
        packet_form_data();
    }
    
    return OPERATION_DONE;
}

int packet_receive_wrq(){
    printf("Write request for %s\n",packet.filename);
    packet_free();
    
    if (transaction.file_open == 0){ 
    
        if ((file_open_write(packet.filename,&transaction.filedata)) == 0){
            transaction.file_open = 1;
            
        }else{
            transaction.file_open = 0;
        }
    }
    
    if (transaction.file_open == 0){
        strcpy(transaction.estring, ESTRING_1);
        transaction.ecode = ECODE_1;
        packet_form_error();       
    }else{
        packet_form_ack();
        transaction.blocknum++;
    }
    
    return OPERATION_DONE;
}

int packet_receive_data(){

    if (packet.blocknum == transaction.blocknum){
    
        if (file_append_from_buffer(&packet, &transaction) == -1){
            strcpy(transaction.estring, ESTRING_2);
            transaction.ecode = ECODE_2;
            packet_free();
            packet_form_error();           
        }else{
            packet_free();
            packet_form_ack();
            transaction.blocknum++;
        }
    }
    
    if (packet.data_length < 512) {
        file_close(&transaction.filedata);
        transaction.file_open = 0;
        transaction.complete = 1;
    }
    
    return OPERATION_DONE;
}

int packet_receive_ack(){

    if (packet.blocknum == transaction.blocknum){
        transaction.blocknum++;
        transaction.timeout_count = 0;
        packet_free();
        
        if (transaction.file_open == 0 && (file_open_read(packet.filename,
                &transaction.filedata)) == -1){
            strcpy(transaction.estring, ESTRING_2);
            transaction.ecode = ECODE_2;
            packet_form_error();            
        }else{
            transaction.filepos = ((transaction.blocknum * BLIMIT) - BLIMIT);
            transaction.filebuffer_length = file_buffer_from_pos(&transaction);
            if (!transaction.filebuffer_length) {
                transaction.complete = 1;
                return OPERATION_ABANDONED;               
            } else {
                packet_form_data();
            }
        }
    }
    
    return OPERATION_DONE;
}

int packet_receive_error(){
    fprintf(stderr, "Received error %i: %s\n",packet.ecode, packet.estring);
    return OPERATION_ABANDONED;
}

int packet_receive_invalid(){  
    transaction.ecode = ECODE_4;
    strcpy(transaction.estring, ESTRING_4);
    packet_free();
    packet_form_error();
    return OPERATION_DONE;
}