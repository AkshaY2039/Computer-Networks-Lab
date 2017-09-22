/*
TFTP.c
function definitions
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include "TFTP.h"

#define RECV_TIMEOUT 10
#define RECV_RETRIES 5

/*function definition for Send_TFTP_DATA message*/
ssize_t Send_TFTP_DATA(int server_fd, uint16_t block_number, uint8_t *data, ssize_t data_length, struct sockaddr_in *client_sock, socklen_t slen)
{
	/*Forming a DATA Message*/
	TFTP_MSG msg;
	msg.Data.OP_CODE = htons(DATA);
	msg.Data.BLOCK_NO = htons(block_number);
	memcpy(msg.Data.data, data, data_length);

	/*Sending the DATA message created to client*/
	ssize_t c;
	if((c = sendto(server_fd, &msg, 4 + data_length, 0, (struct sockaddr *) client_sock, slen)) < 0)
		perror("Server got error in sending DATA : sendto()");
	return c;
}

/*function definition for Send_TFTP_ACK message*/
ssize_t Send_TFTP_ACK(int server_fd, uint16_t block_number, struct sockaddr_in *client_sock, socklen_t slen)
{
	/*Forming a ACK Message*/
	TFTP_MSG msg;
	msg.Ack.OP_CODE = htons(ACK);
	msg.Ack.BLOCK_NO = htons(block_number);

	/*Sending the ACK message created to client*/
	ssize_t c;
	if((c = sendto(server_fd, &msg, sizeof(msg.Ack), 0, (struct sockaddr *) client_sock, slen)) < 0)
		perror("Server got error in sending ACK : sendto()");
	return c;
}

/*function definition for Send_TFTP_ERROR message*/
ssize_t Send_TFTP_ERROR(int server_fd, int error_code, char *error_string, struct sockaddr_in *client_sock, socklen_t slen)
{
	/*checking the error with length of Error String*/
	if(strlen(error_string) >= ERROR_LENGTH_SET)
	{
		fprintf(stderr, "Server: Send_TFTP_ERROR() : error string too long\n");
		return -1;
	}

	/*Forming a ERROR Message*/
	TFTP_MSG msg;
	msg.Error.OP_CODE = htons(ERROR);
	msg.Error.ERROR_CODE = error_code;
	strcpy(msg.Error.ERROR_STRING, error_string);

	/*Sending the ERROR message created to client*/
	ssize_t c;
	if((c = sendto(server_fd, &msg, 4 + strlen(error_string) + 1, 0, (struct sockaddr *) client_sock, slen)) < 0)
		perror("Server got error in sending ERROR : sendto()");
	return c;
}

/*function definition for Recv_TFTP_MSG*/
ssize_t Recv_TFTP_MSG(int server_fd, TFTP_MSG *msg, struct sockaddr_in *client_sock, socklen_t *slen)
{
	/*Receiving Message from the cient*/
	ssize_t msg_size; /*for storing the length of message received on success*/
	/*if there is error in reading message AND NOT an “temporarily unavailable try again later” error */
	if((msg_size = recvfrom(server_fd, msg, sizeof(*msg), 0, (struct sockaddr *) client_sock, slen)) < 0 && errno != EAGAIN)
		perror("Server got error in receiving MSG : recvfrom()");
	return msg_size;
}

