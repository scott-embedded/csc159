// sys-call.c
// calls to kernel services

#include "k-const.h"
#include <spede/stdio.h>
#include <spede/flames.h>
#include "k-data.h"
#include "k-lib.h"

int GetPidCall(void) {
   int pid;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (pid)         // output
       : "g" (GETPID_CALL)  // input
       : "eax"              // used registers
   );

   return pid;
}

void ShowCharCall(int row, int col, char ch) {
   asm("movl %0, %%eax;     // send in row via eax
        movl %1, %%ebx;            // send in col via ebx
        movl %2, %%ecx;            // send in ch via ecx
        int %3"             // initiate call, %3 gets entry_id
       :                    // no output
       : "g" (row), "g" (col), "g" (ch), "g" (SHOWCHAR_CALL)
       : "eax", "ebx", "ecx"         // affected/used registers
   );
}

void SleepCall(int centi_sec) {  // # of 1/100 of a second to sleep
      asm("movl %0, %%eax;
           int %1"     // after, copy eax to variable 'pid'
       :         // output
       : "g" (centi_sec), "g" (SLEEP_CALL)  // input
       : "eax"              // used registers
   ); //for now, but please adjust this into more asm 
}

//request/initialize a mutex
int MuxCreateCall(int flag) {	
	int output;
    asm("movl %1, %%eax; 	// after, copy eax to variable 'output'
         int %2;
		 movl %%eax, %0"     
     : "=g" (output)         // output  
     : "g" (flag), "g" (MUX_CREATE_CALL)  // input
     : "eax"              // used registers
 	); 
	return output;
}
	
//lock or unlock a mutex
void MuxOpCall(int mux_id, int opcode) {
    asm("movl %0, %%eax;
		movl %1, %%ebx;
         int %2"     // after, copy eax to variable 'pid'
     :         // output
     : "g" (mux_id), "g" (opcode), "g" (MUX_OP_CALL)  // input
     : "eax", "ebx"              // used registers
 	); 
}

void WriteCall(int device, char *str) {
    int row = GetPidCall();  //to set row number (need to run demo to find out how this is supposed to behave)
    int col = 0;  //column is set zero
	if (device == STDOUT) {	//if device is STDOUT {
    	while (*str != '\0') {	//while what str points to is not a null character {
          ShowCharCall(row, col, *str);   //Use an existing service call to show this character, at row and column
		  
          str++;   //increment the str pointer and the column position
		  col++;
		}
	}
	else {
		//set 'int term_no' to 0 or 1 depending on the given argument
		//'device' whether it is TERM0_INTR or TERM1_INTR
		int term_no;
		if (device == TERM0_INTR)
			term_no = 0;
		else 
			term_no = 1;
		
		//while what str points to is not a null character 
		while (*str != '\0') {
		  MuxOpCall(term[term_no].out_mux, LOCK);  	//lock the output mutex of the terminal interface data structure
		  breakpoint();
		  EnQ(*str, &term[term_no].out_q);			//enqueue the character of the string to the output queue of the terminal interface data structure
          
		  if (device == TERM0_INTR)					//if the device is TERM0_INTR, issue asm("int $35");
		    asm("int $35");		
		  else										//otherwise, issue: asm("int $36");
			asm("int $36");			
			
		  str++;									//advance pointer 'str'
		  
		}
		
	}
	
}
