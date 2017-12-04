// tools.c, 159

#include "spede.h"
#include "types.h"
#include "data.h"

// clear DRAM data blocks by filling zeroes
void MyBzero(char *p, int size) {
	int i;
	for(i=0;i<size;i++){
		*p = (char)0;
		p++;
	}
}

// dequeue, return 1st element in array, and move all forward
// if queue empty, return -1
int DeQ(q_t *p) {         // return -1 if q[] contains no valid elements
	int i,  element = -1;

	if(p->size == 0) return element;	//f the size of the queue that p points to is zero, return element
	else{//(continue/otherwise)
		element = p->q[0];  			//copy the 1st in the array that p points to to element
		p->size--;//decrement the size of the queue that p points to
		for(i=0;i<=p->size;i++){   	//move all elements in the array to the front by one position
			p->q[i] = p->q[i+1];
		}
	}
	return element;
}

// enqueue element to next available position in array, 'size' is array index
void EnQ(int element, q_t *p) {
	if(p->size == Q_SIZE){	//if the size of the queue that p points to is already Q_SIZE
		cons_printf("\nKernel Panic: queue is full, cannot EnQ!\n");
		return;       // alternative: breakpoint() into GDB
   	}
   	p->q[p->size] = element; //copy element to the location in the array indexed by 'size'
	p->size++;	//inrement size of the queue
}

void MyMemcpy(char *destination, char *source, int byte_size) {

	while(byte_size > 0){	//copy, byte by byte, from source to destination, for byte_size bytes
		*destination = *source;
		destination++;
		source++;
		byte_size--;
	}
	 
}
int MyStrcmp(char *str1, char *str2) { // both null delimited
	while(*str1){			//loop while *str1 is not null {
		if(*str1 != *str2)
			return 0;	//if *str1 is  not *str2, not the same, return 0
		str1++; 		//advance both str1 and str2
		str2++;
	}
	if(*str1 == *str2) return 1; 	//after the loop, if *str1 is *str2, they are the same, return 1
	else return 0;			//else, they are not the same return 0
}

