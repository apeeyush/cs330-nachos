#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;
    int x;
    sys_PrintString("Starting physical address of array: ");
    sys_PrintInt(sys_GetPA((unsigned)array));
    sys_PrintString("Physical address of array[50]: ");
    sys_PrintInt(sys_GetPA(&array[50]));
    sys_PrintChar('\n');

    sys_PrintString("Current physical address of stack top: ");
    sys_PrintInt(sys_GetPA(sys_GetReg(29)));
    sys_PrintChar('\n');
    for (i=0; i<SIZE; i++) array[i] = i;
    sys_PrintString("We are currently at PC: ");
    sys_PrintIntHex(sys_GetReg(34));
    sys_PrintChar('\n');
    sys_Sleep(1000000);
    sys_PrintInt(sys_GetPID());
    sys_PrintChar('\n');
    sys_PrintInt(sys_GetTime());
    sys_PrintChar('\n');

    x = sys_Fork();
    if (x == 0) {
        sys_PrintString("We are currently at PC: ");
        sys_PrintInt(sys_GetPID());
        sys_PrintChar('\n');
    }
//    sys_PrintString("We zsdfghjn dfghj szdf: ");
    sys_PrintInt(x);
    sys_PrintInt(sys_GetPID());
    sys_PrintInt(sys_GetPPID());
    sys_PrintString("We ");
    // for (i=0; i<SIZE; i++) sum += array[i];
    // sys_PrintString("Total sum: ");
    // sys_PrintInt(sum);
    // sys_PrintChar('\n');
    // sys_Exit(0);
    return 0;
}
