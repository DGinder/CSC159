// types.h, 159

#ifndef _TYPES_H_
#define _TYPES_H_

#define TIMER_EVENT 32       // IDT entry #32 has code addr for timer event (DOS IRQ0)
#define SYSCALL_EVENT 128

#define LOOP 1666666         // handly loop limit exec asm("inb $0x80");
#define TIME_SLICE 200       // max timer count, then rotate process
#define PROC_NUM 20          // max number of processes
#define Q_SIZE 20            // queuing capacity
#define PROC_STACK_SIZE 4096 // process runtime stack in bytes

#define SYSCALL_EVENT 128	//phase 2
#define WRITE 4 
#define GETPID 100
#define SLEEP 101
#define STDOUT 1

#define MUTEX 102		//phase 3
#define LOCK 0
#define UNLOCK 1

#define TERM1 0x2f8		//phase 4
#define TERM2 0x3e8

				//phase5
#define GETCHAR 103          // service #
#define TERM1_EVENT 35       // IDT entry #35 leads to Term1Event
#define TERM2_EVENT 36       // IDT entry #36 leads to Term2Event

				//phase6
#define PUTCHAR 104

#define FORK 2 //phase 7

//phase8
#define SIGNAL 48             	// service # for Signal() call
#define SIGINT 2 	   	// signal number when ctrl-c is 2

//phase 9
#define SIGCHLD 17
#define EXIT 1
#define WAITPID 7

//phaseA
#define EXEC 11
#define PAGE_NUM 100
#define PAGE_SIZE 4096

typedef void (*func_p_t)(void); // void-return function pointer type

typedef enum {READY, RUN, SLEEPING, WAIT, WAITCHLD, ZOMBIE} state_t;
typedef enum {KB1, KB2, SCREEN1, SCREEN2} mutex_id_t;

typedef struct {
	unsigned int EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX;
	unsigned int event_type;
	unsigned int EIP;
	unsigned int CS;
	unsigned int EFL;
} proc_frame_t;

typedef struct {
	state_t state;            // state of process
	int run_time;             // CPU runtime this time
	int life_time;            // total CPU runtime
	int wake_time;
	int ppid;
	func_p_t sigint_handler;
	func_p_t sigchld_handler;
	proc_frame_t *proc_frame_p; // points to saved process frame
} pcb_t;                     

typedef struct {             // generic queue type
	int size;                 // size is also where the tail is for new data
	int q[Q_SIZE];            // integers are queued in q[] array
} q_t;

typedef struct {             
   mutex_id_t mutex_id;
   int lock;                 
   q_t wait_q;            
} mutex_t;

typedef struct {
   int addr;
   int lrucount;
   int ownerpid;
} page_t;

#endif // _TYPES_H_
