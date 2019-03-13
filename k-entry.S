#include "k-const.h"
#include <spede/machine/asmacros.h>
#define K_STACK_SIZE (PROC_STACK_SIZE *8)

.comm k_stack, K_STACK_SIZE, 1
.text

// load proc trapframe to run it
ENTRY(Loader)
   movl 4(%esp), %ebx // copy 4 bytes from 4B above esp to ebx
   movl %ebx, %esp    // copy ebx to esp (point esp to process trapframe)
   popa               // pop all (8) registers
   addl $4, %esp      // skip 4 bytes
   iret               // return from interrupt

ENTRY(TimerEntry)
   pushl $TIMER_INTR
   jmp PreKernel

ENTRY(GetPidEntry)
   pushl $GETPID_CALL
   jmp PreKernel

ENTRY(ShowCharEntry)
   pushl $SHOWCHAR_CALL
   jmp PreKernel

ENTRY(SleepEntry)
   pushl $SLEEP_CALL
   jmp PreKernel
   
ENTRY(MuxCreateEntry)
   pushl $MUX_CREATE_CALL
   jmp PreKernel
   
ENTRY(MuxOpEntry)
   pushl $MUX_OP_CALL
   jmp PreKernel
   
ENTRY(Term0Entry)
   pushl $TERM0_INTR
   jmp PreKernel
   
ENTRY(Term1Entry)
   pushl $TERM1_INTR
   jmp PreKernel

PreKernel:
   pushal							// push all (8) registers
   movl %esp, %ebx					// copy esp to ebx
   movw $0x10, %ax					// set kernel Data Segment registers
   mov %ax, %ds						// set kernel Data Degment registers
   mov %ax, %es						// set kernel Data Segment registers
   leal k_stack + K_STACK_SIZE, %esp	// set esp to top of kernel stack
   pushl %ebx						// (onto the kernel stack)
   cld								// clear the direction flag of efl
   call CNAME(Kernel)				// call C function Kernel
