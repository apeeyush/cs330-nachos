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


#include "syscall.h"

int main()
{
    int x;
    int sleep_start, sleep_end;
    int *array = (int*)sys_ShmAllocate(2*sizeof(int));
    sys_PrintString("Parent PID: ");
    sys_PrintInt(sys_GetPID());
    sys_PrintChar('\n');
    x = sys_Fork();
    if (x == 0) {
      array[0] += 2;
      array[1] -= 3;
      sys_PrintInt(array[0]);
      sys_PrintChar('\n');
      sys_PrintInt(array[1]);
      sys_PrintChar('\n');
      sys_PrintString("child : ");
      sys_PrintInt(sys_GetPID());
    }
    else {
      sys_Sleep(1000);
      array[1] -= 4;
      sys_PrintInt(array[0]);
      sys_PrintChar('\n');
      sys_PrintInt(array[1]);
      sys_Join(x);
    }
    return 0;
}
