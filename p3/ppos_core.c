#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include <ucontext.h>

task_t main_task;
task_t *task_exe;
task_t dispatcher;
task_t *queue_ready;

#define READY 0
#define RUNNING 1
#define SUSPENDED 1

long nextId = 0;
void dispatcher_func(void* arg); 
task_t* scheduler();


#define STACKSIZE 64*1024

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
	setvbuf (stdout, 0, _IONBF, 0) ;

    main_task.next = NULL;
    main_task.prev = NULL;

    main_task.id = 0;

    nextId = 1;

    task_exe = &main_task; 

    task_init(&dispatcher,(void *)(dispatcher_func),"dispatcher");
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
    if (task_exe != &dispatcher){
        task_switch(&dispatcher);
    }
    else {
        task_switch(&main_task);
    }
    //task_switch(&main_task);
    return;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    ucontext_t *new_context = &(task->context);
    ucontext_t *context = &(task_exe->context);

 
    task_t *prev_task = task_exe;
    task_exe = task;
    
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
            task_switch(next_task);
        }

        q_size = queue_size((queue_t*) queue_ready); 
    }
    task_exit(0);
    return;
}

task_t* scheduler(){
    return queue_ready;
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
        queue_print("My queue", (queue_t *) queue_ready, print_elem);
        task_exe->status = READY;
    } 
    task_switch(&dispatcher);
}


