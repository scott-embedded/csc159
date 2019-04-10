// k-lib.h, 159

#ifndef __K_LIB__
#define __K_LIB__

#include "k-type.h"			//SOH

void Bzero(char *, int bytes);     // prototype those in k-lib.c here
int QisEmpty(q_t *);
int QisFull(q_t *);
int DeQ(q_t *);
void EnQ(int, q_t *);
void MemCpy(char *dst, char *src, int size);
int StrCmp(char *str1, char *str2);
void Itoa(char *str, int x);

#endif
