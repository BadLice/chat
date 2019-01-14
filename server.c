//if (password of x) = (username y) ?

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
char accountPath[30] = "AccountDB.db";
pthread_mutex_t lock;

typedef struct
{
	int valid;//0 no, 1 yes
	SOCKET socket;
	char username[33];
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

int isOnline(CLIENT *clients,char* username)
{
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		if(clients[i].valid)
			if(!strcmp(username,clients[i].username))
				return 1;
	}
	return 0;
}

int file_exists(const char * filename)
{
	FILE *file;
    if ( file = fopen(filename, "r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

void createFirstFile(const char* path)
{
	if(!file_exists(path))
	{
		FILE* f = fopen(path,"w");
		fclose(f);
	}
}

int strContains(char* str, const char* cont)
{
	int i,j;
	for(i=0;i<strlen(str);i++)
	{
		if(str[i]==cont[0])
		{ 
			int ok = 1;
			for(j=0;(i+j)<strlen(str) && j<strlen(cont) &&ok;j++)
			{
				if(str[i+j]!=cont[j])
					ok = 0;
			}

			if(ok)
				return 1;
		}
	}

	return 0;
}


int isUsedUsername(char* msg,char* path)
{
	int ok = 0;
	char buff[100]="";
	strcat(buff,"-");
	strcat(buff,msg);

	FILE* f = fopen(path,"r");
	char usrnm[33];

	while(!feof(f) && !ok)
	{
		if(fgets(usrnm,33,f)!=NULL)
		{
			usrnm[strlen(usrnm)-1] = '\0';
			if(!strcmp(usrnm,buff))
				ok=1;
		}
	}

	fclose(f);
	return ok;	
}

char* findPassword(char* username, char* path)
{
	FILE *f = fopen(path,"r");
	char rd[1000] = "";
	char* psw = malloc(1000*sizeof(char));

	char usrnm[50]="";
	strcat(usrnm,"-");
	strcat(usrnm,username);
	strcat(usrnm,"\n");

	while(!feof(f))
	{
		fgets(rd,1000,f);
		if(!strcmp(rd,usrnm) && !feof(f))
		{
			fgets(psw,1000,f);
			if(!(strlen(psw)<=0 || !strcmp(psw,"\n")))
			{
				psw[strlen(psw)-1]='\0';
				return psw;
			}
		}
	}

	return NULL;
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

void loginUser(CLIENT *clients, int currentIndex,char* username)
{
	strcpy(clients[currentIndex].username,username);
	printf("Client %s connected!\n", clients[currentIndex].username);
	char sz[30] = "\\USRNM_VLD";//username valid	
	send(clients[currentIndex].socket,sz,strlen(sz),0);
}

int saveNewUser(char* username, char* passw, char* path)
{
	char s1[strlen(username)+1];
	char s2[strlen(passw)+1];

	s1[0]='\0';
	s2[0]='\0';

	strcat(s1,"-");
	strcat(s2,"+");
	
	strcat(s1,username);
	strcat(s2,passw);
	
	int noerr = 1;
	strcat(s1,"\n");
	strcat(s2,"\n\n");

	FILE *accounts = fopen(path,"a+");
	if(fputs(s1,accounts)==EOF || fputs(s2,accounts)==EOF)
		noerr = 0;

	fclose(accounts);

	return noerr;
}

void sendUsernameError(CLIENT* clients, int currentIndex)
{
	printf("Invalid username from client %d, asking again...\n",currentIndex);
	char sz[30] = "\\USRNM_N_VLD";//username not valid
	send(clients[currentIndex].socket,sz,strlen(sz),0);
					
}

void sendPasswordError(CLIENT* clients, int currentIndex)
{
	printf("Password not valid for client %d.\n",currentIndex);
	char sz[30] = "\\USRNM_N_VLD";//username not valid
	send(clients[currentIndex].socket,sz,strlen(sz),0);
					
}

void sendAlreadyOnlineError(CLIENT* clients, int currentIndex, char* insertedUsername)
{
	printf("User \"%s\" is trying to log in from another client!\n",insertedUsername);
	char sz[30] = "\\USRNM_N_VLD";//username not valid
	send(clients[currentIndex].socket,sz,strlen(sz),0);
}

void* updateClient(void* arg)
{
	
	while(1)
	{
		pthread_mutex_lock(&lock);
		updateStr *args = (updateStr*) arg;
		int recv_size;
		char msg[1000];
		char passw[1000];

		FILE* accounts;
		
		pthread_mutex_unlock(&lock);

		if((recv_size = recv(args->clients[args->currentIndex].socket,msg,1000,0)) != SOCKET_ERROR) //recieve from current and send to all other clients
		{
			pthread_mutex_lock(&lock);
			msg[recv_size]='\0';

			if(strlen(args->clients[args->currentIndex].username)==0)
			{
				//sign in procedure
				if(strContains(msg,"\\SGN_IN"))
				{
					char* token = strtok(msg,"*");
					token = strtok(NULL,"*");

					//validate username
					if(strlen(token)<=0 || strlen(token)>32  || isUsedUsername(token,accountPath))
					{
						sendUsernameError(args->clients,args->currentIndex);
					}
					else
					{
						//request password
						//not crypted yet
						printf("Listening\n");
						pthread_mutex_unlock(&lock);
						if((recv_size = recv(args->clients[args->currentIndex].socket,passw,1000,0)) != SOCKET_ERROR)
						{
							printf("Checking\n");
							pthread_mutex_lock(&lock);
							passw[recv_size]='\0';

							//write password to database
							if(strlen(passw)!=0)
							{

								if(!saveNewUser(token,passw,accountPath))
									printf("Error saving new user!\n");
								else
									loginUser(args->clients,args->currentIndex,token);
							}

						}
						else
						{
							pthread_mutex_lock(&lock);			
						}
						printf("Listened\n");
					}	
				}
				//login procedure
				if(strContains(msg,"\\LOG_IN"))
				{
					char* psw;
					char recpass[1000];
					char* token = strtok(msg,"*");
					token = strtok(NULL,"*");//contains right username
					if((psw = findPassword(token,accountPath))!=NULL)
					{
						pthread_mutex_unlock(&lock);
						if((recv_size = recv(args->clients[args->currentIndex].socket,recpass,1000,0)) != SOCKET_ERROR)
						{
							pthread_mutex_lock(&lock);
							recpass[recv_size]='\0';

							//password valid, login
							if(!isOnline(args->clients,token))
							{
								if(!strcmp(psw,recpass))
								{	
									loginUser(args->clients,args->currentIndex,token);
								}
								else
								{
									sendPasswordError(args->clients,args->currentIndex);
								}
							}
							else
							{
								sendAlreadyOnlineError(args->clients,args->currentIndex,token);
							}
							
						}
						else
						{
							pthread_mutex_lock(&lock);

							printf("Cannot recieve password\n");
						}

					}
					else
					{
						sendUsernameError(args->clients,args->currentIndex);
					}
				}
				
			}
			else
			{
				//sned normal message
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
			//client disconnected
			pthread_mutex_lock(&lock);
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

	createFirstFile(accountPath);

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