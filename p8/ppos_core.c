#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

task_t main_task;
task_t *task_exe;
task_t dispatcher;
task_t *queue_ready;
task_t *queue_suspend;

#define READY 0
#define RUNNING 1
#define SUSPENDED 1
#define QUANTUM 20
#define ITTIMER_U 1000


long nextId = 0;
void dispatcher_func(void* arg); 
task_t* scheduler();

struct itimerval timer;
struct sigaction action;
int quantum;
#define STACKSIZE 64*1024
int clock_sys = 0;

unsigned int systime (){
	return clock_sys;
}

void tratador_tick(){
	clock_sys += ITTIMER_U/1000;
	task_exe->processor_time += ITTIMER_U/1000;
	if(quantum < 1){
		task_yield();
	}
	else {
		quantum--;
	}
	return;
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
    setvbuf (stdout, 0, _IONBF, 0) ;

    main_task.next = NULL;
    main_task.prev = NULL;

    main_task.id = 0;

    nextId = 1;

    task_exe = &main_task; 

    task_init(&dispatcher,(void *)(dispatcher_func),"dispatcher");

    action.sa_handler = tratador_tick ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
    	perror ("Erro em sigaction: ") ;
    	exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = ITTIMER_U ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = ITTIMER_U ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
    	perror ("Erro em setitimer: ") ;
    	exit (1) ;
    }
    return;

}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init (task_t *task,			    // descritor da nova tarefa
               void  (*start_func)(void *),	// funcao corpo da tarefa
               void   *arg) {               // argumentos para a tarefa
	
    ucontext_t *context;
    context = &(task->context);   		   
    char *stack = malloc (STACKSIZE) ;

    task->next = NULL;
    task->prev = NULL;

    getcontext(&(task->context));

    if (stack)
    {
        context->uc_stack.ss_sp = stack ;
        context->uc_stack.ss_size = STACKSIZE ;
        context->uc_stack.ss_flags = 0 ;
        context->uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        exit (1) ;
    }

    task->id = nextId;
    nextId++;
    makecontext(context, (void(*)(void)) start_func, 1, arg);

    //task_exe = task;

    task->prio_d = 0;
    task->prio_s = 0;
    task->time_ini = systime();
    task->activations = 0;

    if (task->id > 1){
        queue_append((queue_t **)&queue_ready, (queue_t*) task);
        task->queue = (queue_t **)&queue_ready;
    }

    #ifdef DEBUG
    printf ("task_init: iniciada tarefa %d\n", task->id);
    #endif

    return task->id;	

    }			

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {
    return task_exe->id;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit (int exit_code) {
    #ifdef DEBUG
    printf("Encerrei tarefa %d \n", task_exe->id);
    #endif
    unsigned int time_fin = systime();
    int time_spent = time_fin - task_exe->time_ini;
    int task_id = task_exe->id;
    int activations = task_exe->activations; 
    int processor_time = task_exe->processor_time;
    if (task_id > 0){
    	printf("Task %d exit: execution time %d ms, processor time %d , %d activations \n", task_exe->id,time_spent,processor_time,activations);
    }
    if (task_exe != &dispatcher){
        task_switch(&dispatcher);
    }
    else {
        task_switch(&main_task);
    }
 
    return;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    //ucontext_t *new_context = &(task->context);
    //ucontext_t *context = &(task_exe->context);

 
    task_t *prev_task = task_exe;
    task_exe = task;
    
    task->activations += 1;
    if (swapcontext(&(prev_task->context),&(task->context)) < 0){
        task_exe = prev_task;   
        perror("Erro troca de contexto");
        return -1; 
    }

    return 0; 
}

void dispatcher_func(void* arg){
    int q_size = queue_size( (queue_t*) queue_ready);

    while (q_size != 0){
        task_t *next_task = scheduler();

        if (next_task != NULL){
            queue_remove((queue_t **) &queue_ready, (queue_t *) next_task);

	    quantum = QUANTUM;
            task_switch(next_task);
        }

        q_size = queue_size((queue_t*) queue_ready); 
    }
    task_exit(0);
    return;
}

task_t* scheduler(){
    if (queue_ready == NULL){
        return NULL;
    }
    if (queue_ready->next == queue_ready){
        return queue_ready;
    }

    task_t *task_prio = queue_ready;
    task_prio->prio_d = task_prio->prio_d -1;
    task_t *task_aux = queue_ready->next;
    while(task_aux != task_prio){
        // lowest priority exit first

        if (task_aux->prio_d > -20){
            task_aux->prio_d = task_aux->prio_d - 1;
        }

        int lowest_priority = task_prio->prio_d;
        if (lowest_priority > task_aux->prio_d){
            task_prio = task_aux;
            lowest_priority = task_aux->prio_d; 
        }
        task_aux = task_aux->next;
    }

    task_prio->prio_d = task_prio->prio_s;

    return task_prio;
}

void print_elem (void *ptr)
{
   task_t *elem = ptr ;

   if (!elem)
      return ;

   elem->prev ? printf ("%d", elem->prev->id) : printf ("*") ;
   printf ("<%d>", elem->id) ;
   elem->next ? printf ("%d", elem->next->id) : printf ("*") ;
}

void task_yield(){
    if (task_exe->id > 1 ){
        queue_append((queue_t **) &queue_ready, (queue_t *) task_exe);
        //queue_print("My queue", (queue_t *) queue_ready, print_elem);
        task_exe->status = READY;
    } 
    task_switch(&dispatcher);
}

void task_setprio (task_t *task, int prio){
    if (prio < -20 || prio > 20){
        return;
    }

    if (task == NULL){
        task_exe->prio_d = prio;
        task_exe->prio_s = prio;
    }

    else {
        task->prio_d = prio;
        task->prio_s = prio;
    }
    return; 
}

int task_getprio (task_t *task){

    if (task == NULL){
        return task_exe->prio_s;
    }
    else {
        return task->prio_s;
    }
}

void task_suspend (task_t **queue){
	int result = queue_remove((queue_t **) &queue_ready, (queue_t *) task_exe);
	
	task_exe->status = SUSPENDED;
	if (queue != NULL){
		queue_append((queue_t **) queue, (queue_t*) task_exe);
	}
	task_switch(&dispatcher);	
}

void task_awake (task_t *task, task_t **queue){
	// remove da lista de suspensas
	if (queue != NULL){
		int result = queue_remove((queue_t **) queue, (queue_t *) task);
	}
	task->status = READY;
	// Muda status para pronto e insere na fila de prontos
	if (queue != NULL){
		queue_append((queue_t **) &queue_ready, (queue_t*) task);
	}
}

void task_wait(task_t *task){
	if (task != NULL){

	}
	// Suspende tarefa atual 
	void task_suspend (&queue_suspend);	
}
