// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "tools.h"
#include "k-sr.h"
#include "sys-call.h"
#include "spede/stdio.h"
#include "proc.h"

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
   
   pcb[pid].main_table = kernel_main_table;                        
   
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
		outportb(term[term_no].io_base + DATA, ch);		//2. use outportb() to send it to the DATA (0 in rs232.h) register of the terminal port N
      	term[term_no].tx_missed = FALSE;								//3. set the tx_missed flag to FALSE
		
	}
}

void TermRxSR(int term_no) {
	  int device;
	  int process;
	  char ch = inportb(term[term_no].io_base + DATA);		//read a char from the terminal io_base+DATA
      EnQ(ch, &term[term_no].echo_q);							//enqueue char to the terminal echo_q
      if (ch == '\r') {											//if char is CR -> also enqueue NL to the terminal echo_q
	  	EnQ('\n', &term[term_no].echo_q);
	  }
	  
      if (ch == '\r') {//if char is CR -> enqueue NUL to the terminal in_q  
      	EnQ('\0', &term[term_no].in_q);
      }
      else{ //else -> enqueue char to the terminal in_q
		  EnQ(ch, &term[term_no].in_q);
      }
	  
	  //when all three below are true:
      //1. the input character is ASCII 3 (CTRL-C)
      //2. there is a process suspended by the in_mux of the terminal
      //3. the suspended process has a sigint_handler in its PCB
	  
	  if (ch == 3)  {	//CTRL+C is ascii 3 (1st condition)
		  if (QisEmpty(&mux[term[term_no].in_mux].suspend_q)) {
			  return; //2nd condition does not hold true, return	
		  }
		  
		  process = mux[term[term_no].in_mux].suspend_q.q[0];	
		  if (pcb[process].sigint_handler) {		//(check 3rd condition)
			  //WrapperSR is called with 3 arguments:
			  //a. the suspended pid (read it, but do not dequeued it from suspend_q)
			  //b. the sigint_handler of the pid
			  //c. the current device (TERM0_INTR/TERM1_INTR, depending on term_no)
			  device = term_no == 0 ? TERM0_INTR : TERM1_INTR;	//determine device 
			  WrapperSR(process, pcb[process].sigint_handler, device);
			  
		  }	  
	  }
	  								
	  
	  
      MuxOpSR(term[term_no].in_mux, UNLOCK); 					//unlock the terminal in_mux
}


int ForkSR(void) {
	
	int c_pid;
	int difference;
	int *p;
		
	if (QisEmpty(&pid_q)) {	  								//	a. if pid_q is empty -> 
		cons_printf("PANIC! pid_q is empty! :( \n");			//	1. prompt a Panic msg to PC, 
		return NONE;										//2. return NONE
	}
	
	c_pid = DeQ(&pid_q);      								//b. get a child PID from pid_q
	Bzero((char *)&pcb[c_pid], sizeof(pcb_t));      		//c. clear the child PCB
	pcb[c_pid].state = READY;      								//d. set the state in the child PCB to ... (READY)
	pcb[c_pid].ppid = run_pid;      						//e. set the ppid in the child PCB to ... (run_pid)
	EnQ(c_pid, &ready_q);      								//f. enqueue the child PID to ...

	difference = PROC_STACK_SIZE * (c_pid - run_pid);     									//g. get the difference between the locations of the child's stack and the parent's

	     					
	pcb[c_pid].trapframe_p = (trapframe_t*)( (int) pcb[pcb[ c_pid ].ppid].trapframe_p + difference);	//h. copy the parent's trapframe_p to the child's trapframe_p

	
	MemCpy((char*)&proc_stack[c_pid], (char*)&proc_stack[pcb[c_pid].ppid], PROC_STACK_SIZE);      				//i. copy the parent's proc stack to the child (use your own MemCpy())
	
	pcb[c_pid].trapframe_p->eax = 0;      													//j. set the eax in the new trapframe to 0 (child proc gets 0 from ForkCall)
	pcb[c_pid].trapframe_p->esp += difference;       //k. apply the location adjustment to esp, ebp, esi, edi in the new trapframe (nested base ptrs adjustment:)
	pcb[c_pid].trapframe_p->ebp += difference;
	pcb[c_pid].trapframe_p->esi += difference;
	pcb[c_pid].trapframe_p->edi += difference;
	p = (int*)pcb[c_pid].trapframe_p->ebp;     			//l. set an integer pointer p to ebp in the new trapframe

	while (*p != 0) {      					//m. while (what p points to is not 0) {
		*p += difference;					//      1. apply the location adjustment to the value pointed
	    p = (int*)*p;						//      2. set p to the adjusted value (need a typecast)
	}   

	return c_pid;      				//n. return child PID
}


