// k-lib.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes) {
   ...
}

int QisEmpty(q_t *p) { // return 1 if empty, else 0
   return !p->tail;
}

int QisFull(q_t *p) { // return 1 if full, else 0
   return p->tail == Q_SIZE; 
}

// dequeue, 1st # in queue; if queue empty, return -1
// move rest to front by a notch, set empty spaces -1
int DeQ(q_t *p) { // return -1 if q[] is empty
   int ..., ...

   if(QisEmpty(p)) {
      cons_printf("Panic: queue is full, cannot EnQ!\n");
      return -1; //breakpoint ();
   }

   to_return = p->q[0];
   p->tail--;
   for(i=0; i<p->tail; i++)p->q[i] = p->q[i+1];
   
   //for(i=tail; i<Q_SIZE; i++)p->q[i] = NONE;
   p->q[p->tail] = -1;
   return to_return;
   ...
   ...
}

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p) {
   if(QisFull(p)) {
      cons_printf("Panic: queue is full, cannot EnQ!\n");
      return; //breakpoint();
   }

   p->q[tail] = to_add;
   p->tail++;
   //p->q[tail] = NONE;  good idea 
}



