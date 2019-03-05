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

PreKernel:
   pushal					
   movl %esp, %ebx
   movw $0x10, %ax
   mov %ax, %ds
   mov %ax, %es
   leal k_stack + K_STACK_SIZE, %esp
   pushl %ebx
   cld
   call CNAME(Kernel)
