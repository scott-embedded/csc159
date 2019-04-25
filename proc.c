// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use sys-calls

#include "k-const.h"   // LOOP
#include "sys-call.h"  // all service calls used below
#include "k-data.h"
#include "tools.h"
#include "k-include.h"
#include "proc.h"

void InitTerm(int term_no) {
   int i, j;

   Bzero((char *)&term[term_no].out_q, sizeof(q_t));
   Bzero((char *)&term[term_no].in_q, sizeof(q_t));      // <------------- new
   Bzero((char *)&term[term_no].echo_q, sizeof(q_t));    // <------------- new
   term[term_no].out_mux = MuxCreateCall(Q_SIZE);
   term[term_no].in_mux = MuxCreateCall(0);              // <------------- new

   outportb(term[term_no].io_base+CFCR, CFCR_DLAB);             // CFCR_DLAB is 0x80
   outportb(term[term_no].io_base+BAUDLO, LOBYTE(115200/9600)); // period of each of 9600 bauds
   outportb(term[term_no].io_base+BAUDHI, HIBYTE(115200/9600));
   outportb(term[term_no].io_base+CFCR, CFCR_PEVEN|CFCR_PENAB|CFCR_7BITS);

   outportb(term[term_no].io_base+IER, 0);
   outportb(term[term_no].io_base+MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");
   outportb(term[term_no].io_base+IER, IER_ERXRDY|IER_ETXRDY); // enable TX & RX intr
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");

   for(j=0; j<25; j++) {                           // clear screen, sort of
      outportb(term[term_no].io_base+DATA, '\n');
      for(i=0; i<LOOP/30; i++)asm("inb $0x80");
      outportb(term[term_no].io_base+DATA, '\r');
      for(i=0; i<LOOP/30; i++)asm("inb $0x80");
   }
/*  // uncomment this part for VM (Procomm logo needs a key pressed, here reads it off)
   inportb(term[term_no].io_base); // clear key cleared PROCOMM screen
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");
*/
}

void InitProc(void) {
   int i;

   vid_mux = MuxCreateCall(1);  // create/alloc a mutex, flag init 1

   InitTerm(0);
   InitTerm(1);

   while(1) {
      ShowCharCall(0, 0, '.');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");  // this can also be a kernel service

      ShowCharCall(0, 0, ' ');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");
   }
}

void UserProc(void) {
   int device;
   int child, exit, i;
   int my_pid = GetPidCall();  // get my PID

   char str1[STR_SIZE] = "PID    > ";         // <-------------------- new
   char str2[STR_SIZE];                       // <-------------------- new
   char str3[10] = "         ";
   char str4[STR_SIZE] = "CHILD ALPHABET x ARRIVES";

   str1[4] = '0' + my_pid / 10;  // show my PID
   str1[5] = '0' + my_pid % 10;

   device = my_pid % 2 == 1 ? TERM0_INTR : TERM1_INTR;
   //cons_printf("first calculation device = %i\n", device);

   SignalCall(SIGINT, Ouch);
   
   while(1) {
      WriteCall(device, str1);  // prompt for terminal input
      ReadCall(device, str2);   // read terminal input
      WriteCall(STDOUT, str2);  // show what input was to PC

      if(StrCmp(str2, "race") == FALSE){
        continue;
      }

      for(i=0; i < 5; i++){
	  
     	 child = ForkCall();
		  // Error occured, we could not fork
      	if (child == NONE) {
       	 WriteCall(STDOUT, "Couldn't fork!");
      	}
	  
		  // Are we a child?
      	else if (child == 0) {
		ExecCall((int)Aout, device);
		cons_printf("Exited ExecCall successfully.... (wtf?)");
        	//Aout(device);
      	}
	  
		  // Otherwise we are a parent
      	else {	
			Itoa(str3, child);					//Convert child PID to string & output
        	WriteCall(device, str3);	
        	WriteCall(device, "\n\r");
	}
     }

     SleepCall(300);
     KillCall(0,SIGGO);

     for(i=0;i<5;i++){	
        exit = WaitCall();					//Wait on child
		
		Bzero(str3, sizeof(str3));			//Exit code into string & output
	 	Itoa(str3, exit);
		WriteCall(device, str3);
        	WriteCall(device, "\n\r");
		str4[15] = exit/100+66;
		WriteCall(device, str4);
        	WriteCall(device, "\n\r");
		Bzero(str3, sizeof(str3));
     }
   }
}

void Aout(int device){
	
  int my_pid;
  int i;
  
  char str[] = "xx ( ) Hello, World!\n\r";
  my_pid = GetPidCall();


  str[0] = '0' + my_pid / 10;  // show my PID
  str[1] = '0' + my_pid % 10;
  str[4] = my_pid + 'A';

//  WriteCall(device, str);
  PauseCall();

  for (i = 0; i < 69; i++) {
    ShowCharCall(my_pid, i, my_pid + 'A');
    SleepCall(RandCall()%20+5);
    ShowCharCall(my_pid, i, ' ');
   }
   
  ExitCall(my_pid * 100);
  
}

void Ouch(int device) {
	WriteCall(device, "Can't touch that!\n\r");
}

void Wrapper(int handler, int arg) {   // args implanted in stack
	func_p_t2 func = (func_p_t2)handler;
	asm("pushal");                       // save regs
	func(arg);                           // call signal handler with arg
	asm("popal");                        // restore regs
	asm("movl %%ebp, %%esp; popl %%ebp; ret $8"::); // skip handler & arg
}
