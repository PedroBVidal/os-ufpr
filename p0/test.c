#include<stdlib.h>
#include<stdio.h>
#include "queue.h"


int main (){

  int r = 2;

  typedef struct filaint_t
  {
     struct filaint_t *prev ;  
     struct filaint_t *next ;  
     int id ;
     // outros campos podem ser acrescidos aqui
  } filaint_t ;

  filaint_t *fila0;
  
  fila0 = NULL;
  r = queue_size((queue_t*) fila0); 
  
  printf("queue size return %d \n ", r);
  
}
