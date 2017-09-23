/*FTP Client By CED15I031*/
/*FTP : File Transfer Protocol
client using TCP to implement a baisic FTP features such as listing of files, upload and download
feature.*/
/*To run this FTP-Client
0.  <dir> here is where you want the binary file and get all files downloaded to and uploaded from
1.  compile this program into file called FTP-Client: gcc FTP-Client.c -o <dir>FTP-Client
2.  got to <dir> run that output binary as ./FTP-Client <server_ip> <port>
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
#include <netdb.h>

#define BUFFER_LENGTH 256
#define DEFAULT_PORT 4567
#define TIMEOUT 1
#define RCV_BUFF_CLEAN memset(rcvbuff,'\0',BUFFER_LENGTH);
#define SND_BUFF_CLEAN memset(sndbuff,'\0',BUFFER_LENGTH);

void str_print(char a[])
{
	int i=0;
	while(a[i] != '\0')
	{
		if(a[i] == '\n')
			printf("\t");
		else
			printf("%c", a[i]);
		i++;
	}
	printf("\n");
}

void str_get(char a[], int size)
{
	int i;
	for(i=0; i < size - 1; i++)
	{
		scanf("%c", &a[i]);
		if(a[i] == '\n')
			break;
	}
	a[i] = '\0';
}

int main(int argc, char *argv[])
{
	if(argc < 2) /*if number of arguments are less*/
	{
		printf("Correct usage:\n\t%s <base directory> <port>\n", argv[0]);
		exit(1);
	}

	int network_socket, port;
	network_socket = socket(AF_INET, SOCK_STREAM, 0); /*creating socket*/
	struct  sockaddr_in server_address;

	if(argc == 2)
		port = DEFAULT_PORT; /*default port is 4567*/
	else
		port = atoi(argv[2]);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = inet_addr(argv[1]);

	int connect_stat = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address)); /*establishing connection*/
	if(connect_stat == -1)
	{
		perror("Client unable to connect : connect()");
		exit(0);
	}
	else
		printf("\t\t*** Connected to %s : %u using FTP ***\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
	
	while(1)
	{
		char rcvbuff[BUFFER_LENGTH],sndbuff[BUFFER_LENGTH];
		char file_name[BUFFER_LENGTH - 4];
		SND_BUFF_CLEAN
		printf("\nFTP Client prompt : ");
		str_get(sndbuff, BUFFER_LENGTH);
		send(network_socket, sndbuff, BUFFER_LENGTH, 0);//sending command to server
		sleep(TIMEOUT);
		RCV_BUFF_CLEAN
		recv(network_socket, rcvbuff, BUFFER_LENGTH, 0);
		if(strcmp(rcvbuff,"500SUPP") == 0)
		{
			printf("Command Accepted : %s...\n", rcvbuff);
			if(strcmp(sndbuff, "ls") == 0)
			{
				printf("Received From Server : \n");
				while(1)
				{
					recv(network_socket, rcvbuff, BUFFER_LENGTH, 0);
					if(strcmp(rcvbuff,"755ELIST") == 0)
					{
						break;
					}
					else
						str_print(rcvbuff);
				}
			}
			else
				if(strncmp(sndbuff, "get", 3) == 0)//if given command was a download command
				{
					char* fnmp=&sndbuff[4];
					strcpy(file_name,fnmp);

					
					recv(network_socket, rcvbuff, BUFFER_LENGTH, 0);//read first line to know if file exists or no(server will tell)
					
					if(strcmp(rcvbuff,"404ALIEN") == 0) 
					{
						printf("File Not Found : %s...\n", rcvbuff);
						
					}
					else
						if(strcmp(rcvbuff,"888NOPEN") == 0)
						{
							printf("Server Unable to Open File : %s...\n", rcvbuff);
							
						}
						else
							if(strcmp(rcvbuff,"777BEGIN") == 0)
							{	
								printf("File Transfer Started : %s...\n", rcvbuff);
								FILE* file_desc = fopen(file_name ,"w+");
								printf("Received From Server : \n");
								do
								{
									
									recv(network_socket, rcvbuff, BUFFER_LENGTH, 0);
									if(strcmp(rcvbuff,"778END") != 0)
									{
										str_print(rcvbuff);
										fputs(rcvbuff, file_desc);
									}
								}while(strcmp(rcvbuff,"778END")!=0);
								fclose(file_desc);
								printf("File Transfer Ended : %s...\n", rcvbuff);
								}
							else
							{
								printf("Client didn't receive the 777BEGIN code \n");
								continue;
							}			 
				}
				else
					if(strncmp(sndbuff, "put", 3) == 0)//uploading a file from client directory to server directory
					{
						char* fnmp=&sndbuff[4];
						strcpy(file_name,fnmp);
						SND_BUFF_CLEAN
						int exist_flag=access(file_name,F_OK);
						if(exist_flag == -1) /*file doesn't exist*/
						{
							printf("Client couldn't find the file \"%s\" : ", file_name);
							perror("access()");
							strcpy(sndbuff,"404ALIEN");
							send(network_socket, sndbuff, BUFFER_LENGTH, 0);
							sleep(TIMEOUT);
						}
						else
						{
							FILE* file_desc = fopen(file_name ,"r+"); /*open the existing file*/
							if(file_desc == NULL)
							{
								printf("Client couldn't open the file \"%s\" : ", file_name);
								perror("fopen()");
								strcpy(sndbuff,"888NOPEN");
								send(network_socket, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
							}
							else
							{
								printf("Packets to Server : \n");
								printf("Sending : 777BEGIN\n"); /*for server to know start of file*/
								strcpy(sndbuff,"777BEGIN");
								send(network_socket, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
								SND_BUFF_CLEAN
								while(fgets(sndbuff, BUFFER_LENGTH, file_desc) > 0) /*read the file line by line into buff*/
								{
									str_print(sndbuff);
									send(network_socket, sndbuff, BUFFER_LENGTH, 0); /*send the line in buff to server*/
									sleep(TIMEOUT);
									SND_BUFF_CLEAN
								}
								printf("Sending : 778END\n"); /*for server to know end of file*/
								SND_BUFF_CLEAN
								strcpy(sndbuff,"778END");
								send(network_socket, sndbuff, BUFFER_LENGTH, 0);
								sleep(TIMEOUT);
							}
							fclose(file_desc);
						}
					}
					else
						if((strcmp(sndbuff, "100END") == 0) || (strcasecmp(sndbuff, "exit") == 0))
						{
							printf("You are now disconnected...\n");
							exit(0);
						}
		}
		else
			if(strcmp(rcvbuff,"555UNSP") == 0)
			{
				printf("Command Unsupported hence Rejected : %s...\n", rcvbuff);
				/*RCV_BUFF_CLEAN
				recv(network_socket, rcvbuff, BUFFER_LENGTH, 0);
				if(strcmp(rcvbuff, "100END") == 0)
					printf("Server disconnected You\n");
				exit(0);*/
			}
	}		
	close(network_socket);
	return 0;
}