// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"

// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_q
void NewProcSR(func_p_t p) {  // arg: where process code starts
   int pid;

   if(QisEmpty(&pid_q)) {     // may occur if too many been created
      cons_printf("Panic: no more process!\n");
      breakpoint();                     // cannot continue, alternative: breakpoint();
   }

   pid = DeQ(&pid_q);
   Bzero((char *)&pcb[pid], sizeof(pcb_t));
   Bzero((char *)&proc_stack[pid][0], PROC_STACK_SIZE);
   pcb[pid].state = READY;
   if(pid > 0){
	   EnQ(pid, &ready_q);
   }

// point trapframe_p to stack & fill it out
   pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE];
   pcb[pid].trapframe_p--;
   pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
   pcb[pid].trapframe_p->cs = get_cs();                  // dupl from CPU
   pcb[pid].trapframe_p->eip =(int) p;                        // set to code
   
}

// count run_count and switch if hitting time slice
void TimerSR(void) {
   int i;
   int proc;
   outportb(PIC_CONTROL, TIMER_DONE);                              // notify PIC timer done SOH
   sys_centi_sec++;
   pcb[run_pid].run_count++;
   pcb[run_pid].total_count++;

   if(pcb[run_pid].run_count == TIME_SLICE) {
      pcb[run_pid].state = READY;
      EnQ(run_pid, &ready_q);
      run_pid = NONE;
   }

   if(!QisEmpty(&sleep_q)){
    for(i=0; i< sleep_q.tail; i++){
       proc = DeQ(&sleep_q);
       if(pcb[proc].wake_centi_sec <= sys_centi_sec){
         pcb[proc].state = READY;
         EnQ(proc, &ready_q);
       }
       else{
         EnQ(proc, &sleep_q);
      } 
    }
   }
}
int GetPidSr(void){
  return run_pid;
}

void ShowCharCallSr(int row, int col, char ch){
  unsigned short *p = VID_HOME;
  p+= row*80;
  p+= col;
  *p = ch + VID_MASK;
}

void SleepSr(int centi_sec){
  pcb[run_pid].state = SLEEP;
  pcb[run_pid].wake_centi_sec = sys_centi_sec + centi_sec;
  EnQ(run_pid, &sleep_q);
  run_pid=NONE;
}

int MuxCreateSR(int flag) {
   //allocates a mutex from the OS mutex queue, empty the mutex and set the flag and creater, and return the mutex ID via a register of the trapframe (similar to how GetPidSR() does).
   int current_mux = DeQ(mux_q); 	//allocates a mutex from the OS mutex queue
   Bzero((char *)&mux[current_mux], sizeof(mux_t)); //empty the mutex
   mux[current_mux].flag = flag;		//set the flag
   mux[current_mux].creater = GetPidSr(); 	//set the carrier
   return current_mux; //return the mutex ID
}

void MuxOpSR(int mux_id, int opcode) {
	if (opcode == LOCK) {
		if (mux[mux_id].flag > 0)
			mux[mux_id]--;	//decrement the flag in the mutex by 1 if it is greater than 0
		else {
			EnQ(run_pid, &mux[mux_id].mux_q);	//queue the PID of the calling process to a suspension queue in the mutex
			pcb[run_pid].state = SUSPEND;	//alter the process state to SUSPEND
			run_pid = NONE;	//reset the current running PID to NONE
		}
			
	}
	else if (opcode == UNLOCK) {
		if (QisEmpty(&mux[mux_id].suspend_q))	//if no suspended process in the suspension queue of the mutex
			mux[mux_id].flag++;					//increment the flag of the mutex by 1
		else {
			int released_pid = DeQ(&mux[mux_id].suspend_q));		//release the 1st PID in the suspension queue
			EnQ(released_pid, &ready_q);		//move it to the ready-to-run PID queue
			pcb[released_pid].state = READY;		//update its state
		}
			
	}
}
