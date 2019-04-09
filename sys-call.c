// sys-call.c
// calls to kernel services

#include "k-const.h"
#include <spede/stdio.h>
#include <spede/flames.h>
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"

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

int ForkCall(){
   int fork;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (fork)         // output
       : "g" (FORK_CALL)  // input
       : "eax"              // used registers
   );

   return fork;
}

int WaitCall(){
   int code;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (code)         // output
       : "g" (WAIT_CALL)  // input
       : "eax"              // used registers
   );

   return code;

}

void ExitCall(int code){
      asm("movl %0, %%eax;
           int %1"     // after, copy eax to variable 'pid'
       :         // output
       : "g" (code), "g" (EXIT_CALL)  // input
       : "eax"              // used registers
   ); //for now, but please adjust this into more asm 

}

void WriteCall(int device, char *str) {
    int row = GetPidCall();  //to set row number (need to run demo to find out how this is supposed to behave)
    int col = 0;  //column is set zero
	
	if (device == STDOUT) {	//if device is STDOUT 
    	while (*str != '\0') {	//while what str points to is not a null character 
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
		else if (device == TERM1_INTR)
			term_no = 1;
		
		//while what str points to is not a null character 
		while (*str != '\0') {
		  MuxOpCall(term[term_no].out_mux, LOCK);  	//lock the output mutex of the terminal interface data structure
		  EnQ(*str, &term[term_no].out_q);			//enqueue the character of the string to the output queue of the terminal interface data structure
		  
		  if (device == TERM0_INTR)					//if the device is TERM0_INTR, issue asm("int $35");
		    asm("int $35");		
		  else if (device == TERM1_INTR)							//otherwise, issue: asm("int $36");
			asm("int $36");			
			
		  str++;									//advance pointer 'str'
		  
		}
		
	}
	
}

void ReadCall(int device, char *str) {
	
	//determine which term_no (similar to how WriteCall did in the previous phase)
	int count = 0; //number of chars gathered so far = 0
	int term_no;
	
	if (device == TERM0_INTR)
		term_no = 0;
	else if (device == TERM1_INTR)
		term_no = 1;
	
	while (1) {
		char ch;
		MuxOpCall(term[term_no].in_mux, LOCK);  	//   A. lock the in_mux of the terminal
		ch = DeQ(&term[term_no].in_q);				//   B. dequeue a char from in_q of the terminal
		*str = ch;									//   C. set where the str points to to char
		if (ch == '\0') {							//   D. if char is NUL -> return
			return;
		}			
		str++;										//   E. advance both str pointer and char count
		count++;
		if (count == STR_SIZE) {					//   F. if char count is at the last available space of the given string:
			*str = '\0';								//      a. set where str points to to NUL
			return;									//      b. return
		}
	}
}
