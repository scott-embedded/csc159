// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"
#include "sys-call.h"


// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_q
void NewProcSR(func_p_t p) {  // arg: where process code starts
   int pid;

   if(QisEmpty(&pid_q)) {     // may occur if too many been created
      cons_printf("Panic: no more process!\n");
      breakpoint();                     // cannot continue, alternative: breakpoint();
	  return;
   }

   pid = DeQ(&pid_q);
   Bzero((char *)&pcb[pid], sizeof(pcb_t));
   Bzero((char *)&proc_stack[pid][0], PROC_STACK_SIZE);
   pcb[pid].state = READY;
   if(pid > 0){
	   EnQ(pid, &ready_q);
   }

// point trapframe_p to stack & fill it out
   pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE];	//Give the process a place to live on the stack
   pcb[pid].trapframe_p--;
   pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr by setting bit 9 of EFLAGS reg to 1 (Interrupt Enable Flag IF)
   pcb[pid].trapframe_p->cs = get_cs();                  // dupl from CPU
   pcb[pid].trapframe_p->eip = (int)p;                        // set to code
   
}

// count run_count and switch if hitting time slice
void TimerSR(void) {
   
   
   outportb(PIC_CONTROL, TIMER_DONE);                              // notify PIC timer done SOH
   sys_centi_sec++;
   pcb[run_pid].run_count++;
   pcb[run_pid].total_count++;

   if(pcb[run_pid].run_count == TIME_SLICE) {
      pcb[run_pid].state = READY;
      EnQ(run_pid, &ready_q);
      run_pid = NONE;
   }
   
   CheckWakeProc();

}

void CheckWakeProc(void) {
	int proc;
	int i;
	
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
   int current_mux = DeQ(&mux_q); 	//allocates a mutex from the OS mutex queue
   Bzero((char *)&mux[current_mux], sizeof(mux_t)); //empty the mutex
   mux[current_mux].flag = flag;		//set the flag
   mux[current_mux].creater = GetPidSr(); 	//set the carrier
   return current_mux; //return the mutex ID
}

void MuxOpSR(int mux_id, int opcode) {
	if (opcode == LOCK) {
		if (mux[mux_id].flag > 0)
			mux[mux_id].flag--;	//decrement the flag in the mutex by 1 if it is greater than 0
		else {
			EnQ(run_pid, &mux[mux_id].suspend_q);	//queue the PID of the calling process to a suspension queue in the mutex
			pcb[run_pid].state = SUSPEND;	//alter the process state to SUSPEND
			run_pid = NONE;	//reset the current running PID to NONE
		}
			
	}
	else if (opcode == UNLOCK) {
		if (QisEmpty(&mux[mux_id].suspend_q))	//if no suspended process in the suspension queue of the mutex
			mux[mux_id].flag++;					//increment the flag of the mutex by 1
		else {
			int released_pid = DeQ(&mux[mux_id].suspend_q);		//release the 1st PID in the suspension queue
			EnQ(released_pid, &ready_q);		//move it to the ready-to-run PID queue
			pcb[released_pid].state = READY;		//update its state
		}
			
	}
}

void TermSR(int term_no) {
	int event_type = inportb(term[term_no].io_base + IIR);		//read the type of event from IIR (2 in rs232.h) of the terminal port (IIR is Interrupt Indicator Register)
    if (event_type == TXRDY)  			//if it's TXRDY, call TermTxSR(term_no)
		TermTxSR(term_no);
    else if (event_type == RXRDY)    	//else if it's RXRDY, call TermRxSR(term_no) which does nothing but 'return;'
		TermRxSR(term_no);
    if (term[term_no].tx_missed == TRUE)						//if the tx_missed flag is TRUE, also call TermTxSR(term_no)
		TermTxSR(term_no);
	
}

void TermTxSR(int term_no) {
	char ch;
	if (QisEmpty(&term[term_no].out_q)&&QisEmpty(&term[term_no].echo_q)) { 			//if the out_q in terminal interface data structure is empty:
		term[term_no].tx_missed = TRUE;				//1. set the tx_missed flag to TRUE
		return;										//2. return
	}
	else {	
		if(!QisEmpty(&term[term_no].echo_q)){
      ch = DeQ(&term[term_no].echo_q);
    }
    else{
		  ch = DeQ(&term[term_no].out_q);
      MuxOpSR(term[term_no].out_mux, UNLOCK); 				//4. unlock the out_mux of the terminal interface data structure
    }
		//cons_printf("%c ", ch);
      	outportb(term[term_no].io_base + DATA, ch);		//2. use outportb() to send it to the DATA (0 in rs232.h) register of the terminal port N
      	term[term_no].tx_missed = FALSE;								//3. set the tx_missed flag to FALSE
		
	}
}

void TermRxSR(int term_no) {
	  char ch = inportb(term[term_no].io_base + DATA);		//read a char from the terminal io_base+DATA
      EnQ(ch, &term[term_no].echo_q);							//enqueue char to the terminal echo_q
      if (ch == '\r') {											//if char is CR -> also enqueue NL to the terminal echo_q
	  	EnQ('\n', &term[term_no].echo_q);
	  }
	  
      if (ch == '\r') {											//if char is CR -> enqueue NUL to the terminal in_q  
      	  EnQ('\0', &term[term_no].in_q);
      }												
      else	{													//else -> enqueue char to the terminal in_q
		  EnQ(ch, &term[term_no].in_q);
	  }
      MuxOpSR(term[term_no].in_mux, UNLOCK); 					//unlock the terminal in_mux
  }
