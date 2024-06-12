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
task_t *sleep_tasks;

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
int inter_enabled = 1;

typedef struct filaint_t
{
   struct filaint_t *prev ;  // ptr para usar cast com queue_t
   struct filaint_t *next ;  // ptr para usar cast com queue_t
   int id ;
   // outros campos podem ser acrescidos aqui
} filaint_t ;

// imprime na tela um elemento da fila (chamada pela função queue_print)
void print_elem (void *ptr)
{
	//printf("printing elem \n");
   filaint_t *elem = ptr ;

   if (!elem){
	printf("nao elem :( \n");
      return ;
   }

   elem->prev ? printf ("%d", elem->prev->id) : printf ("*") ;
   printf ("<%d>", elem->id) ;
   elem->next ? printf ("%d", elem->next->id) : printf ("*") ;
}

unsigned int systime (){
	return clock_sys;
}

void tratador_tick(){
	//printf("TRATADOR TICKS \n");
    	int q_size = queue_size( (queue_t*) queue_ready);
    	int sleep_tasks_size = queue_size ((queue_t*) sleep_tasks);

    	//printf("sleep_tasks_size %d ", sleep_tasks_size);
    	//printf("q size %d \n ", q_size);
	//printf("task exe id %d \n" , task_exe->id);
	
	if (!inter_enabled) return;

	clock_sys += ITTIMER_U/1000;
	task_exe->processor_time += ITTIMER_U/1000;
	if (quantum < 1 && task_exe->id != 1){
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
	
//main_task = malloc(sizeof(task_t));
   // task_init(&main_task,NULL,0);


    main_task.next = NULL;
    main_task.prev = NULL;

    main_task.id = 0;
    main_task.tasks_waiting = NULL;
    main_task.status = READY;
    main_task.cont_waiting_join = 0;

      
    main_task.time_ini = systime();
    main_task.activations = 0;
    main_task.prio_s = 0;
    main_task.prio_d = 0;
    main_task.exit_code = -1;

    queue_append((queue_t**)&queue_ready, (queue_t*)&main_task);
    main_task.queue = (queue_t**) &queue_ready;

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
    task->status = READY;
    task->prio_d = 0;
    task->prio_s = 0;
    task->time_ini = systime();
    task->activations = 0;
    task->tasks_waiting = NULL;
    task->cont_waiting_join = 0;
    task->exit_code = -1;

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
    task_exe->exit_code = task_exe->id;

    task_t *aux = task_exe->tasks_waiting;
    printf("cont waiting join task_exe %d \n ", task_exe->cont_waiting_join); 
    for (int i = 0; i < task_exe->cont_waiting_join; i++){
        queue_remove((queue_t **) &(task_exe->tasks_waiting), (queue_t *) aux);
        aux->status = READY;
        printf("putting task %d again to the queue ready \n", aux->id);
        queue_append((queue_t **) &queue_ready, (queue_t *) aux);
	// Next task that it's waiting for join 
	aux = task_exe->tasks_waiting;
    }

    int sleep_tasks_size = queue_size ((queue_t*) sleep_tasks);
    int queue_ready_size = queue_size ((queue_t*) queue_ready);
    queue_print("queue after exit ready", (queue_t *) queue_ready,print_elem) ;

    printf("sleep q size %d ready q size %d \n", sleep_tasks_size, queue_ready_size);

    if (task_id >= 0){
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
void helper_suspended(){
	//printf("entering in handle .suspended \n ");
	if (!sleep_tasks){
		return;
	}

	inter_enabled = 0;

	task_t *aux = sleep_tasks;
	int s = queue_size((queue_t *) sleep_tasks);

	for (int i = 0; i < s; i++){
        int q_size = queue_size( (queue_t*) queue_ready);
		if (aux->wake_at <= systime()){
			queue_remove ((queue_t **) &sleep_tasks, (queue_t *) aux);
			queue_append ((queue_t **) &queue_ready, (queue_t *) aux);
			if (!sleep_tasks){
				inter_enabled = 1;
				return;
			}

			aux = sleep_tasks;
		}
		aux = aux->next;
	}

	inter_enabled = 1;
    int sleep_tasks_size = queue_size ((queue_t*) sleep_tasks);

    //printf("sleep_tasks_size %d \n", sleep_tasks_size);

}

void dispatcher_func(void* arg){
    int q_size = queue_size( (queue_t*) queue_ready);
    int sleep_tasks_size = queue_size ((queue_t*) sleep_tasks);

    printf("sleep_tasks_size %d ", sleep_tasks_size);
    printf("q size %d \n ", q_size);
    while (q_size != 0 || sleep_tasks_size != 0){
	helper_suspended();
	
        task_t *next_task = scheduler();

        if (next_task != NULL){
            queue_remove((queue_t **) &queue_ready, (queue_t *) next_task);

	    quantum = QUANTUM;
            task_switch(next_task);
        }

        q_size = queue_size((queue_t*) queue_ready); 
        sleep_tasks_size = queue_size ((queue_t*) sleep_tasks);

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

void task_yield(){
    if (task_exe->status != SUSPENDED){
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

	// Remove task_exe of the ready queue 
	
	if (queue != NULL){
  	    queue_print("queue BEFORE SUSPENSION \n ", (queue_t *) *queue,print_elem) ; 
			int result = queue_remove((queue_t **) &queue_ready, (queue_t *) task_exe);
			task_exe->status = SUSPENDED;
			queue_append((queue_t **) queue, (queue_t*) task_exe);
            printf("suspendig task id = %d \n ", task_exe->id);
	    queue_print("queue AFTER SUSPENSION \n ", (queue_t *) *queue,print_elem) ; 

            //printf("queue ready size : %d \n ", s);
	}
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
int task_wait(task_t *task){
    
	// Suspende tarefa atual e a insere na lista de espera de task	

    int s = queue_size((queue_t *) queue_ready);

    if (s > 0 && task->exit_code == -1){
	    
	    printf("task that has someone waiting to finish %d \n", task->id);
	    task_suspend (&(task->tasks_waiting));
	    task->cont_waiting_join++;
    }
	task_yield();

	return task->id;	
    
    
}

void task_sleep (int time){

	
	inter_enabled = 0;
	if (time <= 0){
		inter_enabled = 1;
		return;
	}

	task_exe->wake_at = systime()+time;

	queue_remove((queue_t **) &queue_ready, (queue_t *) task_exe);
	queue_append((queue_t **) &sleep_tasks, (queue_t *) task_exe);

	inter_enabled = 1;
	task_yield();
	
}
/*
int sem_init (semaphore_t *s, int value){
	if (s == NULL){
		return -1;
	}

	inter_enabled = 0;
	s->value = value;	
	s->queue = NULL;
	s->valid = 1;

	inter_enabled = 1;
	return 0;
}

int sem_down (semaphore_t *s){
	if (s == NULL || !s->valid){
		return -1;
	}

	inter_enabled = 0;
	s->value = s->value - 1;
	if (s->value < 0){
		task_suspend(&(s->queue));	
	}
	inter_enabled = 1;
	task_yield();

	if (s->valid){
		return 0;
	}
	else {
		return -1;
	}

	return 0;	
}

int sem_up (semaphore_t *s){
	if (s == NULL || !s->valid){
		return -1;
	}
		
	inter_enabled = 0;
	s->value++;
	if (s->queue){
		task_awake ((task_t *) s->queue, (task_t **) &(s->queue));	
	}

	inter_enabled = 1;
	return 0;		
}

int sem_destroy (semaphore_t *s) {
	if (s == NULL || !s->valid){
		return -1;
	}

	inter_enabled = 0;
	while (s->queue){
		task_awake ((task_t *) s->queue, (task_t **) &(s->queue));	
	}

	s->valid = 0;
	inter_enabled = 1;
	return 0;
}
*/

int sem_init(semaphore_t* s, int value) {
	
    if (s == NULL) {
        return -1;
    }
    
    inter_enabled = 0; // Impede preempção
    s->queue = NULL;
    s->value = value;
    s->active = 1;

#ifdef DEBUG
    printf("sem_create: criado semaforo com valor inicial %d.\n", value);
#endif

    inter_enabled = 1; // Retoma preempção
    if(quantum <= 0) {
        task_yield();
    }

    return 0;
}

int sem_down(semaphore_t* s) {
    if (s == NULL || !(s->active)) {
        return -1;
    }

    inter_enabled = 0; // Impede preempção
    s->value--;
    if (s->value < 0) {
#ifdef DEBUG
        printf("sem_down: semaforo cheio, suspendendo tarefa %d\n", task_exe->id);
#endif
        // Caso não existam mais vagas no semáforo, suspende a tarefa.
        //task_suspend(&(s->queue));
	queue_remove((queue_t **) &queue_ready, (queue_t *) task_exe);
	task_exe->status = SUSPENDED;
	queue_append((queue_t **) &(s->queue), (queue_t*) task_exe);

        inter_enabled = 1; // Retoma preempção
        task_yield();

        // Se a tarefa foi acordada devido a um sem_destroy, retorna -1.
        if (!(s->active)) {
            return -1;
        }

#ifdef DEBUG
        printf("sem_down value < 0: semaforo obtido pela tarefa %d\n", task_exe->id);
	queue_print("queue ready after suspension ", (queue_t *) queue_ready,print_elem) ;

#endif
        return 0;
    }

#ifdef DEBUG
    printf("sem_down value > 0: semaforo obtido pela tarefa %d\n", task_exe->id);
    queue_print("queue ready after suspension", (queue_t *) queue_ready,print_elem) ;
#endif

    inter_enabled = 1; // Retoma preempção
    if(quantum <= 0) {
	printf("exiting due to quantum");
	printf("quantum <=0 \n");
        task_yield();
    }
    return 0;
}

int sem_up(semaphore_t* s) {
    if (s == NULL || !(s->active)) {
        return -1;
    }
    
    inter_enabled = 0; // Impede preempção
    s->value++;
    if (s->value <= 0) {
	//task_resume(s->queue);	
	#ifdef DEBUG
	//printf("awaking the task with id %d\n", s->queue->id);
    	queue_print("queue after awaking ", (queue_t *) s->queue,print_elem) ;
	queue_print("queue ready after awaking ", (queue_t *) queue_ready,print_elem) ;
	printf("task exe id %d \n", task_exe->id );
	#endif
	task_awake ((task_t *) s->queue, (task_t **) &(s->queue));	

    }
    inter_enabled = 1; // Retoma preempção

#ifdef DEBUG
    printf("sem_up: semaforo liberado pela tarefa %d\n", task_exe->id);
#endif
    if(quantum <= 0) {
        task_yield();
    }
    return 0;
}

int sem_destroy(semaphore_t* s) {
    if (s == NULL || !(s->active)) {
        return -1;
    }
    
    inter_enabled = 0; // Impede preempção
    s->active = 0;
    while (s->queue != NULL) {
	//task_resume(s->queue);
	task_awake ((task_t *) s->queue, (task_t **) &(s->queue));	

    }

    inter_enabled = 1; // Retoma preempção
    if(quantum <= 0) {
        task_yield();
    }
    return 0;
}