int WaitSR(void) {
	
	int i;
	int match;
	int exit_code;
	int page_cnt;
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
		set_cr3(pcb[match].main_table);
		exit_code = pcb[match].trapframe_p->eax;		//get its exit code (from the eax of the child's trapframe)
		set_cr3(pcb[run_pid].main_table);				
														//reclaim child's resources: 
        pcb[match].state = UNUSED;						//1. alter its state to ... 
        EnQ(match, &pid_q);    							//2. enqueue its PID to ...
		page_cnt = 0;
	    for (i = 0; i < PAGE_NUM; i++) {
	    	if (page_user[i] == match) {
	    		page_user[i] = NONE;		//set page to current run_pid
				page_cnt++;
	    	}
		
			if (page_cnt == 2) {
				break;
			}	
	    } 
	}

    return exit_code;		//return the exit code

}


void ExitSR (int code) { 
  int page_cnt, i;
  int ppid = pcb[run_pid].ppid;
  set_cr3(kernel_main_table);
	  
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
  page_cnt = 0;
  for (i = 0; i < PAGE_NUM; i++) {
  	if (page_user[i] == run_pid) {
  		page_user[i] = NONE;		//set page to current run_pid
		page_cnt++;
  	}

	if (page_cnt == 2) {
		break;
	}	
  } 
 
}



