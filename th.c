#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 

// A normal C function that is executed as a thread 
// when its name is specified in pthread_create() 
pthread_mutex_t lock;

typedef struct
{
	int a;
	int b;
}TRY;

void *myThreadFun(void *arg) 
{ 
	pthread_mutex_lock(&lock);

	TRY *s = (TRY*) arg;
	printf("th %d started\n",s->a);
	printf("%d\n",s->a+s->b);
	printf("th %d finished\n",s->a );

	pthread_mutex_unlock(&lock);
	return NULL; 
} 

int main() 
{ 
	 if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }


	TRY* s = malloc(sizeof(TRY));
	s->a=1;
	s->b=8;

	pthread_t tid[2]; 
	printf("Before Thread\n");
	pthread_create(&tid[0], NULL, myThreadFun, (void*) s); 

	s = malloc(sizeof(TRY));
	s->a=2;
	s->b=42;

	pthread_create(&tid[1], NULL, myThreadFun, (void*) s); 
	
	printf("After Thread\n"); 
	
	pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_mutex_destroy(&lock);
}
