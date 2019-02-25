// main.c, 159
// OS phase 1
// Hi!
// Team Name: Katana (Members: Nora Ali, Scott O'Hair, Tim Gates)

#include "k-include.h"  // SPEDE includes
#include "k-entry.h"    // entries to kernel (TimerEntry, etc.)
#include "k-type.h"     // kernel data types
#include "k-lib.h"      // small handy functions
#include "k-sr.h"       // kernel service routines
#include "proc.h"       // all user process code here

// kernel data are all here:
int run_pid;                        // current running PID; if -1, none selected
int sys_centi_sec;                  // system time in centi-sec, initialize it  CODING HINTS NMA
q_t pid_q, ready_q, sleep_q;        // sleeping proc PID's queued in here  CODING HINTS NMA
pcb_t pcb[PROC_SIZE];               // Process Control Blocks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE];   // process runtime stacks
struct i386_gate *intr_table;    // intr table's DRAM location


void InitKernelData(void) {         // init kernel data
   int i;
   int sys_centi_sec = 0;        //CODING HINTS NMA
      
   intr_table = get_idt_base();            // get intr table location

   Bzero( (char*)&pid_q, sizeof(q_t));                      // clear pid & ready queues 
   Bzero( (char*)&ready_q, sizeof(q_t));
   Bzero( (char*)&sleep_q, sizeof(q_t));    //clear sleep_q CODING HINTS NMA                  

   //Add all the processes to the pid queue
   for (i = 0; i < PROC_SIZE; i++)
	   EnQ(i, &pid_q);

   run_pid = NONE; 		//set run_pid to NONE
}

void InitKernelControl(void) {      // init kernel control

   fill_gate(&intr_table[TIMER_INTR], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0); //fill out intr table for timer
   fill_gate(&intr_table[GETPID_CALL], (int)GetPidEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[SHOWCHAR_CALL], (int)ShowCharEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[SLEEP_CALL], (int)SleepEntry, get_cs(), ACC_INTR_GATE, 0);
   outportb(PIC_MASK, MASK);                   // mask out PIC for timer
}

void Scheduler(void) {      // choose run_pid
   if (run_pid > 0)					//run_pid is greater than 0, just return; // OK/picked
     return;
   if (QisEmpty(&ready_q)) //is empty
      run_pid = 0;    // pick InitProc
   else {
      pcb[0].state = READY;  //change state of PID 0 to ready
      run_pid = DeQ(&ready_q);		//dequeue ready_q to set run_pid
  }
  
   pcb[run_pid].run_count = 0;                  // reset run_count of selected proc
   pcb[run_pid].state = RUN;                    // upgrade its state to run
}

int main(void) {    
	                      	// OS bootstraps
   InitKernelData();		//call to initialize kernel data
   InitKernelControl();		//call to initialize kernel control

   NewProcSR(InitProc);  // create InitProc
   Scheduler();
   Loader(pcb[run_pid].trapframe_p); // load/run it
   
   return 0; // statement never reached, compiler asks it for syntax
}

void Kernel(trapframe_t *trapframe_p) {           // kernel runs
   char ch;

   pcb[run_pid].trapframe_p = trapframe_p; // save it
   switch(trapframe_p->entry_id){
      case SLEEP_CALL:
         SleepSr(trapframe_p->eax);
         break;
      case GETPID_CALL:
         trapframe_p->eax = GetPidSr();
         break;
      case SHOWCHAR_CALL:
         ShowCharCallSr(trapframe_p->eax, trapframe_p->ebx, trapframe_p->ecx);
         break;
      case TIMER_INTR:
         TimerSR();
   }       
   //(kb_hit waits for keystroke and then returns its ASCII code, if not ASCII keeps waiting)
   if (cons_kbhit()) {            // check if keyboard pressed
      ch = cons_getchar();
      if (ch == 'b') {          // 'b' for breakpoint
		 breakpoint();  		// let's go to GDB
	  }
      if (ch == 'n') {           // 'n' for new process
      	NewProcSR(UserProc);     // create a UserProc
	}
	
   }
   Scheduler();    //may need to pick another proc 
   
   Loader(pcb[run_pid].trapframe_p); //load the selected proc
}