void ExecSR(int code, int arg) {
    int i, j, pages[5], *p, entry, page_cnt;
	int code_page_addr;
	int stack_page_addr;
    trapframe_t *q;
    enum {MAIN_TABLE, CODE_TABLE, STACK_TABLE, CODE_PAGE, STACK_PAGE};
	
	
	//1. allocate 5 RAM pages by
	//loop page_user array, find each, put index in pages[5]
	//if not getting 5 indices -> show Panic msg, return
	//set page_user[] to so they're now used by run_pid
	//calculate their addresses (put in/re-use pages[] which had indices)	
	j = 0;
	page_cnt = 0;
    for (i = 0; i < PAGE_NUM; i++) {
		
    	if (page_user[i] == NONE) {
    		pages[j] = i;									
			page_user[i] = run_pid;			//set page to current run_pid
			j++;
			page_cnt++;
    	}
		
		if (page_cnt == 4) {
			break;	//we assigned 5 pages, break out of the loop
		}	
		
    }  	
	if (page_cnt < 4) {	//check to make sure we allocated 5 pages
		cons_printf("Panic: could not allocate 5 pages!\n");
		breakpoint();
	}
	
	
	//2.build code page (use addr already calculated in pages[CODE_PAGE])
	//   (call MemCpy to copy from code similar to as before)
	code_page_addr = pages[CODE_PAGE] * PAGE_SIZE + RAM;
	MemCpy((char*)code_page_addr, (char*)code, PAGE_SIZE);
	
	
	//3. build stack page (addr is pages[STACK_PAGE])
	//   A. Bzero it
	//   B. put arg on top (set p, lower it, and write to where it points)
	//   C. skip 4 bytes (not used, just lower p)
	//   D. build trapframe by
	//      a. set q to p (needs typecasting of course)
	//      b. lower q (by one whole trapframe_t space)
	//      c. set q->efl and q->cs as before
	//      d. but set q->eip to virtual addr, the constant M256
	stack_page_addr = pages[STACK_PAGE] * PAGE_SIZE + RAM;
    Bzero((char*)stack_page_addr, PAGE_SIZE);
	p =  (int*)(stack_page_addr - 1);
	*p = arg;
	p--;
	
	pcb[run_pid].trapframe_p = (trapframe_t*)( p );	//set TF to correct position
	pcb[run_pid].trapframe_p--;
	pcb[run_pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR;	//set EFLAGS
    pcb[run_pid].trapframe_p->cs = get_cs();                  //set CS
	pcb[run_pid].trapframe_p->eip = M256;      // set to code
	
	
	//4. build addr-trans main table
	//   A. Bzero it
	//   B. MemCpy from kernel_main_table to duplicate the 1st 4 entries (each size int)
	//   C. get entry # = leftmost 10 bits in virtual addr M256
	//   D. set the content of this entry to the addr of code table bitwise-OR-ed with
	//      the PRESENT and RW flags
	//   E. get entry # from leftmost 10 bits in virtual addr G1_1 (or V_TF, same)
	//   F. set the content of this entry to the addr of stack table bitwise-OR-ed with
	//      the PRESENT and RW flags
    Bzero((char*)&pages[MAIN_TABLE], PAGE_SIZE);
	entry = M256 & 0xFFC00000;	//preserve these in their original location?
	p = (int*)entry;
	*p = code_page_addr | PRESENT | RW;
	entry = G1_1 & 0xFFC00000; //preserve these in their original location?
	p = (int*)entry;
	*p = stack_page_addr | PRESENT | RW;
	
	//5. build code table (subtable)
	//   A. Bzero it
	//   B. get entry # = bits 11~20 from left (2nd 10 bits) in virtual addr M256 (use MASK10)
	//   C. set the content of this entry to the addr of code page bitwise-OR-ed with
	//      the PRESENT and RW flags
    Bzero((char*)&pages[CODE_TABLE], PAGE_SIZE);
	entry = M256 & MASK10;
	p = entry;
	*p = code_page_addr | PRESENT | RW;
	
	
	//6. build stack table (subtable)
	//   A. Bzero it
	//   B. get entry # = bits 11~20 from left (2nd 10 bits) in virtual addr G1_1 (use MASK10)
	//   C. set the content of this entry to the addr of stack page bitwise-OR-ed with
	//      the PRESENT and RW flags
    Bzero((char*)&pages[STACK_TABLE], PAGE_SIZE);
	entry = G1_1 & MASK10;
	p = entry;
	*p = stack_page_addr | PRESENT | RW;
	
	
	//7. set the main_table in the PCB of run_pid to the addr of the main table
	//   set the trapframe_p in the PCB of run_pid to V_TF (typecast of course)
	pcb[run_pid].main_table = pages[MAIN_TABLE];
	pcb[run_pid].trapframe_p = (trapframe_t*) V_TF;
	
}

/*
//Original ExecSR, leaving here for now until transition to new ExecSr is complete
void ExecSR(int code, int arg) {
	int i, page1, page2, page_cnt;
	char *code_page_addr;
	char *stack_page_addr;
		
	page_cnt = 0;	// track # of pages we assigned

    for (i = 0; i < PAGE_NUM; i++) {
    	if (page_user[i] == NONE) {
    		page_user[i] = run_pid;		//set page to current run_pid
			page_cnt++;
    	}
		
		if (page_cnt == 1) {
			page1 = i;	//remember the index of the page we set (code space)
		}	
		else if (page_cnt == 2) {	
			page2 = i;	//remember the index of the page we set (stack space)
			break;		//we assigned two pages, break out of for loop
		}
			
    }  		
	

	code_page_addr = (char*)(page1 * PAGE_SIZE + RAM);
	stack_page_addr = (char*)(page2 * PAGE_SIZE + RAM);

	MemCpy(code_page_addr, (char*)code, PAGE_SIZE);

    Bzero(stack_page_addr, PAGE_SIZE);
	
	stack_page_addr += PAGE_SIZE;	//move pointer to top of stack (sub 4 to be in last int position at top of our stack)
	stack_page_addr -= 4;
	*stack_page_addr = arg;			//copy arg over

	stack_page_addr -= 4;

	pcb[run_pid].trapframe_p = (trapframe_t*)( stack_page_addr);	//set TF to correct position
	pcb[run_pid].trapframe_p--;

	pcb[run_pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR;	//set EFLAGS
    pcb[run_pid].trapframe_p->cs = get_cs();                  //set CS
	
	pcb[run_pid].trapframe_p->eip = (int)code_page_addr;      // set to code
	
	
}*/
   
void SignalSR(int sig_num, int handler) {
    pcb[run_pid].sigint_handler = handler; //Just set sigint_handler in PCB of run_pid to handler.

}

void WrapperSR(int pid, int handler, int arg) {
   int *p;	//track our position
   trapframe_t temp_trap;
   temp_trap = *pcb[pid].trapframe_p;

   pcb[pid].trapframe_p->efl=arg;
   pcb[pid].trapframe_p->cs=handler;
   
   (int*)pcb[pid].trapframe_p -=3;

   *pcb[pid].trapframe_p = temp_trap;
   pcb[pid].trapframe_p->eip = (int) Wrapper;
}

void PauseSR(){
   pcb[run_pid].state = PAUSE;
   run_pid= NONE;  
}

void KillSR(int pid, int sig_num){
   int i;

   if(pid == 0){
      for(i = 0; i < PROC_SIZE; i++){
         if(pcb[i].ppid == run_pid && pcb[i].state == PAUSE){
	    pcb[i].state = READY;
            EnQ(i, &ready_q);
         }
      }
   }
}

int RandSR(){

   rand = run_pid * rand + A_PRIME;
   rand %= G2;
   return rand;
}   
