// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use sys-calls

#include "k-const.h"   // LOOP
#include "sys-call.h"  // all service calls used below

void InitProc(void) {
   MuxCreateCall(1);	//initialize mutex flag value to 1
   int i;
   while(1) {
      ShowCharCall(0, 0, '.');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service

      ShowCharCall(0, 0, ' ');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service
   }
}

void UserProc(void) {
   int my_pid = GetPidCall();  // get my PID

   while(1) {
	  MuxOpCall(vid_mux, LOCK); 
      ShowCharCall(my_pid, 0, '0' + my_pid / 10);  // show my PID
      ShowCharCall(my_pid, 1, '0' + my_pid % 10);
	  MuxOpCall(vid_mux, UNLOCK); 
      SleepCall(500);                              // sleep .5 sec

	  MuxOpCall(vid_mux, LOCK); 
      ShowCharCall(my_pid, 0, ' ');                // erasure
      ShowCharCall(my_pid, 1, ' ');
	  MuxOpCall(vid_mux, UNLOCK); 
      SleepCall(500);
	  
   }
}
