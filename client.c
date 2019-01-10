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

		// printf("Listening...\n");
		if((recv_size = recv(*s,server_reply,2000,0))==SOCKET_ERROR)
		{
			printf("Server disconnected.\n");
			STOP = 1;
			return (void*)1;
		}
		// printf("Recived\n");
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
		// printf("Sending %s\n",message );

		if(send(*s,message,strlen(message),0)<0)
		{
			printf("Cannot send message, server disconnected.\n");
			STOP = 1;
			return (void*)1;
		}
		// printf("sent.\n");		
	}
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

	int validate;
	do
	{
		printf("Insert username: ");
		char message[30],valid[30];
		validate = 0;
		scanf("%s",message);
		fflush(stdin);
		int recv_size;


		if(send(s,message,strlen(message),0)<0)
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
	
	} while(!validate);
	
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
