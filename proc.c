// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use syscalls

#include "k-const.h"
#include "k-data.h"

void Delay(void) {  // delay CPU for half second by 'inb $0x80'
   int i;
   for (i = 0; i < 500000; i++)	//guess?
	   asm("inb $0x80");
}

void ShowChar(int row, int col, char ch) { // show ch at row, col
   unsigned short *p = VID_HOME;
   
   p += row * 80 + col;
   *p = ch + VID_MASK;
}

void InitProc(void) {
	//cons_printf("InitProc\n");
   while(1) {
      ShowChar(0, 0, '.');
      Delay();

      ShowChar(0, 0, ' ');
      Delay();
   }
}

void UserProc(void) {
   //cons_printf("USERPROC\n");
   while(1) {
       
      ShowChar(run_pid, 0, run_pid / 10 + '0');
      ShowChar(run_pid, 1, run_pid % 10 + '0');
      Delay();

      ShowChar(run_pid, 0, ' ');
      ShowChar(run_pid, 1, ' ');
      Delay();
   }
}