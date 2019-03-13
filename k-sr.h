// k-sr.h, 159

#ifndef __K_SR__
#define __K_SR__

#include "k-type.h"

void NewProcSR(func_p_t);  // prototype those in k-sr.c here
void TimerSR(void);
int GetPidSr(void);
void CheckWakeProc(void);
void ShowCharCallSr(int row, int col, char ch);
void SleepSr(int centi_sec);
int MuxCreateSR(int flag);
void MuxOpSR(int mux_id, int opcode);
void TermSR(int term_no);
void TermTxSR(int term_no);
void TermRxSR(int term_no);

#endif
