/*TFTP Server By CED15I031*/
/*TFTP : Trivial File Transfer Protocol
server using UDP that should understand the TFTP protocol.
Implement a baisic TFTP features such as listing of files and download
feature.*/
/*To run this TFTP Server (copy all three files TFTP.c TFTP-Server.c TFTP.h into one folder)
0.  <dir> here is where you want the binary file while <Directory> is the base for Server
1.	compile this program into file called TFTP-Server: gcc TFTP-Server.c TFTP.c -o <dir>TFTP-Server
2.	got to <dir> run that output binary as ./TFTP-Server <Directory> <port>
Note: if it says permission denied to bind. Just try "sudo" before the runnung command 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "TFTP.h"

/* base directory */
char *base_directory;

void trm_handler(int sig)
{
	int status;
	wait(&status);
}

/*Main Module for the Server*/
int main(int argc, char *argv[])
{
	int s_fd;
	uint16_t port = 0; /*port is a 16b number*/
	struct protoent *proto_instance; /* protocol instance*/
	struct servent *service_instance; /*service entry*/
	struct sockaddr_in server_sock; /*server socket*/

	if(argc < 2) /*if number of arguments are less*/
	{
		printf("Correct usage:\n\t%s <base directory> <port>\n", argv[0]);
		exit(1);
	}

	base_directory = argv[1]; /*base directory*/

	if(chdir(base_directory) < 0)
	{
		perror("Server unable to change directory : chdir()");
		exit(1);
	}

	if(argc > 2)
	{
		if(sscanf(argv[2], "%hu", &port))
		{
			port = htons(port);
		}
		else
		{
			fprintf(stderr, "Error: invalid port number\n");
			exit(1);
		}
	}
	else
	{
		if((service_instance = getservbyname("tftp", "udp")) == 0)
		{
			fprintf(stderr, "Server got error trying to find service instance : getservbyname() \n");
			exit(1);
		}
	}
	
	/*getting the UDP protocol number*/
	if((proto_instance = getprotobyname("udp")) == 0)
	{
		fprintf(stderr, "Server got error trying to find service instance : getprotobyname(\"udp\") protocol\n");
		exit(1);
	}

	/*opening a socket with  that UDP protocol*/
	if((s_fd = socket(AF_INET, SOCK_DGRAM, proto_instance->p_proto)) < 0)
	{
		perror("Server got error in opening socket : socket()");
		exit(1);
	}

	/*initializing server socket*/
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sock.sin_port = port ? port : service_instance->s_port;

	if(bind(s_fd, (struct sockaddr *) &server_sock, sizeof(server_sock)) < 0)
	{
		perror("Server got a binding error : bind()");
		close(s_fd);
		exit(1);
	}

	signal(SIGCLD, (void *) trm_handler); /*wait for terminanation signal*/

	printf("TFTP Server is on : listening on Port : %d\n", ntohs(server_sock.sin_port));

	while(1) /*start listening for clients */
	{
		struct sockaddr_in client_sock;
		socklen_t slen = sizeof(client_sock);
		ssize_t len;

		TFTP_MSG message;
		uint16_t OP_CODE;

		if((len = Recv_TFTP_MSG(s_fd, &message, &client_sock, &slen)) < 0) /*keep listening for clients*/
		{
			continue;
		}

		if(len < 4) /*in case the message is not one that is expected right now i.e. not a request*/
		{ 
			printf("%s : %u : Invalid Message Size Received \n", inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));
			Send_TFTP_ERROR(s_fd, 0, "Invalid Message Size", &client_sock, slen);
			continue;
		}

		OP_CODE = ntohs(message.OP_CODE);

		if(OP_CODE == READ_REQUEST || OP_CODE == WRITE_REQUEST)
		{
			/* make a new child process to handle the request */
			if (fork() == 0)
			{
				Handle_TFTP_Request(&message, len, &client_sock, slen, base_directory);
				exit(0);
			}
			else
			{
				wait(0);
			}
		}
		else
		{
			printf("%s : %u : INVALID REQUEST RECEIVED : %d \n", inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port), OP_CODE);
			Send_TFTP_ERROR(s_fd, 0, "INVALID OP_CODE", &client_sock, slen);
		}
	}

	close(s_fd); /*close server socket*/
	return 0;
}