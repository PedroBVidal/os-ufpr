#pragma once 
#include "disk.h"
//#include "ppos_data.h"
#include <stddef.h>
#include "ppos.h" 
#include <stdlib.h>

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__
#endif

#define READ_REQUEST 1
#define WRITE_REQUEST 0
// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.


typedef struct request_disk_t {
    struct request_disk_t* next;
    struct request_disk_t* prev;
    task_t* task_request;
    char op;
    int block;
    void* buffer;
} request_disk_t;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  // completar com os campos necessarios
  int blocks_num;
  int block_size;

  semaphore_t s;
    
  char sinal;
  char free;
  task_t * disk_q;
  request_disk_t* request_q;

} disk_t ;

#ifndef PPOS_DISK_H
#define PPOS_DISK_H
task_t task_disk;
// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

void diskDriverBody (void * args);

void diskSignalHandler();

#endif
