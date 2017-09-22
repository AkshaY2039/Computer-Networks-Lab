/*FTP Server By CED15I031*/
/*FTP : File Transfer Protocol
server using TCP to implement a baisic FTP features such as listing of files, upload and download
feature.*/
/*To run this FTP-Server
0.  <dir> here is where you want the binary file while <Directory> is the base for Server
1.  compile this program into file called FTP-Server: gcc FTP-Server.c -o <dir>FTP-Server
2.  got to <dir> run that output binary as ./FTP-Server <Directory> <port> or else default port set to 4567
Note: if it says permission denied to bind. Just try "sudo" before the runnung command 
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_LENGTH 256 /*The length for send and receive buffer*/
#define DEFAULT_PORT 4567 /*To set the default port when port number is missing in CL arguments*/
#define BACKLOG 10 /*set BACKLOG for the sever socket*/
#define TIMEOUT 1 /*for wait after sending any packet*/
#define RCV_BUFF_CLEAN memset(rcvbuff,'\0',BUFFER_LENGTH); /*to clean the receive buffer*/
#define SND_BUFF_CLEAN memset(sndbuff,'\0',BUFFER_LENGTH); /*to clean the send buffer*/

/* base directory */
char *base_directory;

void str_print(char a[]) /*to print a string in a defined way*/
{
	int i=0;
	while(a[i] != '\0') /*until the */
	{
		if(a[i] == '\n')
			printf("\t");
		else
			printf("%c", a[i]);
		i++;
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	if(argc < 2) /*if number of arguments are less*/
	{
		printf("Correct usage:\n\t%s <base directory> <port>\n", argv[0]);
		exit(1);
	}

	base_directory = argv[1]; /*base directory*/

	if(chdir(base_directory) == -1)
	{
		perror("Server unable to change directory : chdir()");
		exit(1);
	}

	int i, port, server_socket;
	if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) /*creating socket*/
	{
		perror("Server was unable to open a socket : socket()");
		exit(0);
	}
	struct sockaddr_in server_address,client_address; /*server socket*/

	if(argc == 2)
		port = DEFAULT_PORT;
	else
		port = atoi(argv[2]);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_address.sin_zero),'\0',8);

	if(bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr)) == -1) /*binding to socket*/
	{
		perror("Server unable to bind : bind()");
		exit(1);
	}

	if(listen(server_socket, BACKLOG) == -1) /*start listening on the socket*/
	{
		perror("Server Listen Failed : listen()");
		exit(1);
	}

	printf("\n\t\t*** FTP-Server is listening on port : %d ***\n", ntohs(server_address.sin_port));

	while(1)
	{
		int client_sock, sin_size = sizeof(struct sockaddr_in);
		if(client_sock = (accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&sin_size)) == -1) /*establish connection with client*/
		{
			perror("Error in accpeting connection : accept()");
			exit(0);
		}
		printf("\n\t\t*** Connection from %s : %u ***\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
		pid_t pid;
		if((pid = fork()) < 0) /*for concurrent server*/
			perror("Server fork failed : ");
		if(pid == 0) /*here child's job is specified*/
		{
			close(server_socket); /*no need of the server socket in child*/
			char rcvbuff[BUFFER_LENGTH],sndbuff[BUFFER_LENGTH];
			char file_name[BUFFER_LENGTH - 4];
			int connection_flag = 0;
			while(1)
			{
				RCV_BUFF_CLEAN
				recv(client_sock, rcvbuff, BUFFER_LENGTH, 0); /*receive first command from client*/
				printf("\nRequest from %s : %u : ", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
				str_print(rcvbuff);
				if(strncmp(rcvbuff, "ls", 2) == 0) /*executing ls*/
				{
					SND_BUFF_CLEAN
					strcpy(sndbuff,"500SUPP");
					send(client_sock, sndbuff, BUFFER_LENGTH, 0);
					sleep(TIMEOUT);
					char command[20];
					sprintf(file_name, "%d.txt", getpid());
					sprintf(command, "ls > %s", file_name);
					system(command); /*output of ls into list.txt*/
					FILE* file_desc;
					file_desc = fopen(file_name,"r");
					printf("Packets to Client : \n");
					SND_BUFF_CLEAN
					while(fgets(sndbuff, BUFFER_LENGTH, file_desc) > 0) /*read the content of list.txt into buffer*/
					{
						if(strncmp(sndbuff,file_name,8) != 0)
						{
							send(client_sock, sndbuff, BUFFER_LENGTH, 0); /*send the contents of buffer to client*/
							str_print(sndbuff);
							sleep(TIMEOUT);
						}
						SND_BUFF_CLEAN
					}
					SND_BUFF_CLEAN
					strcpy(sndbuff,"755ELIST");
					send(client_sock, sndbuff, BUFFER_LENGTH, 0);
					sleep(TIMEOUT);
					sprintf(command, "rm %s", file_name);
					system(command); /*remove the temporary list.txt*/
					fclose(file_desc);
				}
				else
					if(strncmp(rcvbuff, "get", 3) == 0) /*provide the file to client*/
					{
						SND_BUFF_CLEAN
						strcpy(sndbuff,"500SUPP");
						send(client_sock, sndbuff, BUFFER_LENGTH, 0);
						sleep(TIMEOUT);
						char* fnmp=&rcvbuff[4];
						strcpy(file_name,fnmp);
						int exist_flag=access(file_name,F_OK);
						if(exist_flag == -1) /*file doesn't exist*/
						{
							printf("Server couldn't find the file \"%s\" : ", file_name);
							perror("access()");
							SND_BUFF_CLEAN
							strcpy(sndbuff,"404ALIEN");
							send(client_sock, sndbuff, BUFFER_LENGTH, 0);
							sleep(TIMEOUT);
						}
						else
						{
							FILE* file_desc = fopen(file_name ,"r+"); /*open the existing file*/
							if(file_desc == NULL)
							{
								printf("Server couldn't open the file \"%s\" : ", file_name);
								perror("fopen()");
								SND_BUFF_CLEAN
								strcpy(sndbuff,"888NOPEN");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
							}
							else
							{
								printf("Packets to Client : \n");
								printf("Sending : 777BEGIN\n"); /*for client to know start of file*/
								SND_BUFF_CLEAN
								strcpy(sndbuff,"777BEGIN");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
								SND_BUFF_CLEAN
								while(fgets(sndbuff, BUFFER_LENGTH, file_desc) > 0) /*read the content of list.txt into buffer*/
								{
									send(client_sock, sndbuff, BUFFER_LENGTH, 0); /*send the contents of buffer to client*/
									str_print(sndbuff);
									sleep(TIMEOUT);
									SND_BUFF_CLEAN
								}
								printf("Sending : 778END\n"); /*for client to know end of file*/
								SND_BUFF_CLEAN
								strcpy(sndbuff,"778END");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
							}
							fclose(file_desc);
						}
					}
					else
						if(strncmp(rcvbuff, "put", 3) == 0) /*receive file from client*/
						{
							SND_BUFF_CLEAN
							strcpy(sndbuff,"500SUPP");
							send(client_sock, sndbuff, BUFFER_LENGTH, 0);
							sleep(TIMEOUT);
							char* fnmp=&rcvbuff[4];
							strcpy(file_name,fnmp);
							RCV_BUFF_CLEAN
							recv(client_sock, rcvbuff, BUFFER_LENGTH, 0);//read(client_sock, rcvbuff, BUFFER_LENGTH);
							if(strcmp(rcvbuff,"777BEGIN") == 0)
							{
								FILE* file_desc = fopen(file_name ,"w+");
								printf("Received from Client : \n");
								do
								{
									RCV_BUFF_CLEAN
									recv(client_sock, rcvbuff, BUFFER_LENGTH, 0);//read(client_sock, rcvbuff, BUFFER_LENGTH);
									if(strcmp(rcvbuff,"778END") != 0)
									{
										str_print(rcvbuff);
										fputs(rcvbuff, file_desc);
									}
								}while(strcmp(rcvbuff,"778END")!=0); 
								fclose(file_desc);
							}
							else
							{
								printf("Server didn't receive the 777BEGIN code \n");
							}
						}
						else
							if((strcmp(rcvbuff, "100END") == 0) || (strcasecmp(rcvbuff, "exit") == 0))
							{
								SND_BUFF_CLEAN
								strcpy(sndbuff,"500SUPP");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
								connection_flag=1;
								break;
							}
							else
							{
								SND_BUFF_CLEAN
								strcpy(sndbuff,"555UNSP");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
								SND_BUFF_CLEAN
								strcpy(sndbuff,"100END");
								send(client_sock, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
								connection_flag=2;
								break;
							}
			}
			printf("Connection to %s : %u : closed", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			close(client_sock);
			switch(connection_flag)
			{
				case 1:	printf(" from Client\n");
						break;
				case 2:	printf(" by Server\n");
						break;
				default:printf(" Client Crashed\n");
			}
			exit(0);
		}
		else
			if(pid > 0)
		{
			wait(NULL);
			close(client_sock);
		}
	}
	return 0;
}