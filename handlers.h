// handlers.h, 159

#ifndef _HANDLERS_H_
#define _HANDLERS_H_

#include "types.h"   // need definition of 'func_p_t' below

void NewProcHandler(func_p_t);//phase 1

void TimerHandler(void);//phase 2
void GetPidHandler(void);
void WriteHandler(proc_frame_t *);
void SleepHandler(void);

void MutexLockHandler(void);//phase 3
void MutexUnlockHandler(void);

void GetCharHandler(proc_frame_t *p);//phase 5
void PutCharHandler(int);
void TermHandler(int);

void ForkHandler(proc_frame_t *); // phase 7

void SignalHandler(proc_frame_t*);	//phase8
void InsertWrapper(int, func_p_t);

//phase 9
void ExitHandler(proc_frame_t *);
void WaitPidHandler(proc_frame_t *);

//phaseA
void ExecHandler(proc_frame_t *);
#endif
