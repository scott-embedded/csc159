#include "k-const.h"
#include <spede/machine/asmacros.h>
#define K_STACK_SIZE (PROC_STACK_SIZE *8)

.comm k_stack, K_STACK_SIZE, 1
.text

ENTRY(Loader)
   movl 4(%esp), %ebx
   movl %ebx, %esp
   popa
   add $4, %esp
   iret

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
PreKernel:
   pusha
   movl %esp, %ebx
   movw $0x10, %ax
   mov %ax, %ds
   mov %ax, %es
   leal k_stack + K_STACK_SIZE, %esp
   pushl %ebx
   cld
   call CNAME(Kernel)
