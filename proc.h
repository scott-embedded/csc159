// proc.h, 159

#ifndef __PROC__	//SOH
#define __PROC__

void InitProc(void);
void UserProc(void);
void InitTerm(int term_no);
void Aout(int device);
void Ouch(int device);
void Wrapper(int handler, int arg);

#endif
