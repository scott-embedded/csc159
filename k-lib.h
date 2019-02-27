// k-lib.h, 159

#ifndef __K_LIB__
#define __K_LIB__

#include "k-type.h"		

void Bzero(char *, int bytes);     // prototype those in k-lib.c here
int QisEmpty(q_t *);
int QisFull(q_t *);
int DeQ(q_t *);
void EnQ(int, q_t *);

#endif
