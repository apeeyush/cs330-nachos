// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
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
#include "synchop.h"

#define divRoundUp(n,s)    (((n) / (s)) + ((((n) % (s)) > 0) ? 1 : 0))

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

extern void StartProcess (char*);
extern void ExecProcess (char*);

void
ForkStartFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

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


static void 
SwapHeader (NoffHeader *noffH)
{
  noffH->noffMagic = WordToHost(noffH->noffMagic);
  noffH->code.size = WordToHost(noffH->code.size);
  noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
  noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
  noffH->initData.size = WordToHost(noffH->initData.size);
  noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
  noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
  noffH->uninitData.size = WordToHost(noffH->uninitData.size);
  noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
  noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;	// Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);
    int exitcode;		// Used in syscall_Exit
    unsigned i;
    char buffer[1024];		// Used in syscall_Exec
    int waitpid;		// Used in syscall_Join
    int whichChild;		// Used in syscall_Join
    Thread *child;		// Used by syscall_Fork
    unsigned sleeptime;		// Used by syscall_Sleep

    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_Exit)) {
       exitcode = machine->ReadRegister(4);
       printf("[pid %d]: Exit called. Code: %d\n", currentThread->GetPID(), exitcode);
       // We do not wait for the children to finish.
       // The children will continue to run.
       // We will worry about this when and if we implement signals.
       exitThreadArray[currentThread->GetPID()] = true;

       // Find out if all threads have called exit
       for (i=0; i<thread_index; i++) {
          if (!exitThreadArray[i]) break;
       }
       currentThread->Exit(i==thread_index, exitcode);
    }
    else if ((which == SyscallException) && (type == syscall_Exec)) {
       // Copy the executable name into kernel space
       vaddr = machine->ReadRegister(4);
       bool flag = machine->ReadMem(vaddr, 1, &memval);
       while(flag != TRUE){
        flag = machine->ReadMem(vaddr, 1, &memval);
       }
       i = 0;
       while ((*(char*)&memval) != '\0') {
          buffer[i] = (*(char*)&memval);
          i++;
          vaddr++;
          bool flag = machine->ReadMem(vaddr, 1, &memval);
          while(flag != TRUE){
            flag = machine->ReadMem(vaddr, 1, &memval);
          }
       }
       buffer[i] = (*(char*)&memval);
       ExecProcess(buffer);
    }
    else if ((which == SyscallException) && (type == syscall_Join)) {
       waitpid = machine->ReadRegister(4);
       // Check if this is my child. If not, return -1.
       whichChild = currentThread->CheckIfChild (waitpid);
       if (whichChild == -1) {
          printf("[pid %d] Cannot join with non-existent child [pid %d].\n", currentThread->GetPID(), waitpid);
          machine->WriteRegister(2, -1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
       else {
          exitcode = currentThread->JoinWithChild (whichChild);
          machine->WriteRegister(2, exitcode);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
    }
    else if ((which == SyscallException) && (type == syscall_Fork)) {
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       child = new Thread("Forked thread", GET_NICE_FROM_PARENT);
       child->fallMem = new char[currentThread->space->GetNumPages()*PageSize];
       child->space = new AddrSpace (currentThread->space, child->GetPID());
       child->space->AddrSpaceInitialize(currentThread->space, child->GetPID());

       child->SaveUserState ();		     		      // Duplicate the register set
       child->ResetReturnValue ();			     // Sets the return register to zero
       child->StackAllocate (ForkStartFunction, 0);	// Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->GetPID());		// Return value for parent
    }
    else if ((which == SyscallException) && (type == syscall_Yield)) {
       currentThread->Yield();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
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
        writeDone->P() ;        // wait for previous write to finish
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       bool flag = machine->ReadMem(vaddr, 1, &memval);
       while(flag != TRUE){
        flag = machine->ReadMem(vaddr, 1, &memval);
       }
       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          bool flag = machine->ReadMem(vaddr, 1, &memval);
          while(flag != TRUE){
            flag = machine->ReadMem(vaddr, 1, &memval);
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetReg)) {
       machine->WriteRegister(2, machine->ReadRegister(machine->ReadRegister(4))); // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPA)) {
       vaddr = machine->ReadRegister(4);
       machine->WriteRegister(2, machine->GetPA(vaddr));  // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPID)) {
       machine->WriteRegister(2, currentThread->GetPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPPID)) {
       machine->WriteRegister(2, currentThread->GetPPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Sleep)) {
       sleeptime = machine->ReadRegister(4);
       if (sleeptime == 0) {
          // emulate a yield
          currentThread->Yield();
       }
       else {
          currentThread->SortedInsertInWaitQueue (sleeptime+stats->totalTicks);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Time)) {
       machine->WriteRegister(2, stats->totalTicks);
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
    else if ((which == SyscallException) && (type == syscall_NumInstr)) {
       machine->WriteRegister(2, currentThread->GetInstructionCount());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else if ((which == SyscallException) && (type == syscall_ShmAllocate)){
       int shm_size = machine->ReadRegister(4);
       AddrSpace* curr_space = currentThread->space;
       TranslationEntry *oldPageTable = curr_space->GetPageTable();
       int old_size = curr_space->GetNumPages() * PageSize;
       int new_size = old_size + shm_size;
       int num_pages = divRoundUp(new_size, PageSize);
       int old_num_pages = curr_space->GetNumPages();
       new_size = num_pages*PageSize;
       TranslationEntry *newPageTable = new TranslationEntry[num_pages];
       // Copy new page table
       for(int i =old_num_pages; i<num_pages; i++){
        newPageTable[i].virtualPage = i;
        newPageTable[i].physicalPage = currentThread->space->FindNextPage(-1);
        bzero(&machine->mainMemory[newPageTable[i].physicalPage*PageSize], PageSize);
        newPageTable[i].valid = TRUE;
        newPageTable[i].use = FALSE;
        newPageTable[i].dirty = FALSE;
        newPageTable[i].readOnly = FALSE;
        newPageTable[i].is_shared = TRUE;
        newPageTable[i].is_changed = FALSE;
        phy_to_pte[newPageTable[i].physicalPage] = &newPageTable[i];
        phy_to_pid[newPageTable[i].physicalPage] = currentThread->GetPID();
        lru_clock[newPageTable[i].physicalPage] = -1;
        stats->numPageFaults++;
       }
       // Copy old page table
       for(int i =0; i<curr_space->GetNumPages(); i++){
        newPageTable[i].virtualPage = i;
        newPageTable[i].physicalPage = oldPageTable[i].physicalPage;
        newPageTable[i].valid = oldPageTable[i].valid;
        newPageTable[i].use = oldPageTable[i].use;
        newPageTable[i].dirty = oldPageTable[i].dirty;
        newPageTable[i].readOnly = oldPageTable[i].readOnly;
        newPageTable[i].is_shared = oldPageTable[i].is_shared;
        newPageTable[i].is_changed = oldPageTable[i].is_changed;
        phy_to_pte[newPageTable[i].physicalPage] = &newPageTable[i];
        phy_to_pid[newPageTable[i].physicalPage] = currentThread->GetPID();
       }
       curr_space->SetNumPages(num_pages);
       curr_space->SetPageTable(newPageTable);
       machine->pageTable = newPageTable;
       machine->pageTableSize = num_pages;
       machine->WriteRegister(2,old_num_pages*PageSize);
       delete oldPageTable;
       DEBUG('t',"Allocated Shared Memory with numPagesAllocated = %d\n\n", numPagesAllocated);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_SemGet)){
       int key = machine->ReadRegister(4);
       int id = -1;
       for(int i=0; i<MaxSemCount;i++){
          if (id_key_sem_map[i] == key){
            id = i;
            break;
          }
       }
       if(id == -1){
          int empty_index = -1;
          IntStatus oldLevel = interrupt->SetLevel(IntOff);
          for(int i=0; i<MaxSemCount;i++){
            if (id_key_sem_map[i] == -1){
              empty_index = i;
              break;
            }
          }
          if(empty_index>=0){
            id_key_sem_map[empty_index] = key;
            id = empty_index;
            sem_list[empty_index] = new Semaphore("sema", 0);
          }
          (void) interrupt->SetLevel(oldLevel);
       }
       machine->WriteRegister(2,id);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_SemOp)){
       int id = machine->ReadRegister(4);
       int adjustment = machine->ReadRegister(5);
       if(id_key_sem_map[id] != -1){
        if(adjustment == -1){
          sem_list[id]->P();
        }else if(adjustment == 1){
          sem_list[id]->V();
        }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_SemCtl)){
       int id = machine->ReadRegister(4);
       int operation = machine->ReadRegister(5);
       int mem_addr = machine->ReadRegister(6);
       if(id_key_sem_map[id] != -1){
        if(operation == SYNCH_REMOVE){
          delete sem_list[id];
          id_key_sem_map[id] = -1;
          machine->WriteRegister(2,0);
        }else if(operation == SYNCH_GET){
          int main_mem_addr = machine->GetPA(mem_addr);
          if(main_mem_addr!=-1){
            machine->mainMemory[main_mem_addr] = sem_list[id]->getSemValue();
            machine->WriteRegister(2,0);
          }else{
            machine->WriteRegister(2,-1);
          }
        }else if(operation == SYNCH_SET){
          int main_mem_addr = machine->GetPA(mem_addr);
          if(main_mem_addr!=-1){
            IntStatus oldLevel = interrupt->SetLevel(IntOff);
            sem_list[id]->setSemValue(machine->mainMemory[main_mem_addr]);
            (void) interrupt->SetLevel(oldLevel);
            machine->WriteRegister(2,0);
          }else{
            machine->WriteRegister(2,-1);
          }
        }else{
          machine->WriteRegister(2,-1);
        }
       }else{
        machine->WriteRegister(2,-1);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_CondGet)){
       int key = machine->ReadRegister(4);
       int id = -1;
       for(int i=0; i<MaxCondCount;i++){
          if (id_key_cond_map[i] == key){
            id = i;
            break;
          }
       }
       if(id == -1){
          int empty_index = -1;
          IntStatus oldLevel = interrupt->SetLevel(IntOff);
          for(int i=0; i<MaxCondCount;i++){
            if (id_key_cond_map[i] == -1){
              empty_index = i;
              break;
            }
          }
          if(empty_index>=0){
            id_key_cond_map[empty_index] = key;
            id = empty_index;
            cond_list[empty_index] = new Condition("cond");
          }
          (void) interrupt->SetLevel(oldLevel);
       }
       machine->WriteRegister(2,id);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_CondOp)){
       int cond_id = machine->ReadRegister(4);
       int operation = machine->ReadRegister(5);
       int sem_id = machine->ReadRegister(6);
       DEBUG('l', "OK\n");
       if(id_key_sem_map[sem_id] != -1 && id_key_cond_map[cond_id] != -1){
        if (operation == COND_OP_WAIT){
          cond_list[cond_id]->Wait(sem_list[sem_id]);
        }else if (operation == COND_OP_SIGNAL){
          cond_list[cond_id]->Signal();
        }else if (operation == COND_OP_BROADCAST){
          cond_list[cond_id]->Broadcast();
        }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if((which == SyscallException) && (type == syscall_CondRemove)){
       int cond_id = machine->ReadRegister(4);
       if(id_key_cond_map[cond_id] != -1){
        delete cond_list[cond_id];
        machine->WriteRegister(2,0);
        id_key_cond_map[cond_id] = -1;
       }else{
        machine->WriteRegister(2,-1);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    }
    else if(which == PageFaultException){
      DEBUG('P', "Starting PageFaultException...\n");
      int virtAddr=machine->ReadRegister(BadVAddrReg);
      unsigned int vpn = (unsigned) virtAddr / PageSize;
      unsigned int offset = (unsigned) virtAddr % PageSize, pageFrame;
      TranslationEntry *pageTable = currentThread->space->GetPageTable();
      TranslationEntry *entry = &pageTable[vpn];

      int page_to_replace = currentThread->space->FindNextPage(-1);
      entry->physicalPage = page_to_replace;
      if(page_replacement_algo == FIFO){
        fifo->add_at_beginning(entry->physicalPage);
      }else if(page_replacement_algo == LRU){
        lru->add_at_beginning(entry->physicalPage);
      }else if(page_replacement_algo == LRUCLOCK){
        lru_clock[entry->physicalPage] = 1;
      }      
      phy_to_pte[entry->physicalPage] = entry;
      phy_to_pid[entry->physicalPage] = currentThread->GetPID();

      DEBUG('T', "Started copying memory \n");
      // Memory Copy
      bzero(&machine->mainMemory[entry->physicalPage*PageSize], PageSize);
      if(entry->is_changed == TRUE){
        DEBUG('T', "Copying memory from fallback Memory\n");
        for(int j=0; j<PageSize;j++){
          machine->mainMemory[entry->physicalPage*PageSize+j] = currentThread->fallMem[entry->virtualPage*PageSize+j];
        }
      }else{
        OpenFile *executable = fileSystem->Open(currentThread->space->exec_filename);
        NoffHeader noffH;
        executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
          SwapHeader(&noffH);

        int size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
              + UserStackSize;
        int new_numPages = divRoundUp(size, PageSize);
        size =  new_numPages * PageSize; 

        char *exec_cont = new char[size];
        bzero(exec_cont,size);
        // Sir Code
        if (noffH.code.size > 0) {
          vpn = noffH.code.virtualAddr/PageSize;
          offset = noffH.code.virtualAddr%PageSize;
          pageFrame = entry->physicalPage;
          executable->ReadAt(&(exec_cont[vpn * PageSize + offset]),
                  noffH.code.size, noffH.code.inFileAddr);
        }
        if (noffH.initData.size > 0) {
            vpn = noffH.initData.virtualAddr/PageSize;
            offset = noffH.initData.virtualAddr%PageSize;
            pageFrame = entry->physicalPage;
            executable->ReadAt(&(exec_cont[vpn * PageSize + offset]),
                    noffH.initData.size, noffH.initData.inFileAddr);
        }
        for(int i=0;i<PageSize;i++){
          machine->mainMemory[entry->physicalPage*PageSize+i]=exec_cont[entry->virtualPage*PageSize+i];
        }
        delete exec_cont;
        delete executable;
      }
      DEBUG('P', "Page replacement finished with %d \n",numPagesAllocated);
      entry->valid = TRUE;
      DEBUG('P', "Finishing PageFaultException...\n");
      stats->numPageFaults++;
      if(!entry->is_shared)
        currentThread->SortedInsertInWaitQueue (1000+stats->totalTicks);
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
