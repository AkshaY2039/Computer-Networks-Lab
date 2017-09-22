/* tftp.h 
   Copyright (c) 2013 James Northway
*/

#ifndef TFTP
#define TFTP

#define BLIMIT      512 /*Maximum amount of file contents that can be buffered*/
#define PACKETSIZE  516 /*Total size of TFTP packet*/

typedef unsigned short opcode_t; /*TFTP Operation Code*/
#define OPCODE_RRQ   1
#define IS_RRQ(op)   ((op) == OPCODE_RRQ) /*Evaluates as true or false*/
#define OPCODE_WRQ   2
#define IS_WRQ(op)   ((op) == OPCODE_WRQ)
#define OPCODE_DATA  3
#define IS_DATA(op)  ((op) == OPCODE_DATA)
#define OPCODE_ACK   4
#define IS_ACK(op)   ((op) == OPCODE_ACK)
#define OPCODE_ERROR 5
#define IS_ERROR(op) ((op) == OPCODE_ERROR)

typedef unsigned short ecode_t; /*TFTP Error code*/
#define ECODE_NONE  8
#define ECODE_0     0
#define IS_ECODE_0(ec)  ((ec) == ECODE_0)
#define ESTRING_0   "Not defined, see error message(if any)."
#define ECODE_1     1
#define IS_ECODE_1(ec)  ((ec) == ECODE_1)
#define ESTRING_1   "File not found."
#define ECODE_2     2
#define IS_ECODE_2(ec)  ((ec) == ECODE_2)
#define ESTRING_2   "Access violation."
#define ECODE_3     3
#define IS_ECODE_3(ec)  ((ec) == ECODE_3)
#define ESTRING_3   "Disk full or allocation exceeded."
#define ECODE_4     4
#define IS_ECODE_4(ec)  ((ec) == ECODE_4)
#define ESTRING_4   "Illegal TFTP operation."
#define ECODE_5     5
#define IS_ECODE_5(ec)  ((ec) == ECODE_5)
#define ESTRING_5   "Unknown transfer ID."
#define ECODE_6     6
#define IS_ECODE_6(ec)  ((ec) == ECODE_6)
#define ECODE_7     7
#define IS_ECODE_7(ec)  ((ec) == ECODE_7)
#define ESTRING_7   "No such user."

/*Transfer operation modes*/
#define MODE_NETASCII "netascii" 
#define MODE_OCTET    "octet"
#define MODE_MAIL     "mail"

#define TIMEOUT         1 /*Socket receive timeout*/
#define TIMEOUT_LIMIT   10

typedef unsigned short bnum_t; /*TFTP Block number, for data packets*/
typedef char packetbuffer_t; /*Packet buffer*/

/*Struct for storage of the data extracted from a packet buffer*/
typedef struct tftp_packet{
    char filename[PACKETSIZE];
    opcode_t opcode;
    char mode[PACKETSIZE];
    char data[BLIMIT];
    int data_length;
    bnum_t blocknum;
    ecode_t ecode;
    char estring[PACKETSIZE];
    int estring_length;
}packet_t;

/*Struct for storage of data relating to the current transfer*/
typedef struct tftp_transaction{
    unsigned last_ack;
    int timeout_count;
    int timed_out;
    int bad_packet_count;
    int final_packet;
    int complete;
    int rebound_socket;
    int tid;
    int file_open;
    int filepos;
    int filedata;
    char filebuffer[BLIMIT];
    int filebuffer_length;
    char mode[PACKETSIZE];
    bnum_t blocknum;
    ecode_t ecode;
    char estring[64];
    int estring_length;
}transaction_t;

/* Packet Parsing */
int packet_parse(packet_t *packet, const packetbuffer_t *pbuf, 
    int *packet_in_length);
void packet_extract_opcode(packet_t *packet,const packetbuffer_t *pbuf);
void packet_parse_rq(packet_t *packet, const packetbuffer_t *pbuf);
void packet_parse_data(packet_t *packet, const packetbuffer_t *pbuf, 
    int *packet_in_length);
void packet_parse_ack(packet_t *packet, const packetbuffer_t *pbuf);
void packet_parse_error(packet_t *packet, const packetbuffer_t *pbuf, 
    int *packet_in_length);

/* Packet Formation */
packetbuffer_t *append_to_packet(const void *data, const int data_size);
void packet_form_rrq();
void packet_form_wrq();
void packet_form_data();
void packet_form_error();
void packet_form_ack();
void packet_free();

/* Packet Receipt */
int packet_receive_rrq();
int packet_receive_wrq();
int packet_receive_data();
int packet_receive_ack();
int packet_receive_error();
int packet_receive_invalid();

#endif
