#include "stdio.h"
#include "winsock2.h"
#include <pthread.h> 

int STOP = 0;

void* read(void* args)
{
	while(1)
	{
		SOCKET* s = (SOCKET *) args;
		int recv_size;
		char server_reply[2000];

		if((recv_size = recv(*s,server_reply,2000,0))==SOCKET_ERROR)
		{
			printf("Server disconnected.\n");
			STOP = 1;
			return (void*)1;
		}
		server_reply[recv_size]='\0';
		printf("%s\n",server_reply );
	}
}

void* write(void* args)
{
	while(1)
	{
		SOCKET* s = (SOCKET *) args;
		char message[30];
		scanf("%[^\n]%*c",message);
		fflush(stdin);

		if(send(*s,message,strlen(message),0)<0)
		{
			printf("Cannot send message, server disconnected.\n");
			STOP = 1;
			return (void*)1;
		}
	}
}

int validateUsername(char* username)
{
	int i;
	for(i=0;i<strlen(username);i++)
	{
		if(username[i]=='*')
			return 0;
	}

	return 1;
}

int setCommand(char* commandServer, char cmd)
{
	char login[15]="\\LOG_IN*";
	char singin[15]="\\SGN_IN*";

	switch(cmd)
	{
		case '1':
			strcpy(commandServer,singin);
		break;
		
		case '2':
			strcpy(commandServer,login);
		break;

		default: return 0; break;
	}

	return 1;
}


int main(int argc, char const *argv[])
{
	WSADATA wsa;
	SOCKET s;
	printf("Loading...\n");
	if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)//init library
	{
		printf("Error on loading\n");
		return 1;
	}

	if((s=socket(AF_INET, SOCK_STREAM,0)) == INVALID_SOCKET)//socket(ipv4,tcp,ip)
	{
		printf("Could not create socket err: %d\n", WSAGetLastError());
		return 1;
	}

	printf("Insert IP address of server: ");
	char IP[16]; 
	scanf("%s",IP);
	fflush(stdin);

	// strcpy(IP,"127.0.0.1");

	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(8080);

	if(connect(s,(struct sockaddr *)&server, sizeof(server))<0)
	{
		printf("Connection Error.\n");
		return 1;
	}
	printf("Connected!\n");

	char command;
	char *commandServer=calloc(15,sizeof(char));
	do
	{
		printf("Choose:\n");
		printf("1 - Sing in\n");
		printf("2 - Log in\n");

		scanf("%c",&command);
		fflush(stdin);

		int validate;
		char username[30], password[1000], valid[30];
		
		validate = 0;
		int recv_size;

		do
		{
			if(setCommand(commandServer,command))
			{
				do
				{
					printf("Insert username: ");
					scanf("%s",username);
					fflush(stdin);
				} while(!validateUsername(username));

				printf("Insert password: ");
				scanf("%s",password);
				fflush(stdin);

				strcat(commandServer,username);

				if(send(s,commandServer,strlen(commandServer),0)<0)
				{
					printf("Cannot send username!\n");
					return 1;
				}

				if(send(s,password,strlen(password),0)<0)
				{
					printf("Cannot send username!\n");
					return 1;
				}

				if((recv_size = recv(s,valid,30,0)) != SOCKET_ERROR)
				{
					valid[recv_size]='\0';
					if(!strcmp(valid,"\\USRNM_N_VLD"))
						validate=0;
					else
					if(!strcmp(valid,"\\USRNM_VLD"))
						validate = 1;
				}

				if(!validate)
					printf("Username already used or invalid.\n");
			}
			else
				break;
		
		} while(!validate);

	} while(command !='1' && command!='2');	
	
	printf("Validated\n");
	SOCKET *s1 = malloc(sizeof(SOCKET));
	*s1 = s;

	SOCKET *s2 = malloc(sizeof(SOCKET));
	*s2 = s;

	pthread_t readTh; 
	// printf("Reading Thread Started\n"); 
	pthread_create(&readTh, NULL, read, (void*)s1);

	pthread_t writeTh; 
	// printf("Writing Thread Started\n"); 
	pthread_create(&writeTh, NULL, write, (void*)s2);

	printf("Chat started!\n");

	while(!STOP);

	printf("Exiting.\n");
	pthread_cancel(readTh);
	pthread_cancel(writeTh);

	closesocket(s);
	WSACleanup();

	return 0;
}