/*function definition to process put request*/
void ProcessPut(int s_fd, struct sockaddr_in *client_sock, socklen_t slen, char *filename)
{
	FILE *file_desc = fopen(filename, "w");
	TFTP_MSG msg1;
	ssize_t c;
	uint16_t Block_Number = 0;
	int CountDown;
	int CLOSE_FLAG = 0;

	c = Send_TFTP_ACK(s_fd, Block_Number, client_sock, slen); /*Sending ACK that server is ready to receive data*/

	if(c < 0) /*if any error in sending ACK Message*/
	{
		printf("%s : %u : ACK before Transfer Failed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
		exit(1);
	}

	while(!CLOSE_FLAG)
	{
		for(CountDown = RECV_RETRIES; CountDown; CountDown--) /* number of times each packet should be listened for */
		{
			c = Recv_TFTP_MSG(s_fd, &msg1, client_sock, &slen); /*listening for DATA message from Client for the RECVTIMEOUT set*/

			if(c >= 0 && c < 4) /* condition for invalid DATA Message*/
			{
				 printf("%s : %u : Invalid Message Size during PUT \n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				 Send_TFTP_ERROR(s_fd, 0, "Invalid Message Size during PUT", client_sock, slen);
				 exit(1);
			}
			else
				if(c >= 4) /* acceptible message size */
					break;

			if(errno != EAGAIN) /*if there is an error but its not the TRY AGAIN then... */
			{
				printf("%s : %u : Transfer Failed/Killed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				exit(1);
			}

			/*sending ACK for current Block*/
			c = Send_TFTP_ACK(s_fd, Block_Number, client_sock, slen);

			if(c < 0)
			{
				printf("%s : %u : Transfer Failed/Killed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				exit(1);
			}
		}

		if(!CountDown) /*if Countdown becomes 0 means that block didn't reach TIMED_OUT*/
		{
			printf("%s : %u : Transfer Timed Out \n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
			exit(1);
		}

		Block_Number++; /*increment Block Number*/

		if(c < sizeof(msg1.Data)) /*if last message read is not of DATA Message size*/
			CLOSE_FLAG = 1; /*set CLOSE_FLAG to 1*/

		if(ntohs(msg1.OP_CODE) == ERROR)
		{
			printf("%s : %u : Error Message Received : %u %s\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port), ntohs(msg1.Error.ERROR_CODE), msg1.Error.ERROR_STRING);
			exit(1);
		}
		else
			if(ntohs(msg1.OP_CODE) != DATA)
			{
				printf("%s : %u : Invalid Message During Transfer\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				Send_TFTP_ERROR(s_fd, 0, "Invalid Message During Transfer", client_sock, slen);
				exit(1);
			}
			else
				if(ntohs(msg1.Ack.BLOCK_NO) != Block_Number) /*Block number is Invalid*/
				{
					printf("%s : %u : Invalid Block number received\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
					Send_TFTP_ERROR(s_fd, 0, "Invalid Block Number", client_sock, slen);
					exit(1);
				}

		c = fwrite(msg1.Data.data, 1, c - 4, file_desc);

		if(c < 0) /*if the write to file fails*/
		{
			perror("Server got error while writing file : fwrite()");
			exit(1);
		}

		c = Send_TFTP_ACK(s_fd, Block_Number, client_sock, slen);

		if(c < 0) /*if the ACK message fails*/
		{
			printf("%s : %u : transfer killed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
			exit(1);
		}
	}
}

/*function definition to process get request*/
void ProcessGet(int s_fd, struct sockaddr_in *client_sock, socklen_t slen, char *filename)
{
	FILE *file_desc = fopen(filename, "r");
	TFTP_MSG msg1;
	uint8_t data[DATA_LENGTH_SET];
	ssize_t datalen, c;
	uint16_t Block_Number = 0;
	int CountDown;
	int CLOSE_FLAG = 0;

	while(!CLOSE_FLAG)
	{
		/* reading data for 1 Block */
		datalen = fread(data, 1, sizeof(data), file_desc);
		Block_Number++; /*incrementing block to have packet check*/

		if(datalen < 512) /*if the data is for the last block*/
			CLOSE_FLAG = 1; /*set CLOSE_FLAG as 1*/ 

		for(CountDown = RECV_RETRIES; CountDown; CountDown--) /* number of times each packet should be retried sending */
		{
			c = Send_TFTP_DATA(s_fd, Block_Number, data, datalen, client_sock, slen); /*sending a block to the client*/

			if(c < 0) /*if Sending Fails*/
			{
				printf("%s : %u : Transfer Failed/Killed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				exit(1);
			}

			c = Recv_TFTP_MSG(s_fd, &msg1, client_sock, &slen); /*listening for ACK message from Client for the RECVTIMEOUT set*/

			if(c >= 0 && c < 4) /* since ACK message should be 4B*/
			{
				printf("%s : %u : Invalid Message Size during GET\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				Send_TFTP_ERROR(s_fd, 0, "Invalid Message Size during GET", client_sock, slen);
				exit(1);
			}
			else
				if(c >= 4) /*even if more than 4B message is received... go for next block*/
					break;

			if(errno != EAGAIN) /*if there is an error but its not the TRY AGAIN then... */
			{
				printf("%s : %u : Transfer Failed/Killed\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				exit(1);
			}
		}

		if(!CountDown) /*if Countdown becomes 0 means that block didn't reach TIMED_OUT*/
		{
			printf("%s : %u : Transfer Timed Out (Tried %d Times)\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port), RECV_RETRIES);
			exit(1);
		}

		if(ntohs(msg1.OP_CODE) == ERROR)
		{
			printf("%s : %u : Error Message Received : %u %s\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port), ntohs(msg1.Error.ERROR_CODE), msg1.Error.ERROR_STRING);
			exit(1);
		}
		else
			if(ntohs(msg1.OP_CODE) != ACK)
			{
				printf("%s : %u : Invalid Message During Transfer\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
				Send_TFTP_ERROR(s_fd, 0, "Invalid Message During Transfer", client_sock, slen);
				exit(1);
			}
			else
				if(ntohs(msg1.Ack.BLOCK_NO) != Block_Number) /*Block number is Invalid*/
				{
					printf("%s : %u : Invalid ACK number received\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
					Send_TFTP_ERROR(s_fd, 0, "Invalid ACK Number", client_sock, slen);
					exit(1);
				}
	}
}

/* function definition for parsing the client request */
void Parse_Client_Req(int s_fd, TFTP_MSG *msg, ssize_t msg_len, struct sockaddr_in *client_sock, socklen_t slen, char *base_directory)
{
	char *filename, *mode_s, *end;

	/* extracting filename from received msg */
	filename = msg->Request.FileName_and_Mode;
	end = &filename[msg_len - 2 - 1]; /*by the structure of Request message*/

	if(*end != '\0') /*if the set end doesn't contain '\0' means either the name is longer than expected or invalid*/
	{
		/*on server side print the client:port with error message*/
		printf("%s : %u : Invalid FileName or Mode\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
		/*send the respective error to the client*/
		Send_TFTP_ERROR(s_fd, 0, "Invalid FileName or Mode", client_sock, slen);
		exit(1);
	}

	mode_s = strchr(filename, '\0') + 1; /* set the starting pointer of Mode in filename as just after first '\0' */

	if(mode_s > end) /*if the mode_s is beyond the end specified mode is missing*/
	{
		/*on server side print the client:port with error message*/
		printf("%s : %u : Transfer Mode missing in Request\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
		/*send the respective error to the client*/
		Send_TFTP_ERROR(s_fd, 0, "Transfer Mode missing in Request", client_sock, slen);
		exit(1);
	}

	/* check if the file is outside the base Server Directory */
	if(strncmp(filename, "../", 3) == 0 || strstr(filename, "/../") != NULL || (filename[0] == '/' && strncmp(filename, base_directory, strlen(base_directory)) != 0))
	{
		/*on server side print the client:port with error message*/
		printf("%s : %u : File Outside Server's Base Directory\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
		/*send the respective error to the client*/
		Send_TFTP_ERROR(s_fd, 0, "File Outside Server's Base Directory", client_sock, slen);
		exit(1);
	}
	
	FILE *file_desc;
	uint16_t OP_CODE;
	int mode;

	OP_CODE = ntohs(msg->OP_CODE);
	file_desc = fopen(filename, OP_CODE == READ_REQUEST ? "r" : "w");

	if (file_desc == NULL) /*if error in opening file*/
	{
		perror("Server got error in opening file : fopen()");
		Send_TFTP_ERROR(s_fd, errno, strerror(errno), client_sock, slen);
		exit(1);
	}

	fclose(file_desc);

	mode = strcasecmp(mode_s, "netascii") ? NETASCII_MODE : strcasecmp(mode_s, "octet")	? OCTET_MODE : 0;

	if(mode == 0)
	{
		printf("%s : %u : Mode of Transfer isn't Valid\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
		Send_TFTP_ERROR(s_fd, 0, "Mode of Transfer isn't Valid", client_sock, slen);
		exit(1);
	}

	printf("%s : %u : Request Received : %s '%s' %s\n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port), ntohs(msg->OP_CODE) == READ_REQUEST ? "get" : "put", filename, mode_s);

	if (OP_CODE == READ_REQUEST)
		ProcessGet(s_fd, client_sock, slen, filename);
	else
		if (OP_CODE == WRITE_REQUEST)
			ProcessPut(s_fd, client_sock, slen, filename);

	printf("%s : %u : Transfer Completed : \n", inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
	close(s_fd);
	exit(0);
}


/*function definition for Handle_TFTP_Request*/
void Handle_TFTP_Request(TFTP_MSG *msg, ssize_t msg_len, struct sockaddr_in *client_sock, socklen_t slen, char *base_directory)
{
	int s_fd;
	/*protocol entry structure instance for (NAME, ALIASES, PROTOCOL NUMBER) of a protocol from <netdb.h>*/
	struct protoent *proto_instance; /*protocol instance*/
	/*timeval structure for (time elapsed seconds, time elapsed microseconds) from <sys/time.h>*/
	struct timeval time_elapsed;

	/*getting protocol details by name for protocol instance "udp" */
	if((proto_instance = getprotobyname("udp")) == 0)
	{
		fprintf(stderr, "Server got error trying to find service instance : getprotobyname(\"udp\") protocol\n");
		exit(1); /* exit if no such protocol is registered */
	}

	/* opening new socket, on new port, to handle client request */
	if((s_fd = socket(AF_INET, SOCK_DGRAM, proto_instance->p_proto)) == -1)
	{
		perror("Server got error in opening new socket : socket()");
		exit(1); /*exit if socket can't be opened*/
	}

	/*setting the value for elapsed time*/
	time_elapsed.tv_sec = RECV_TIMEOUT;
	time_elapsed.tv_usec = 0;
	/*setting receive timeout for the socket*/
	if(setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, &time_elapsed, sizeof(time_elapsed)) < 0)
	{
		perror("Server got error in setting RECV_TIMEOUT for the socket: setsockopt()");
		exit(1); /*exit if there is any error in setting the optvalue of socket*/
	}
	Parse_Client_Req(s_fd, msg, msg_len, client_sock, slen, base_directory);
}