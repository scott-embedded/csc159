// k-type.h, 159

#ifndef __K_TYPE__
#define __K_TYPE__

#include "k-const.h"

typedef void (*func_p_t)(void); // void-return function pointer type

typedef enum {UNUSED, READY, RUN, SLEEP} state_t;  //CODING HINTS NMA

   
typedef struct {
   unsigned int reg[8];
   unsigned int edi; 
   unsigned int esi;
   unsigned int ebp;
   unsigned int esp;
   unsigned int ebx;
   unsigned int edx;
   unsigned int ecx;
   unsigned int eax;
   unsigned int entry_id;
   unsigned int eip;
   unsigned int cs;
   unsigned int efl;
} trapframe_t;

typedef struct {
   state_t state;                       // read in 1.html
   int run_count;
   int total_count;
   trapframe_t *trapframe_p;
   int wake_centi_sec;
} pcb_t;                     

typedef struct {             // generic queue type
  int q[Q_SIZE];             // for a simple queue
  int tail;
} q_t;

#endif
