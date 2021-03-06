// handlers.c, 159

#include "spede.h"
#include "types.h"
#include "data.h"
#include "tools.h"
#include "proc.h"
#include "handlers.h"
#include "syscalls.h"

// to create process, 1st alloc PID, PCB, and process stack space
// build process frame, initialize PCB, record PID to run_q (if not 0)
void NewProcHandler(func_p_t p) {  // arg: where process code starts
	int pid;

	if(ready_q.size == 0)  {//the size of ready_q is 0 // may occur as too many processes been created
		cons_printf("Kernel Panic: cannot create more process!\n");
		return;                   // alternative: breakpoint() into GDB
	}

	pid = DeQ(&ready_q); //get a 'pid' from ready_q
	MyBzero((char *)&pcb[pid], sizeof(pcb_t)); //use tool function MyBzero to clear PCB and runtime stack
	MyBzero( (char *)&proc_stack[pid], sizeof(proc_stack[pid]));
	pcb[pid].state = RUN;
	if( pid != 0 ) 
		EnQ( pid, &run_q );
   //queue it (pid) to be run_q unless it's 0 (SystemProc)

	pcb[pid].proc_frame_p = (proc_frame_t *)&proc_stack[pid][PROC_STACK_SIZE-sizeof(proc_frame_t)]; //point proc_frame_p to into stack (to where best to place a process frame)
	pcb[pid].proc_frame_p->EFL = EF_DEFAULT_VALUE|EF_INTR; //fill out EFL with "EF_DEFAULT_VALUE|EF_INTR" // to enable intr!
	pcb[pid].proc_frame_p->EIP = (unsigned int) p; //fill out EIP to p
	pcb[pid].proc_frame_p->CS = get_cs(); //fill CS with the return from a get_cs() call
}

// count run_time of running process and preempt it if reaching time slice
void TimerHandler(void) {//phase 1
	int i;
	timer_ticks++;
	for(i = 0; i < PROC_NUM; i++){
		if(( pcb[i].state == SLEEPING ) && ( pcb[i].wake_time == timer_ticks)){
			pcb[i].state = RUN;
			EnQ(i, &run_q); 
		}
	}

	outportb(0x20, 0x60); //dismiss timer event (IRQ0)

	if(run_pid == 0) return;   //if the running process is SystemProc, simply return

	pcb[run_pid].run_time++; //upcount the run_time of the running process
	if(pcb[run_pid].run_time == TIME_SLICE){ //it reaches the the OS time slice
		EnQ(run_pid, &run_q); //queue it back to run queue
		run_pid = -1; //reset the running pid (to -1)  // no process running anymore
	}
}

void GetPidHandler(void){//phsae 2
    //fill out the register value in the process frame of the calling process (syscall GetPid() will subsequently pull it out to return to the calling process code)
	pcb[run_pid].proc_frame_p->EAX = run_pid;
}
void WriteHandler(proc_frame_t* p){//phase 2
	int i;
	char* strMsg  = (char *)p->ECX;

	if(p->EBX == STDOUT)  cons_printf(strMsg);
	else{
		while(*strMsg){
			outportb(p->EBX + DATA, *strMsg);
			for(i = 0; i < 5000; i++) asm("inb $0x80");
      			strMsg++;
		}
	}
}

void SleepHandler(void){//phsae 2
      //(syscall Sleep() has placed the sleep time in a CPU register)
      //calculate the wake time by the current time plus the sleep time
      //(both in terms of timer-event counts), and place it into the
      //PCB of the calling process, alter its state, reset the running PID
	pcb[ run_pid ].wake_time = timer_ticks + 100 * pcb[ run_pid ].proc_frame_p->EBX;
    //Change state and wait until wake_time
	pcb[ run_pid ].state = SLEEPING;
    //Return CPU usage to SysProc
	run_pid = -1;
}

void MutexLockHandler(void){//phase 3
	if(mutex.lock == UNLOCK){
		mutex.lock = LOCK;
	}
	else{
		EnQ(run_pid, &mutex.wait_q);
		pcb[run_pid].state = WAIT;
		run_pid = 0;
	}
	return;
}
void MutexUnlockHandler(void){//phase 3
	int pid;
	if(mutex.wait_q.size == 0)
		mutex.lock = UNLOCK;
	else{
		pid = DeQ(&mutex.wait_q);
		pcb[pid].state = RUN;
		EnQ(pid, &run_q);
	}
	return;

}

