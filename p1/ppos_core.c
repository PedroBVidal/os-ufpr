#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
    return;
}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init (task_t *task,			    // descritor da nova tarefa
               void  (*start_func)(void *),	// funcao corpo da tarefa
               void   *arg) {               // argumentos para a tarefa

               }			
// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {
    return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit (int exit_code) {
    return;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    return 0;
}
