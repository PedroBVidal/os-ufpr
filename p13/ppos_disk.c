#include "ppos_disk.h"

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022



disk_t d;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize){

    sem_init(&(d.s), 1);
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0 , NULL);
    d.blocks_num = *numBlocks;
    d.block_size = *blockSize;
    d.free = 1;
    d.disk_q = NULL;
    d.sinal = 0;
    d.request_q = NULL; 
    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){
    
    request_disk_t* r;
    
    if (sem_down(&(d.s)) < 0){
        return -1;
    }        
    r = malloc(sizeof(request_disk_t));
    r->prev = NULL;
    r->next = NULL;
    r->op = READ_REQUEST;
    r->block = block;
    r->buffer = buffer;
    //r->task_request = task_exe;

    queue_append((queue_t **)&(d.request_q), (queue_t*) r);
    
    if (task_disk.status == SUSPENDED){
        //task_awake((task_t *)&task_disk, (task_t **) &(task_disk));
    }

    sem_up(&(d.s));
    
    task_suspend(&(d.disk_q));
    task_yield();
    return;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
   request_disk_t * r;
   
   if (sem_down(&d.s) < 0){
        return -1;
   }

   r = malloc(sizeof(request_disk_t));
   r->prev = NULL;
   r->next = NULL;
   r->block = block;
   r->buffer = buffer;
   r->op = WRITE_REQUEST;
   //r->task_request = task_exe;
    
    queue_append((queue_t **) &(d.request_q), (queue_t*) r);

    if (task_disk.status == SUSPENDED){
    } 
    
    sem_up(&(d.s));

    task_suspend(&(d.disk_q));
    task_yield();
}

void diskDriverBody (void * args)
{
   request_disk_t * r;
   while (1) 
   {
      if (d.sinal){
        d.sinal = 0;
        //task_awake(
        d.free = 1;
      }
        
      if (d.free && d.request_q != NULL){
        r = (request_disk_t *) queue_remove((queue_t **)&(d.request_q), (queue_t *) d.request_q);

        if (r->op == READ_REQUEST){
            disk_cmd(DISK_CMD_READ, r->block, r->buffer);
            d.free = 0;
        }
        else if (r->op == WRITE_REQUEST){
            disk_cmd(DISK_CMD_WRITE, r->block, r->buffer);
            d.free = 0;
        }

        sem_up(&(d.s));

    } 
  }
}

void diskSignalHandler() {
    #ifdef DEBUG
    printf("Sinal de disco recebido.\n");
    #endif
    d.sinal = 1;
}
