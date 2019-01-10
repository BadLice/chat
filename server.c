#include "stdio.h"
#include "winsock2.h"
#include "string.h"
#include "pthread.h" 
#include "stdlib.h"

#ifndef PORT
#define PORT 8080
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 100
#endif

int toUpdate=-1;

pthread_mutex_t lock;

typedef struct
{
	int valid;//0 no, 1 yes
	SOCKET socket;
	char username[32];
} CLIENT;

typedef struct
{
	CLIENT *clients;
	SOCKET *s;
} sniffStr;

typedef struct
{
	CLIENT *clients;
	int currentIndex;
	void * thread_id;

} updateStr;

int isUsed(char* msg, CLIENT* clients)
{
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		if(!strcmp(clients[i].username,msg))
			return 1;
	}
	return 0;
}

int getSocketIndex(CLIENT *clients)
{
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		if(!clients[i].valid)
			return i;
	}
	return -1;
}

CLIENT* initClients()
{
	CLIENT *c = calloc(MAX_CLIENTS,sizeof(CLIENT));
	int i=0;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		c[i].socket = INVALID_SOCKET;
		c[i].username[0]='\0';
		c[i].valid=0;
	}

	return c;
}


void* updateClient(void* arg)
{
	
	while(1)
	{
		pthread_mutex_lock(&lock);
		updateStr *args = (updateStr*) arg;
		int recv_size;
		char msg[1000];
		pthread_mutex_unlock(&lock);

		if((recv_size = recv(args->clients[args->currentIndex].socket,msg,1000,0)) != SOCKET_ERROR) //recieve from current and send to all other clients
		{
			pthread_mutex_lock(&lock);
			msg[recv_size]='\0';

			if(strlen(args->clients[args->currentIndex].username)==0)
			{
				if(strlen(msg)==0 || isUsed(msg,args->clients))
				{
					printf("Invalid username from a new connection, asking again...\n");
					char sz[30] = "\\USRNM_N_VLD";//username not valid
					send(args->clients[args->currentIndex].socket,sz,strlen(sz),0);
				}
				else
				{
					strcpy(args->clients[args->currentIndex].username,msg);
					printf("Client %s connected!\n", args->clients[args->currentIndex].username);
					char sz[30] = "\\USRNM_VLD";//username valid	
					send(args->clients[args->currentIndex].socket,sz,strlen(sz),0);

				}

			}
			else
			{
				int i;
				for(i=0;i<MAX_CLIENTS;i++)
				{
					if(i!=args->currentIndex & args->clients[i].socket != INVALID_SOCKET)
					{
						char sz[1032] = "";
						strcat(sz,"[");
						strcat(sz,args->clients[args->currentIndex].username);
						strcat(sz,"]: ");
						strcat(sz,msg);
						// printf("Sending :\"%s\" to [%s] - currentIndex=%d, i=%d\n",sz,args->clients[i].username,args->currentIndex,i);

						send(args->clients[i].socket,sz,strlen(sz),0);
					}
				}
			}

		}
		else
		{
			printf("Client %s disconnected.\n",args->clients[args->currentIndex].username);
			args->clients[args->currentIndex].username[0]='\0';
			args->clients[args->currentIndex].socket = INVALID_SOCKET;
			args->clients[args->currentIndex].valid=0;

			pthread_mutex_unlock(&lock);
			return (void*) 1;
		}
		pthread_mutex_unlock(&lock);
		
	}
}


void* sniff(void* arg)//listen and accept new connections
{
	sniffStr *args = (sniffStr*) arg;
	while(1)
	{
		int si = getSocketIndex(args->clients);

		struct sockaddr_in client;

		listen(*(args->s),3);

		//new client connected
		int c = sizeof(struct sockaddr_in);
		args->clients[si].socket = accept(*(args->s),(struct sockaddr *)&client, &c);
		if(args->clients[si].socket == INVALID_SOCKET)
		{
			printf("Error connecting to new client! Error: %d\n", WSAGetLastError());
		}
		else
		{
			args->clients[si].valid=1;
		}

		toUpdate=si;
	}
}


int main(int argc, char const *argv[])
{
	printf("Loading...\n");

	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;
	CLIENT* clients = initClients();
	pthread_t updateTh[MAX_CLIENTS];

	if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)//init library
	{
		printf("Library cannot be initialized!\nExiting...");
		return 1;
	}

	if((s = socket(AF_INET,SOCK_STREAM,0))== INVALID_SOCKET)
	{
		printf("Cannot create socket! Error: %d\n",WSAGetLastError());
		return 1;
	}

	printf("Socket created.\n");

	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);

	if(bind(s,(struct sockaddr *)&server,sizeof(server)) == SOCKET_ERROR)
	{
		printf("Cannot bind socket. Error: %d\n",WSAGetLastError() );
	}

	printf("Socket bound.\n");

	sniffStr arg;
	arg.clients = clients;
	arg.s = &s;
	
	pthread_t sniffTh; 
	printf("Chat started.\nListening on port %d.\n",PORT);	
	pthread_create(&sniffTh, NULL, sniff, (void*) &arg); 

	while(1)
	{
		if(toUpdate!=-1)
		{
			int si = toUpdate;
			updateStr *rg = malloc(sizeof(updateStr));
			rg->currentIndex=si;
			rg->clients = clients;
			int *k = (int *)malloc(sizeof(int));
			*k = si;
			rg->thread_id = (void*)k;

			// printf("Update Thread for client %d started....\n",si); 
			pthread_create(&updateTh[si], NULL, updateClient, (void*) rg); 
			toUpdate=-1;
		}	
		
	}

	closesocket(s);
	WSACleanup();

	return 0;
}