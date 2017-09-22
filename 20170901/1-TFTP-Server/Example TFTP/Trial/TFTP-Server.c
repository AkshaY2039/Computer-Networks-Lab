/*TFTP : Trivial File Transfer Protocol
server using UDP that should understand the TFTP protocol.
Implement a baisic TFTP features such as listing of files and download
feature.*/
/*To run this TFTP Server
1.	compile this program into file called TFTP-Server: gcc TFTP Server.c -o TFTP-Server
2.	copy this TFTP-Server binary to any folder where you want to run the server
3.	run that output binary as ./TFTP-Server <port> or ./TFTP-Server where port becomes 741 by default
Note: if it says permission denied to bind. Just try "sudo" before the runnung command 
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LIMIT 512 /*Maximum amount of file contents that can be buffered*/
#define PACKET_SIZE 516 /*Total size of TFTP packet*/

/*State Codes*/
#define STANDBY 1
#define RECEIVE 2
#define WAIT 3
#define SEND 4
#define RESET 5
#define FINISHED 6
#define DONE 0
#define FAILED 1
#define ABANDONED 2

typedef unsigned short opcode_t; /*TFTP Operation Code*/
#define OPCODE_RRQ   1
#define IS_RRQ(op)   ((op)==OPCODE_RRQ) /*Evaluates as true or false*/
#define OPCODE_WRQ   2
#define IS_WRQ(op)   ((op)==OPCODE_WRQ)
#define OPCODE_DATA  3
#define IS_DATA(op)  ((op)==OPCODE_DATA)
#define OPCODE_ACK   4
#define IS_ACK(op)   ((op)==OPCODE_ACK)
#define OPCODE_ERROR 5
#define IS_ERROR(op) ((op)==OPCODE_ERROR)

typedef unsigned short ecode_t; /*TFTP Error codes*/
#define ECODE_NONE  8
#define ECODE_0	 0
#define IS_ECODE_0(ec)  ((ec)==ECODE_0)
#define ESTRING_0   "Not defined, see error message(if any)."
#define ECODE_1	 1
#define IS_ECODE_1(ec)  ((ec)==ECODE_1)
#define ESTRING_1   "File not found."
#define ECODE_2	 2
#define IS_ECODE_2(ec)  ((ec)==ECODE_2)
#define ESTRING_2   "Access violation."
#define ECODE_3	 3
#define IS_ECODE_3(ec)  ((ec)==ECODE_3)
#define ESTRING_3   "Disk full or allocation exceeded."
#define ECODE_4	 4
#define IS_ECODE_4(ec)  ((ec)==ECODE_4)
#define ESTRING_4   "Illegal TFTP operation."
#define ECODE_5	 5
#define IS_ECODE_5(ec)  ((ec)==ECODE_5)
#define ESTRING_5   "Unknown transfer ID."
#define ECODE_6	 6
#define IS_ECODE_6(ec)  ((ec)==ECODE_6)
#define ECODE_7	 7
#define IS_ECODE_7(ec)  ((ec)==ECODE_7)
#define ESTRING_7   "No such user."

#define TIMEOUT		 1 /*Socket receive timeout*/
#define TIMEOUT_LIMIT   10

typedef unsigned short bnum_t; /*TFTP Block number, for data packets*/
typedef char packetbuffer_t; /*Packet buffer*/

/*Struct for storage of the data extracted from a packet buffer*/
typedef struct tftp_packet
{
	char filename[PACKET_SIZE];
	opcode_t opcode;
	char mode[PACKET_SIZE];
	char data[BUFFER_LIMIT];
	int data_length;
	bnum_t blocknum;
	ecode_t ecode;
	char estring[PACKET_SIZE];
	int estring_length;
}packet_t;

/*Struct for storage of data relating to the current transfer*/
typedef struct tftp_transaction
{
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
	char filebuffer[BUFFER_LIMIT];
	int filebuffer_length;
	char mode[PACKET_SIZE];
	bnum_t blocknum;
	ecode_t ecode;
	char estring[64];
	int estring_length;
}TFTP_TXN;

/*for the TFTP packet details*/
packetbuffer_t packet_in_buffer[PACKET_SIZE],*packet_in,*packet_out;
int packet_in_length,packet_out_length;
packet_t packet;
TFTP_TXN Trnxn;

