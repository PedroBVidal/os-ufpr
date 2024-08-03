#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"


#define TAM 5
#define NUM_PROD 3
#define NUM_CONS 2
#define WORKLOAD 20000

typedef struct buff {
	int a [TAM];
	int i;
} buff;

void buffer_init (buff *b){
	b->i = 0;
}

void buffer_insert (buff *b, int value){
	if (b->i == TAM){
		return;
	}

	b->a[b->i] = value;
	b->i++;
}

int buffer_remove (buff *b){
	if (b->i == 0){ 
		return 0; 
	}
	//int index = b->i-1;
	int removed_value = b->a[0];

	for (int j = 1; j < b->i; j++){
		b->a[j-1] = b->a[j];
	}

	b->i--;
	return removed_value;
}

task_t prod [NUM_PROD];
task_t cons [NUM_CONS];

semaphore_t s_buffer;
semaphore_t s_vaga;
semaphore_t s_item;

buff b;


void producer (void *id){
	int *id_value = (int *) id;
	

	while (1){
		task_sleep(1000);
		int item = random() % 100;
		
		sem_down (&s_vaga);
		sem_down (&s_buffer);
		
		buffer_insert(&b,item);
		
		sem_up(&s_buffer);
		sem_up(&s_item);	
		
		printf("p%d produziu %d \n", *id_value,item);

	}
	task_exit(0);
}


void consumer (void *id){
	int *id_value = (int *) id;


	while (1){
		sem_down (&s_item);
		sem_down (&s_buffer);
		
		int removed_value = buffer_remove(&b);

		sem_up(&s_buffer);
		sem_up(&s_vaga);
		
		printf("		c%d consumiu %d \n", *id_value,removed_value);
		task_sleep(1000);
	}
	task_exit(0);
}


int main (int argc, char *argv[]) {
	int i;
	ppos_init () ;
	
	sem_init(&s_buffer, 1);
	sem_init(&s_item, 0);
	sem_init(&s_vaga, 5);
	
	buffer_init (&b);
	
	
	for (int i = 0; i < NUM_PROD; i++){
		printf("incialzing %d \n", i);
		task_init(&(prod[i]), producer, &(prod[i].id));
	}
	for (int i = 0; i < NUM_CONS; i++){
		task_init(&(cons[i]), consumer, &(cons[i].id));
	}

	task_exit(0);	
	exit(0);	

}

