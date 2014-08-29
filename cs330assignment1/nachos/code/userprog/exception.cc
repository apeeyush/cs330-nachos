// exception.cc 
//  Entry point into the Nachos kernel from user programs.
//  There are two kinds of things that can cause control to
//  transfer back to here from user code:
//
//  syscall -- The user code explicitly requests to call a procedure
//  in the Nachos kernel.  Right now, the only function we support is
//  "Halt".
//
//  exceptions -- The user code does something that the CPU can't handle.
//  For instance, accessing memory that doesn't exist, arithmetic errors,
//  etc.  
//
//  Interrupts (which can also cause control to transfer from user
//  code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"
#include "thread.h"

extern void StartProcess(char *file);

void ForkStartFunction(int arg)
{
  currentThread->Startup();
  machine->Run();
}

//----------------------------------------------------------------------
// ExceptionHandler
//  Entry point into the Nachos kernel.  Called when a user program
//  is executing, and either does a syscall, or generates an addressing
//  or arithmetic exception.
//
//  For system calls, the following is the calling convention:
//
//  system call code -- r2
//    arg1 -- r4
//    arg2 -- r5
//    arg3 -- r6
//    arg4 -- r7
//
//  The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//  "which" is the kind of exception.  The list of possible exceptions 
//  are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }



static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    int regtype;
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    Thread *child;    // Used by SC_Fork

    if ((which == SyscallException) && (type == syscall_Halt)) {
  DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
    writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
       writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
       writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
  writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
    writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    // syscall_GetReg (syscall 1)
    else if ((which == SyscallException) && (type == syscall_GetReg)) {
       regtype = machine->ReadRegister(4);
       machine->WriteRegister(2,machine->ReadRegister(regtype));
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_GetPA (syscall 2)
    else if ((which == SyscallException) && (type == syscall_GetPA)) {
       if ( machine->ReadRegister(4)/PageSize > machine->pageTableSize ){
          machine->WriteRegister(2,-1);
       }
       else if ( machine->pageTable[machine->ReadRegister(4)/PageSize].valid == FALSE ){
          machine->WriteRegister(2,-1);
       }
       else if ( machine->pageTable[machine->ReadRegister(4)/PageSize].physicalPage > NumPhysPages ){
          machine->WriteRegister(2,-1);
       }
       else{
          int physAddr;
          machine->Translate(machine->ReadRegister(4), &physAddr, PageSize, FALSE);
          machine->WriteRegister(2,physAddr);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_GetPID (syscall 3)
    else if ((which == SyscallException) && (type == syscall_GetPID)) {
       machine->WriteRegister(2,currentThread->getPid());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_GetPPID (syscall 4)
    else if ((which == SyscallException) && (type == syscall_GetPPID)) {
       machine->WriteRegister(2,currentThread->getPpid());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_Time (syscall 5)
    else if ((which == SyscallException) && (type == syscall_Time)) {
       machine->WriteRegister(2,stats->totalTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_Yield (syscall 6)
    else if ((which == SyscallException) && (type == syscall_Yield)) {
       currentThread->Yield();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_NumInstr (syscall 7)
    else if ((which == SyscallException) && (type == syscall_NumInstr)) {
       machine->WriteRegister(2,stats->userTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    // syscall_Exec (syscall 8)
    // syscall_Exec: This is same as the execv() call we have discussed in the class. In
    // the context of NachOS, you need to mimic what the StartProcess() function
    // does. Do not try to reuse or overwrite the existing address space.
    // Remember to set up the page table for the new address space properly.
    else if ((which == SyscallException) && (type == syscall_Exec)) {
       int physicalAddress = machine->ReadRegister(4);
       char filename[1000];
       int i=0;
       while(machine->mainMemory[physicalAddress] != NULL){
        filename[i] = machine->mainMemory[physicalAddress];
        i++;
        physicalAddress++;
       }
       filename[i] = '\0';
       AddrSpace* addrspace = currentThread->space;
       addrspace->~AddrSpace();
       StartProcess(filename);
    }
    // syscall_Sleep (syscall 9)
    else if ((which == SyscallException) && (type == syscall_Sleep)) {
       int arg = machine->ReadRegister(4);
       if (arg == 0){
          currentThread->Yield();
       }else{
      //    timer = new Timer(TimerInterruptHandler, 0, randomYield);
          int nticks =  arg + stats->totalTicks;
          IntStatus oldLevel = interrupt->SetLevel(IntOff);
          sleepQ->SortedInsert(currentThread,nticks);   // disable interrupts
          currentThread->Sleep();
          (void) interrupt->SetLevel(oldLevel);               // re-enable interrupts
       }

       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Fork)) {

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

       child = new Thread("Forked thread");
       child->space = new AddrSpace (currentThread->space);  // Duplicates the address space
       child->SaveUserState ();                 // Duplicate the register set
       child->ResetReturnValue ();           // Sets the return register to zero
       child->StackAllocate (ForkStartFunction, 0); // Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->getPid());    // Return value for parent
    }

    else {
  printf("Unexpected user mode exception %d %d\n", which, type);
  ASSERT(FALSE);
    }

}
