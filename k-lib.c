// k-lib.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes) {
	memset(p, 0, bytes);		//SOH
}

int QisEmpty(q_t *p) { // return 1 if empty, else 0
	if (p->tail == 0)
		return 1;
	else
		return 0;
}

int QisFull(q_t *p) { // return 1 if full, else 0
   if (p->tail == Q_SIZE) 	//SOH
	   return 1;
   else return 0;
}

// dequeue, 1st # in queue; if queue empty, return -1
// move rest to front by a notch, set empty spaces -1
int DeQ(q_t *p) { // return -1 if q[] is empty
   int i, ret;	//SOH

   if(QisEmpty(p)) {
      return -1;
   }
   
   ret = p->q[0];
   p->tail--;

   for (i = 0; i < Q_SIZE; i++) {
	   if (i < tail - 1)
		   p->q[i] = [i + 1];
	   else
		   p->q[i] = -1;
   }
   return ret;
}

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p) {
   if(QisFull(p)) {
      cons_printf("Panic: queue is full, cannot EnQ!\n");
      return;	//SOH
   }

   p->q[tail] = to_add;	//put
   p->tail++;
}

