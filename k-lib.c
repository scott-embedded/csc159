// k-lib.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes) {
	while (bytes--)
		*p++ = '\0';
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
   int i, ret;

   if(QisEmpty(p)) {
      cons_printf("Panic: queue is full, cannot EnQ!\n");
      return -1;
   }
   
   
   ret = p->q[0];	//set return value to from front of queue
   p->tail--;		//move tail back

   //shift everything to the front by 1
   for(i = 0; i < p->tail; i++)	
	   p->q[i] = p->q[i + 1];
   
   //clear out 'old' entries
   for(i = p->tail; i < Q_SIZE; i++)
	   p->q[i] = NONE;
   
   return ret;
}

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p) {
   if(QisFull(p)) {
      cons_printf("Panic: queue is full, cannot EnQ!\n");
      return;	
   }
   
   p->q[p->tail] = to_add;	//add item to current end
   p->tail++;	//increment tail position
}

