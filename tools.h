// tools.h, 159

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "types.h" // need definition of 'q_t' below

void EnQ(int, q_t *);
int DeQ(q_t *);
void MyBzero(char *, int);
void MyMemcpy(char *, char *, int);
int MyStrcmp(char *, char *);

#endif