/* Sockets,Ports,Addresses Variable*/
int socket1,socket2,addrlen,port2;
char *port;

struct sockaddr_in ftpServer,ftpClient;
struct sockaddr_in xfServer,xfClient;

/*File Handling functions*/
int file_open_read(char *filename,int *filedata)
{
	*filedata=open(filename,O_RDONLY);

	if(*filedata==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}
	else
		return 0;
}

int file_buffer_from_pos(TFTP_TXN *Trnxn)
{
	if((lseek(Trnxn->filedata,Trnxn->filepos,SEEK_SET))==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}

	int bytes_read=read(Trnxn->filedata,&Trnxn->filebuffer,BUFFER_LIMIT);
	if (bytes_read==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));	   
		return -1;
	}	
	return bytes_read;
}

int file_open_write(char *filename,int *filedata)
{
	*filedata=open(filename,O_RDWR | O_CREAT,0666);

	if(*filedata==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

int file_append_from_buffer(packet_t *packet,TFTP_TXN *Trnxn)
{
	if((write(Trnxn->filedata,packet->data,packet->data_length))==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

int file_close(int *filedata)
{
	if((close(*filedata))==-1)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}
	else
		return 0;
}

/*State Helper*/
static void wait_alarm(int signo)
{
	Trnxn.timed_out=1;
	signo++;
	return;
}

/*Bind Server to the port*/
void UDP_Bind_Server(int *ssock,char *port)
{
	*ssock=socket(AF_INET,SOCK_DGRAM,0);
	if(*ssock==-1)
	{
		printf("Socket Creation Failed: %s\n",strerror(errno));
		exit(1);
	}
	ftpServer.sin_family=AF_INET;
	ftpServer.sin_addr.s_addr=INADDR_ANY;
	ftpServer.sin_port=htons(atoi(port));
	if((bind(*ssock,(struct sockaddr *)&ftpServer,sizeof(ftpServer)))==-1)
	{
		printf("Could not bind server to socket: %s\n",strerror(errno));
		printf("Maybe you should try \"sudo\"\n");
		exit(1);
	}
}

/*ReBind Server*/
void UDP_ReBind_Server(int *ssock,int *port2)
{
	*ssock=socket(AF_INET,SOCK_DGRAM,0);
	if(*ssock==-1)
	{
		printf("Socket Creation Failed: %s\n",strerror(errno));
		exit(1);
	}
	xfServer.sin_family=AF_INET;
	xfServer.sin_addr.s_addr=INADDR_ANY;
	xfServer.sin_port=0;
	if((bind(*ssock,(struct sockaddr *)&xfServer,sizeof(xfServer)))==-1)
	{
		printf("Could not bind server to socket: %s\n",strerror(errno));
		printf("Maybe you should try \"sudo\"\n");
		exit(1);
	}
}

/*StandBy State*/
void StandBy(int *Operation)
{
	int leave=0;
	int readbytes=0;   
	Trnxn.timeout_count=0;
	Trnxn.blocknum=0;
	Trnxn.final_packet=0;
	Trnxn.file_open=0;
	Trnxn.complete=0;
	Trnxn.ecode=ECODE_NONE;
	Trnxn.filepos=0;
	packet_in_length=0;
	printf("\n\n\tTFTP Server is Online and in StandBy\n");

	while(leave==0)
	{
		addrlen=sizeof(ftpClient);
		readbytes=recvfrom(socket1,packet_in,PACKET_SIZE,0,(struct sockaddr *) &ftpClient,(socklen_t *) &addrlen);
		packet_in_length=readbytes;

		if(readbytes==-1)
		{
			fprintf(stderr,"Could not read command from socket!:%s\n",strerror(errno));
			continue;
		}

		UDP_ReBind_Server(&socket2,&port2);
		leave=1;
	}
	*Operation=DONE;
}

/*Packet Extract Opcode*/
void Packet_Extract_OPCODE(packet_t *packet,const packetbuffer_t *pbuf)
{
	memmove(&packet->opcode,pbuf,sizeof(opcode_t));
	packet->opcode=ntohs(packet->opcode);
}

/*Packet Parse RQ*/
void Packet_Parse_RQ(packet_t *packet,const packetbuffer_t *pbuf)
{
	strcpy(packet->filename,pbuf+sizeof(opcode_t));
	strcpy(packet->mode,pbuf+sizeof(opcode_t)+strlen(packet->filename)+1);
}

/*Packet Parse ACK*/
void Packet_Parse_ACK(packet_t *packet,const packetbuffer_t *pbuf)
{
	memcpy(&packet->blocknum,pbuf+sizeof(opcode_t),sizeof(bnum_t));
	packet->blocknum=ntohs(packet->blocknum);
}

/*Packet Parse DATA*/
void Packet_Parse_DATA(packet_t *packet,const packetbuffer_t *pbuf,int * data_length)
{
	memcpy(&packet->blocknum,pbuf+sizeof(opcode_t),sizeof(bnum_t));
	packet->blocknum=ntohs(packet->blocknum);
	packet->data_length=*data_length-sizeof(opcode_t)-sizeof(bnum_t);
	memcpy(packet->data,pbuf+sizeof(opcode_t)+sizeof(bnum_t),packet->data_length);
}

/*Packet Parse ERROR*/
void Packet_Parse_ERROR(packet_t *packet,const packetbuffer_t *pbuf,int * data_length)
{
	memcpy(&packet->ecode,pbuf+sizeof(opcode_t),sizeof(ecode_t));
	packet->ecode=ntohs(packet->ecode);
	packet->estring_length=*data_length-sizeof(opcode_t)-sizeof(ecode_t);
	memcpy(packet->estring,pbuf+sizeof(opcode_t)+sizeof(ecode_t),packet->estring_length);
}

/*append data to packet*/
packetbuffer_t *append_to_packet(const void *data,const int data_size)
{
	packet_out=realloc(packet_out,sizeof(packetbuffer_t)*packet_out_length+data_size);
	memmove(packet_out+packet_out_length,data,data_size);
	packet_out_length+=data_size;
	return packet_out;
}

/*Packet Form DATA*/
void Packet_Form_DATA()
{
	opcode_t opcode_out=htons(OPCODE_DATA);
	bnum_t new_blocknum=htons(Trnxn.blocknum);
	append_to_packet(&opcode_out,sizeof(opcode_t));
	append_to_packet(&new_blocknum,sizeof(bnum_t));
	append_to_packet(&Trnxn.filebuffer,Trnxn.filebuffer_length);
	printf("\rTransferred block: %i",Trnxn.blocknum);
}

/*Packet Form ERROR*/
void Packet_Form_ERROR()
{
	opcode_t opcode_out=htons(OPCODE_ERROR);
	ecode_t new_ecode=htons(Trnxn.ecode);
	append_to_packet(&opcode_out,sizeof(opcode_t));
	append_to_packet(&new_ecode,sizeof(ecode_t));
	append_to_packet(Trnxn.estring,strlen(Trnxn.estring)+1); 
}

/*Packet Form ACK*/
void Packet_Form_ACK()
{
	opcode_t opcode_out=htons(OPCODE_ACK);
	printf("\rReceived block: %i",Trnxn.blocknum);
	bnum_t new_blocknum=htons(Trnxn.blocknum);
	append_to_packet(&opcode_out,sizeof(opcode_t));
	append_to_packet(&new_blocknum,sizeof(bnum_t));
}

/*Parse Packet*/
int Parse_Packet(packet_t *packet,const packetbuffer_t *pbuf,int *packet_in_length)
{   
	Packet_Extract_OPCODE(packet,pbuf);

	if(IS_RRQ(packet->opcode)||IS_WRQ(packet->opcode))
		Packet_Parse_RQ(packet,pbuf);
	else
		if(IS_ACK(packet->opcode))
			Packet_Parse_ACK(packet,pbuf);
		else
			if(IS_DATA(packet->opcode))
				Packet_Parse_DATA(packet,pbuf,packet_in_length);
			else
				if(IS_ERROR(packet->opcode))
					Packet_Parse_ERROR(packet,pbuf,packet_in_length);
				else
					return -1;	
	return 0;
}

/*Free the Packet*/
void Packet_Free()
{
	if (packet_out_length>0)
	{
		free(packet_out);
		packet_out_length=0;
		packet_out=NULL;
	}
}

/*Packet Receive Read Request*/
int Packet_Receive_RRQ()
{
	printf("Read request for %s\n",packet.filename);
	Packet_Free();

	if(Trnxn.file_open==1)
		file_close(&Trnxn.filedata);

	if((file_open_read(packet.filename,&Trnxn.filedata))==-1)
	{
		strcpy(Trnxn.estring,ESTRING_1);
		Trnxn.ecode=ECODE_1;
		Packet_Form_ERROR();
	}
	else
	{
		Trnxn.file_open=1;
		Trnxn.blocknum=1;
		Trnxn.filepos=((Trnxn.blocknum * BUFFER_LIMIT)-BUFFER_LIMIT);
		Trnxn.filebuffer_length=file_buffer_from_pos(&Trnxn);
		Packet_Form_DATA();
	}
	return DONE;
}

/*Packet Receive Write Request*/
int Packet_Receive_WRQ()
{
	printf("Write request for %s\n",packet.filename);
	Packet_Free();

	if(Trnxn.file_open==0)
	{
		if((file_open_write(packet.filename,&Trnxn.filedata))==0)
			Trnxn.file_open=1;
		else
			Trnxn.file_open=0;
	}

	if(Trnxn.file_open==0)
	{
		strcpy(Trnxn.estring,ESTRING_1);
		Trnxn.ecode=ECODE_1;
		Packet_Form_ERROR();	   
	}
	else
	{
		Packet_Form_ACK();
		Trnxn.blocknum++;
	}
	return DONE;
}

/*Packet Receive Data*/
int Packet_Receive_DATA()
{
	if(packet.blocknum==Trnxn.blocknum)
	{
		if(file_append_from_buffer(&packet,&Trnxn)==-1)
		{
			strcpy(Trnxn.estring,ESTRING_2);
			Trnxn.ecode=ECODE_2;
			Packet_Free();
			Packet_Form_ERROR();		   
		}
		else
		{
			Packet_Free();
			Packet_Form_ACK();
			Trnxn.blocknum++;
		}
	}

	if(packet.data_length<512)
	{
		file_close(&Trnxn.filedata);
		Trnxn.file_open=0;
		Trnxn.complete=1;
	}
	return DONE;
}

/*Packet Receive Acknowledgement*/
int Packet_Receive_ACK()
{
	if(packet.blocknum==Trnxn.blocknum)
	{
		Trnxn.blocknum++;
		Trnxn.timeout_count=0;
		Packet_Free();

		if (Trnxn.file_open==0&&(file_open_read(packet.filename,&Trnxn.filedata))==-1)
		{
			strcpy(Trnxn.estring,ESTRING_2);
			Trnxn.ecode=ECODE_2;
			Packet_Form_ERROR();			
		}
		else
		{
			Trnxn.filepos=((Trnxn.blocknum * BUFFER_LIMIT)-BUFFER_LIMIT);
			Trnxn.filebuffer_length=file_buffer_from_pos(&Trnxn);
			if (!Trnxn.filebuffer_length)
			{
				Trnxn.complete=1;
				return ABANDONED;			   
			}
			else
				Packet_Form_DATA();
		}
	}
	return DONE;
}

/*Packet Receive Invalid*/
int Packet_Receive_INVALID()
{  
	Trnxn.ecode=ECODE_4;
	strcpy(Trnxn.estring,ESTRING_4);
	Packet_Free();
	Packet_Form_ERROR();
	return DONE;
}

/*Packet Receive ERROR*/
int Packet_Receive_ERROR()
{
	fprintf(stderr,"Received error %i: %s\n",packet.ecode,packet.estring);
	return ABANDONED;
}

/*Receive State*/
void Receive(int *Operation)
{
	Parse_Packet(&packet,packet_in,&packet_in_length);

	if(IS_RRQ(packet.opcode))
		*Operation=Packet_Receive_RRQ();
	else
		if(IS_WRQ(packet.opcode))
			*Operation=Packet_Receive_WRQ();
		else
			if(IS_ACK(packet.opcode))
				*Operation=Packet_Receive_ACK();
			else
				if(IS_DATA(packet.opcode))
					*Operation=Packet_Receive_DATA();
				else
					if(IS_ERROR(packet.opcode))
						*Operation=Packet_Receive_ERROR();
					else
						*Operation=Packet_Receive_INVALID();
}

/*Wait State*/
void Wait(int *Operation)
{
	int readbytes=0;
	Trnxn.timed_out=0;
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	sa.sa_handler=(void(*)(int)) wait_alarm;
	sigemptyset(&sa.sa_mask);
	int res=sigaction(SIGALRM,&sa,NULL);

	if(res==-1)
	{
		fprintf(stderr,"Unable to set SA_RESTART to false: %s\n",strerror(errno));
		abort();
	}

	alarm(TIMEOUT);

	if((readbytes=recvfrom(socket2,packet_in,PACKET_SIZE,0,(struct sockaddr *)&ftpClient,(socklen_t *)&addrlen)) < 0)
	{
		if((errno==EINTR)&&Trnxn.timed_out)
			Trnxn.timeout_count++;		   
		else
			fprintf(stderr,"%s\n",strerror(errno));
		
		if (Trnxn.timeout_count==TIMEOUT_LIMIT)
		{
			fprintf(stderr,"Timeout.\n");
			*Operation=ABANDONED;		   
		}
		else
			*Operation=FAILED;		
	}
	else
	{
		alarm(0);
		Trnxn.timeout_count=0;
		packet_in_length=readbytes;
		*Operation=DONE;
	} 
}

/*UDP Send Packet*/
void UDP_Send_Packet(int *ssock,struct sockaddr* dest,packetbuffer_t ** packet_out,int * length)
{	  
	int bytes=sendto(*ssock,*packet_out,*length,0,(struct sockaddr *)dest,sizeof(*dest));

	if(bytes==-1)
	{
		printf("Could not send packet via socket: %s\n",strerror(errno));
		exit(1);
	}
}

/*Send State*/
void Send(int *Operation)
{
	UDP_Send_Packet(&socket2,(struct sockaddr *)&ftpClient,&packet_out,&packet_out_length);

	if(Trnxn.complete==1||Trnxn.ecode!=ECODE_NONE)
	{
		printf("\nSent.\n");
		*Operation=ABANDONED;
	}
	else
		*Operation=DONE;
}

/*Reset State*/
void Reset(int *Operation)
{
	if(Trnxn.file_open==1)
		file_close(&Trnxn.filedata);

	Packet_Free();
	close(socket2);
	*Operation=DONE;
}

/*State Handler*/
void server_state(int *state,int *operation)
{
	switch(*state)
	{
		case STANDBY:switch(*operation)
					{
						case DONE:	*state=RECEIVE;
									break;
					}
					break;
		case RECEIVE:switch(*operation)
					{
						case DONE:	*state=SEND;
									break;
						case ABANDONED:	*state=RESET;
										break;
					}
					break;
		case WAIT:	switch(*operation)
					{
						case DONE:	*state=RECEIVE;
									break;
						case FAILED:*state=SEND;
									break;
						case ABANDONED:	*state=RESET;
										break;
					}
					break;
		case SEND:	switch(*operation)
					{
						case DONE:	*state=WAIT;
									break;
						case ABANDONED:	*state=RESET;
										break;
					}
					break;
		case RESET:	switch(*operation)
					{
						case DONE:	*state=STANDBY;
									break;
					}
					break;
	}
}

/*Server Main Module*/
int main(int argc,char *argv[])
{
	if(argc<1)
	{
		printf("Usage: %s <port_number>\n",argv[0]);
		exit(1);
	}
	int Current_State=STANDBY;
	int Operation;
	if(argv[1]==NULL)
	{
		port="741";
		printf("Port set by default as 741\n");
	}
	else
		port=argv[1];
	UDP_Bind_Server(&socket1,port);
	while(Current_State!=FINISHED)
	{
		switch(Current_State)
		{
			case STANDBY:	StandBy(&Operation);
							break;
			case RECEIVE:	Receive(&Operation);
							break;
			case WAIT:		Wait(&Operation);
							break;
			case SEND:		Send(&Operation);
							break;
			case RESET:		Reset(&Operation);
							break;
		}
		server_state(&Current_State,&Operation);
	}
	return 0;
}