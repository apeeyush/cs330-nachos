//  printtest.c
//  *	Simple program to test whether printing from a user program works.
//  *	
//  *	Just do a "syscall" that shuts down the OS.
//  *
//  * 	NOTE: for some reason, user programs with global data structures 
//  *	sometimes haven't worked in the Nachos environment.  So be careful
//  *	out there!  One option is to allocate data structures as 
//  * 	automatics within a procedure, but if you do this, you have to
//  *	be careful to allocate a big enough stack to hold the automatics!
 

// #include "syscall.h"

// int
// main()
// {
//     sys_PrintString("hello world\n");
//     sys_PrintString("Executed ");
//     sys_PrintInt(sys_GetNumInstr());
//     sys_PrintString(" instructions.\n");
//     return 0;
// }

// #include "syscall.h"
// #include "synchop.h"

// int main()
// {
//     int x;
//     int sleep_start, sleep_end;
//     int *array = (int*)sys_ShmAllocate(2*sizeof(int));
//     // sys_PrintString("Parent PID: ");
//     // sys_PrintChar('\n');
    
//     int y = sys_SemGet(1);
//     int t = sys_CondGet(1);
//     array[0] = 5;
//     array[1] = 0;
//     int a = 1;
//     sys_SemCtl(y, SYNCH_SET, &a);
//     x = sys_Fork();
//     if (x == 0) {
//       // sys_SemOp(y,-1);
//       // sys_CondOp(t,COND_OP_WAIT, y);
//       // array[0] = 1;
//       // sys_PrintInt(array[1]);
//       // sys_PrintChar('\n');
//       // sys_PrintString("Finish Chils");
//       // sys_SemOp(y,1);
//     }
//     else {
//         sys_SemOp(y,-1);
//         // array[1] = 1;
//         // sys_PrintInt(array[0]);
//         // sys_PrintChar('\n');
//         sys_CondOp(t,COND_OP_SIGNAL, y);
//         sys_SemOp(y,1);
//         sys_Join(x);
//         sys_PrintString("Finish Parent");
//     }
//     sys_PrintString("Finish");
//     return 0;
// }


#include "syscall.h"
#define OUTER_BOUND 4
#define SIZE 100
#include "synchop.h"
int
main()
{
int arr[SIZE], i, k, sum, pid=sys_GetPID()-1;
int id;
for(i = 0;i<64;i++)
id = sys_SemGet(i);
sys_SemCtl(32,SYNCH_REMOVE,0);
id = sys_SemGet(32);
id = sys_SemGet(323);
int a = 1;
sys_SemCtl(id,SYNCH_SET,&a);
unsigned start_time, end_time;
int *array = (int*)sys_ShmAllocate(2*sizeof(int));
char *abc = (char*)sys_ShmAllocate(158);
abc[32] = 'd';
array[0] = 1;
array[1] = 2;
sys_PrintInt(array[0]);
sys_PrintChar(':');
sys_PrintInt(array[1]);
sys_PrintString("\nAbout to Fork \n");
if(sys_Fork() == 0){
sys_SemOp(id,-1);
abc[131] = '#';
array[0] = 4;
array[1] = 6;
sys_PrintChar(abc[131]);
sys_PrintString("child sees: ");
sys_PrintInt(array[0] );
sys_PrintChar(':');
sys_PrintInt(array[1]);
sys_PrintChar('\n');
sys_PrintChar(abc[131]);
sys_SemOp(id,1);
}
else{
sys_SemOp(id,-1);
abc[131]='*';
sys_PrintChar(abc[131]);
sys_PrintString("Parent: ");
sys_PrintInt(array[0] );
sys_PrintChar(':');
sys_PrintInt(array[1]);
sys_PrintChar('\n');
sys_PrintChar(abc[131]);
sys_SemOp(id,1);
}
return 0;
}