// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use sys-calls


#include "k-const.h"   // LOOP
#include "sys-call.h"  // all service calls used below
#include "k-data.h"
#include "k-lib.h"
#include <spede/string.h> //for strcat

void InitProc(void) {
   int i;
   vid_mux = MuxCreateCall(1);	//initialize mutex flag value to 1
   while(1) {
      ShowCharCall(0, 0, '.');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service

      ShowCharCall(0, 0, ' ');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service
   }
}

void UserProc(void) {
   int my_pid = GetPidCall();  // get my PID
   
    //Build the string
   char output_arr[80];
   char blank[80];
   char ch[3];
   
   Bzero((char*)&output_arr, 80);
   Bzero((char*)&blank, 80);
   
   ch[0] = '0' + my_pid / 10;
   ch[1] = '0' + my_pid % 10;
   ch[2] = '\0';
   
   strcat(output_arr, "PID ");
   strcat(output_arr, ch);
   strcat(output_arr, " PROCESS IS RUNNING USING EXCLUSIVE ACCESS TO THE VIDEO DISPLAY");
     
   strcat(blank,      "                                                                     ");

   while(1) {
	  
	  MuxOpCall(vid_mux, LOCK); 
	  WriteCall(STDOUT, output_arr);
      SleepCall(50);                              // sleep .5 sec

	  WriteCall(STDOUT, blank);
	  MuxOpCall(vid_mux, UNLOCK); 
      SleepCall(50);
	  
   }
}
