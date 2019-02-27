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

   if( QisEmpty(&pid_q) ) {     // may occur if too many been created
      cons_printf("Panic: no more process!\n");
      breakpoint();                     // cannot continue, alternative: breakpoint();
   }

   pid = DeQ(&pid_q);								//grab next process in line
   Bzero( (char *)&pcb[pid], sizeof( pcb_t )); 		//clear the process control block of the selected process
   Bzero( (char *)&proc_stack[pid][0], PROC_STACK_SIZE); 	//clear the process stack of the selected process
   pcb[pid].state = READY;
   
   
   if(pid > 0)	//if a process
	   EnQ(pid, &ready_q);

	//point trapframe_p to stack & fill it out
   	pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE]; // point to stack top
   	pcb[pid].trapframe_p--;                   // lower by trapframe size
   	pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
   	pcb[pid].trapframe_p->cs = get_cs();                  // dupl from CPU
   	pcb[pid].trapframe_p->eip = p;                        // set next instruction pointer to the input function 
}


// count run_count and switch if hitting time slice
void TimerSR(void) {
   outportb(PIC_CONTROL, TIMER_DONE);                	// notify PIC timer done

   pcb[run_pid].run_count++;	//we just did another run, increment
   pcb[run_pid].total_count++;	//total count also goes up

   if(pcb[run_pid].run_count == TIME_SLICE) {
      pcb[run_pid].state = READY;
      EnQ(run_pid, &ready_q);
      run_pid = NONE;
   }
}
