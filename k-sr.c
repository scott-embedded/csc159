// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"
#include "sys-call.h"
#include "spede/stdio.h"


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


int ForkSR(void) {
	
	
	int c_pid;
	int difference;
	int *p;
		
	//cons_printf("ForkSR() hit\n");
	if (QisEmpty(&pid_q)) {	  								//	a. if pid_q is empty -> 
		cons_printf("PANIC! pid_q is empty! :( \n");			//	1. prompt a Panic msg to PC, 
		return NONE;										//2. return NONE
	}
	
	c_pid = DeQ(&pid_q);      								//b. get a child PID from pid_q
	//cons_printf("	...grabbed PID %i\n", c_pid);      													// (build a new PCB for the new child process:)
	Bzero((char *)&pcb[c_pid], sizeof(pcb_t));      		//c. clear the child PCB
	pcb[c_pid].state = READY;      								//d. set the state in the child PCB to ... (READY)
	pcb[c_pid].ppid = run_pid;      						//e. set the ppid in the child PCB to ... (run_pid)
	EnQ(c_pid, &ready_q);      								//f. enqueue the child PID to ...

	difference = PROC_STACK_SIZE * (c_pid - run_pid);     									//g. get the difference between the locations of the child's stack and the parent's
	
	
	
	cons_printf("difference =  %X\n", difference);
	cons_printf("pcb[run_pid].trapframe_p =  %X\n", pcb[run_pid].trapframe_p);
	
	
	//pcb[c_pid].trapframe_p = pcb[run_pid].trapframe_p + (difference / sizeof(trapframe_t));     					//h. copy the parent's trapframe_p to the child's trapframe_p
	//opt = (tcp_option_t *)((char*)opt+1);
	pcb[c_pid].trapframe_p = (trapframe_t*)((char*)pcb[run_pid].trapframe_p + difference);	     					//h. copy the parent's trapframe_p to the child's trapframe_p
	cons_printf("pcb[c_pid].trapframe_p =  %X\n", pcb[c_pid].trapframe_p);      																					//   (with the location adjustment applied to the new trapframe_p)
	
	
	
	
	memcpy(&proc_stack[c_pid][0], &proc_stack[run_pid][0], PROC_STACK_SIZE);      				//i. copy the parent's proc stack to the child (use your own MemCpy())
	//MemCpy(&proc_stack[c_pid], &proc_stack[run_pid], sizeof(proc_stack[c_pid]));      				//i. copy the parent's proc stack to the child (use your own MemCpy())
	pcb[c_pid].trapframe_p->eax = 0;      													//j. set the eax in the new trapframe to 0 (child proc gets 0 from ForkCall)
	//cons_printf("ebp before adding difference = %X\n", pcb[c_pid].trapframe_p->ebp);
	pcb[c_pid].trapframe_p->esp += difference;       				//k. apply the location adjustment to esp, ebp, esi, edi in the new trapframe (nested base ptrs adjustment:)
	pcb[c_pid].trapframe_p->ebp += difference;
	//cons_printf("ebp after adding difference = %X\n", pcb[c_pid].trapframe_p->ebp);
	pcb[c_pid].trapframe_p->esi += difference;
	pcb[c_pid].trapframe_p->edi += difference;
	p = pcb[c_pid].trapframe_p->ebp;     			//l. set an integer pointer p to ebp in the new trapframe
	//cons_printf("\npre while: *p = %X\n", *p);
	//cons_printf("pre while: p = %X\n", p);
	//cons_printf("About to enter while loop...\n");
	while (*p != 0) {      					//m. while (what p points to is not 0) {
		*p -= difference;					//      1. apply the location adjustment to the value pointed
		cons_printf("\n*p = %X\n", *p);
	    p = (int*)*p;						//      2. set p to the adjusted value (need a typecast)
		cons_printf("p = %X\n", p);
	}   
	//cons_printf("Left...\n");   				
	breakpoint();
	return c_pid;      				//n. return child PID
}


int WaitSR(void) {
	
	int i;
	int match;
	int exit_code;
	match = -1;
	
	for (i = 0; i < PROC_SIZE; i++) {								// loop thru the PCB array (looking for a ZOMBIE child):
		if (pcb[i].ppid == run_pid && pcb[i].state == ZOMBIE) {		// the proc pid is run_pid and the state is ZOMBIE -> break the loop
			match = i;
			break; 
		}
	}
														// if the whole PCB array was looped thru (didn't find any ZOMBIE child):
	if (match < 0) {									//no hit found, else match will contain PID of match
    	pcb[run_pid].state = WAIT;        				//1. alter the state of run_pid to ... (WAIT)
    	run_pid = NONE;        							//2. set run_pid to ... (NONE)
    	return NONE;        							//3. return NONE		
	}
	
	else {
		exit_code = pcb[match].trapframe_p->eax;		//get its exit code (from the eax of the child's trapframe)
														//reclaim child's resources: 
        pcb[match].state = UNUSED;						//1. alter its state to ... 
        EnQ(match, &pid_q);    							//2. enqueue its PID to ...
	}

    return exit_code;		//return the exit code

}


void ExitSR (int code) { 
  int ppid = pcb[run_pid].ppid;
  if (pcb[ppid].state != WAIT) {			//if the process state of my parent (ppid) is not WAIT:
    pcb[run_pid].state = ZOMBIE;			//1. alter my state to ...
    run_pid = NONE;							//2. reset run_pid to ...
    return;									//3. return
  }

  pcb[ppid].state = READY;					//3. return
  EnQ(ppid, &ready_q);						//enqueue ppid to ...
  pcb[ppid].trapframe_p->eax = code;		//don't forget to pass it the exit code (via eax of parent's trapframe)
											//reclaim child's resources:
  pcb[run_pid].state = UNUSED;				//1. alter its state to ...
  EnQ(run_pid, &pid_q);						//2. enqueue its PID to ...
  run_pid = NONE;							//3. reset run_pid to ...
 
}

