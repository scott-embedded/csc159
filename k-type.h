// k-type.h, 159

#ifndef __K_TYPE__
#define __K_TYPE__

#include "k-const.h"

typedef void (*func_p_t)(void); // void-return function pointer type

typedef enum {UNUSED, READY, RUN, SLEEP, SUSPEND} state_t;  

   
typedef struct {
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

typedef struct {
	int flag;	//max number of processes
	int creater;	//requester/owning PID
	q_t suspend_q;	//suspended PID's
} mux_t;

typedef struct {
   	int tx_missed;   // when initialized or after output last char
    int io_base;     // terminal port I/O base #
    int out_mux;     // flow-control mux
   	q_t out_q;       // characters to send to terminal buffered here
} term_t;

#endif