void GetCharHandler(proc_frame_t *p) {  // GetChar() call, like MutexLock()
	int i;
	int fileno = p->EBX;

	if(fileno == TERM1)
		i = 0;
	else
		i = 1;

	if(terminal_buffer[i].size > 0)//if has stored input
		pcb[run_pid].proc_frame_p->ECX = DeQ(&term_kb_wait_q[i]); //get one from it and give it to the calling process
	else{// (input has yet arrived)
		pcb[run_pid].state = WAIT;
		EnQ(run_pid, &term_kb_wait_q[i]); //block the calling process to terminal wait queue i
		run_pid = -1;
	}
}

void PutCharHandler(int fileno) {
	int i;
	char ch = pcb[run_pid].proc_frame_p->ECX;			//ch is the ECX in proc frame (set by syscall)

	i = (fileno == TERM1)? 0 : 1;	 		//if fileno is TERM1, i is 0; otherwise 1

	outportb(fileno + DATA, ch);		//call outportb() to send ch to data register based on fileno
      	pcb[run_pid].state = WAIT;
	EnQ(run_pid, &term_screen_wait_q[i]);	//block the calling process (to terminal wait queue i, etc.)
	run_pid = -1;
}

void TermHandler(int port) {  // IRQ3 or IRQ4, like MutexUnlock()
	int i;
	char ch;
	int pid, indicator;

	//if port is TERM1, i is 0; otherwise 1
	i = (port == TERM1)? 0 : 1;

	indicator = inportb(port+IIR);
      	if(indicator == IIR_RXRDY){
		ch = inportb(port + DATA);
		if(term_kb_wait_q[i].size == 0)//if terminal wait queue is empty
			EnQ((int) ch, &terminal_buffer[i]); //append ch to terminal kb buffer i
		else// (some process awaits)
			pid = DeQ(&term_kb_wait_q[i]);
			pcb[pid].state = RUN;
			EnQ(pid, &run_q);

			if(pcb[pid].sigint_handler !=0 && ch == (char) 3){
				InsertWrapper(pid, pcb[pid].sigint_handler); 
			}
			pcb[pid].proc_frame_p->ECX = ch;
	}
	else{
		if(term_screen_wait_q[i].size > 0){
			pid = DeQ(&term_screen_wait_q[i]);
			pcb[pid].state = RUN;
			EnQ(pid, &run_q);
		}
	}
}

void ForkHandler(proc_frame_t *parent_frame_p) { // Kernel() provides this ptr
	int child_pid, delta, *bp, temp;
	proc_frame_t *child_frame_p;
	
	if(ready_q.size == 0){
		cons_printf("Kernel Panic: cannot create more process!\n");
		pcb[run_pid].proc_frame_p->EBX = -1;
		return;
	}

	child_pid = DeQ(&ready_q);				//get first process available 
	EnQ(child_pid, &run_q);					//enque it into run queue
	MyBzero((char *)&pcb[child_pid], sizeof(pcb_t));	//clear out the child_pid pcd
	pcb[child_pid].state = RUN;				//set its state to run;
	pcb[child_pid].ppid = run_pid;		//set parent pid to current pid;
	pcb[child_pid].sigint_handler = pcb[run_pid].sigint_handler;
	
	MyMemcpy((char *)&proc_stack[child_pid], (char *)&proc_stack[run_pid], PROC_STACK_SIZE);

	delta = proc_stack[child_pid] - proc_stack[run_pid];	//a. delta = child stack <--- byte distance ---> parent stack

	temp = ((int) parent_frame_p + delta);
	child_frame_p = (proc_frame_t *) temp;			//b. set child frame location = parent frame location + delta

	pcb[child_pid].proc_frame_p = child_frame_p;

	child_frame_p->ESP += delta;		//c. the same goes to the ESP, EBP, ESI, and EDI; in the new child process frame each of these is added with delta
	child_frame_p->EBP += delta; 		
	child_frame_p->ESI += delta;
	child_frame_p->EDI += delta;

		

	parent_frame_p->EBX = child_pid; 				//d. set the EBX in the parent's process frame to the child_pid, but it's given 0 to the EBX in the child's process frame
	child_frame_p->EBX = 0; 					 

	

	bp = (int *)child_frame_p->EBP;
	while(*bp != '\0'){
       		*bp += delta;
        	bp = (int *)*bp;
	}	
}

