// sys-call.h, 159

#ifndef __SYS_CALL__
#define __SYS_CALL__

int GetPidCall(void);
void ShowCharCall(int row, int col, char ch);
void SleepCall(int centi_sec);
int MuxCreateCall(int);
void MuxOpCall(int, int);
void WriteCall(int, char *);

#endif