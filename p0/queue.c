#include<stdio.h>
#include<stdlib.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila
int queue_size(queue_t *queue){
  if (!queue){
    return 0;
  }

  queue_t *aux  = queue->next;
  int size  = 1;
  while (aux != queue){
    aux = aux->next;
    size = size + 1;
  }

  return size;  

}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){

  if (!queue){
    print_elem(queue);
  }
  
  queue_t *aux = queue;

  while(aux != queue){
    print_elem(aux);
    aux = aux->next;
  }
  return;
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem){

  if (elem->next != NULL || elem->prev != NULL){
    return -1;
  }

  if (!(*queue)){
    elem->next = elem;
    elem->prev = elem;
    (*queue) = elem;

    return 0;
  }

  queue_t *aux  = (*queue)->next;
  queue_t *prev = (*queue);


  while (aux != (*queue)){
    prev = aux;
    aux = aux->next;
  } 

  
  prev->next = elem;
  (*queue)->prev = elem;
  elem->prev = prev;
  elem->next = (*queue); 
   
  return 0;
}
  

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem){

  if (queue == NULL || *queue == NULL || elem == NULL)
  {
      return -1;
  }
   
  queue_t *aux  = (*queue)->next;
  queue_t *prev = (*queue);

  queue_t *frist_elem = (*queue);
  queue_t *last_element;

  
  int size = 1;
  int found_element = 0;

  // Testa caso de elemento unico
  if (elem == prev) { found_element = 1; }

  while (aux != (*queue)){
    size = size + 1;
    if (elem == aux){
      found_element = 1;
    }
    prev = aux;
    aux  = aux->next; 
  } 

  if (found_element == 0){
    return -1;
  }

  if (size == 1){
    elem->prev = NULL;
    elem->next = NULL;
    (*queue) = NULL;
    return 0;
  }

  // Remoção de cabeça
  if (elem == *(queue)){
    printf("remoção de cabeça \n ");
    elem->next->prev = elem->prev;
    // altered 
    elem->prev->next  = elem->next;
    (*queue) = elem->next;
    elem->prev = NULL;
    elem->next = NULL;
    return 0;
  }

  
  //Remoção da cauda 
  if (elem == (*queue)->prev){
    elem->prev->next = elem->next;
    // altered
    (*queue)->prev = elem->prev;
    elem->next = NULL;
    elem->prev = NULL;
    return 0;
  }

  // Remoção de elemento no meio da fila
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;  
  elem->next = NULL;
  elem->prev = NULL;
  return 0;
  
}