//phase8
void SignalHandler(proc_frame_t* p){
	if(p->EBX == SIGINT)
		pcb[run_pid].sigint_handler = (func_p_t)p->ECX;
	else if(p->EBX == SIGCHLD)
		pcb[run_pid].sigchld_handler = (func_p_t)p->ECX;
}

void InsertWrapper(int pid, func_p_t handler){
	proc_frame_t temp_frame = *pcb[pid].proc_frame_p;
	int *temp_location;

	temp_location = (int *)&pcb[pid].proc_frame_p->EFL;
	*temp_location = (int)handler;
	temp_location--;
	*temp_location = (int)temp_frame.EIP;
	pcb[pid].proc_frame_p = (proc_frame_t *) ((int)pcb[pid].proc_frame_p - sizeof(int[2]));

	MyMemcpy((char *)pcb[pid].proc_frame_p, (char *) &temp_frame, sizeof(proc_frame_t));

	pcb[pid].proc_frame_p->EIP = (unsigned int) Wrapper;
}

//phase9
void ExitHandler(proc_frame_t *p) {              // when child calls Exit()
	int ppid;
	
	ppid = pcb[run_pid].ppid;

	if(pcb[ppid].state != WAITCHLD){		// child early, parent late
		pcb[run_pid].state = ZOMBIE;		// zombie to be reclaimed later
		run_pid = -1;				//reset run_pid to ?  // no longer (resources still used)
		if(pcb[ppid].sigchld_handler != NULL){              // is it right time to redirect?
			InsertWrapper(ppid, pcb[ppid].sigchld_handler);
		}
	}
	else {
		pcb[ppid].state = RUN;			//release parent, change state to ?
		EnQ(ppid, &run_q);
	
							//do'nt forget to deliver to the released parent process:
         	pcb[ppid].proc_frame_p->EBX = pcb[run_pid].proc_frame_p->EBX;	// child's exiting number
        	pcb[ppid].proc_frame_p->ECX = run_pid;		// exiting child PID

         						//kernel reclaims exiting child process' resources:
         	EnQ(run_pid, &ready_q);			//1. its PID (enqueue it to ?)
         	pcb[run_pid].state = READY;			//2. its state (changed back to ?)
         	run_pid = -1;				//3. reset run_pid to ?
      	}
}

void WaitPidHandler(proc_frame_t *p) {           // when parent calls WaitPid()
	int child_pid, child_exit_num, *parent_exit_num_p, i;

	parent_exit_num_p = (int *)pcb[run_pid].proc_frame_p->EBX;
	child_pid = -1;
	for(i = 0; i < PROC_NUM; i++){		//search loop, for each in the PCB array:
		if(pcb[i].state == ZOMBIE && pcb[i].ppid == run_pid)	//if its state is ZOMBIE and its ppid is run_pid
			child_pid = i;					// --> break loop (found)
			break;
	}

	if(child_pid == -1){		// not found
		pcb[run_pid].state = WAITCHLD;	//change parent's state to ?
		run_pid = -1;			//reset run_pid to ?
	}
	else {
					//deliver to parent process:
		pcb[run_pid].proc_frame_p->EBX = pcb[child_pid].proc_frame_p->EBX;	//ZOMBIE's exiting number
		pcb[run_pid].proc_frame_p->ECX = child_pid;					//2. ZOMBIE PID
					//kernel reclaims exiting child process' resources:
         	EnQ(child_pid, &ready_q);			//1. its PID (enqueue it to ?)
		for(i = 0;i<PAGE_NUM; i++){
			if(page[i].ownerpid == child_pid){
        			page[i].ownerpid = -1;
				break;
      			}
		}
         	pcb[child_pid].state = READY;			//2. its state (changed back to ?)
      }
}

void ExecHandler(proc_frame_t *p){
	int i;
	for( i = 0;i<PAGE_NUM;i++){
		if(page[i].ownerpid == -1) break;
	}
	page[i].ownerpid = run_pid;
	MyMemcpy((char *)page[i].addr, (char *)p->EBX, PAGE_SIZE);
	pcb[run_pid].proc_frame_p = (proc_frame_t *)(page[i].addr + PAGE_SIZE);
	pcb[run_pid].proc_frame_p--;
	MyMemcpy((char *)pcb[run_pid].proc_frame_p, (char *)p, sizeof(proc_frame_t));
	pcb[run_pid].proc_frame_p->EIP = (unsigned int)page[i].addr;
}
