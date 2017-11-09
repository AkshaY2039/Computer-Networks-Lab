/*
TFTP.h
header for opcodes, functions, packet structure of TFTP messages
*/
#ifndef TFTP
#define TFTP

#include <stdint.h>

#define ERROR_LENGTH_SET 512
#define DATA_LENGTH_SET 512
#define FN_MODE_LENGTH_SET 515

/*TFTP MESSAGE CATEGORIES AND OPERATION CODES*/
enum OP_CODE
{
	/*enumeration will sequentially assign a value to these codes
	finally ACK gets 4 and so on*/
	READ_REQUEST=1, WRITE_REQUEST, DATA, ACK, ERROR
};

/*TFTP MODES OF TRANSFER*/
enum TR_MODE
{
	/*MAIL_MODE is obsolete now so not included*/
	NETASCII_MODE=1,OCTET_MODE
};

/*TFTP MESSAGES STRUCTURE*/
typedef union /*union makes all the message types here share the same memory space*/
{
	/*in case of only OP_CODE is there in message*/
	uint16_t OP_CODE; /*for OP_CODE to be represented as 16bit i.e. 2bytes as per the protocol*/

	/*in case the Message is a RRQ or WRQ
	Message Format: | OP_CODE[2B] | Filename[Variable] | [1B]All 0s | TR_MODE[Variable] | [1B]All 0s | */
	struct RequestMessage
	{
		uint16_t OP_CODE;
		uint8_t FileName_and_Mode[FN_MODE_LENGTH_SET]; /*can be variable as per the protocol 512 + 1 + 1 + 1*/
	}Request;

	/*in case the Message is DATA
	Message Format: | OP_CODE[2B] | BLOCK_NO[2B] | data[0-512B] | */
	struct DataMessage
	{
		uint16_t OP_CODE,BLOCK_NO;
		uint8_t data[DATA_LENGTH_SET];
	}Data;

	/*in case the message is Acknowledgement
	Message Format: | OP_CODE[2B] | BLOCK_NO[2B] | */
	struct Acknowledgement
	{
		uint16_t OP_CODE,BLOCK_NO;
	}Ack;

	/*in case the message is an Error message
	Message Format: | OP_CODE[2B] | ERROR_CODE[2B] | ERROR_STRING[Variable] | [1B]All 0s | */
	struct ErrorMessage
	{
		uint16_t OP_CODE,ERROR_CODE;
		uint8_t ERROR_STRING[ERROR_LENGTH_SET];
	}Error;
}TFTP_MSG;

/*	ssize_t for portability as a block read by the machine */
/* Function prototype for Data Sending */
ssize_t Send_TFTP_DATA(int server_fd, uint16_t block_number, uint8_t *data, ssize_t data_length, struct sockaddr_in *client_sock, socklen_t slen);

/* Function prototype for ACK Sending */
ssize_t Send_TFTP_ACK(int server_fd, uint16_t block_number, struct sockaddr_in *client_sock, socklen_t slen);

/* Function prototype for ERROR Sending */
ssize_t Send_TFTP_ERROR(int server_fd, int error_code, char *error_string, struct sockaddr_in *client_sock, socklen_t slen);

/* Function prototype for MSG Receiving */
ssize_t Recv_TFTP_MSG(int server_fd, TFTP_MSG *msg, struct sockaddr_in *client_sock, socklen_t *slen);

/*function prototype for processing Get Request*/
void ProcessGet(int s_fd, struct sockaddr_in *client_sock, socklen_t slen, char *filename);

/*function prototype for processing Put Request*/
void ProcessPut(int s_fd, struct sockaddr_in *client_sock, socklen_t slen, char *filename);

/* Function prototype for Parsing Client Requests */
void Parse_Client_Req(int s_fd, TFTP_MSG *msg, ssize_t msg_len, struct sockaddr_in *client_sock, socklen_t slen, char *base_directory);

/* Function prototype for Handling TFTP Requests */
void Handle_TFTP_Request(TFTP_MSG *msg, ssize_t msg_len, struct sockaddr_in *client_sock, socklen_t slen, char *base_directory);

#endif
