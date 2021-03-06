README

1.GetReg
In the implementation of this syscall first we read argument passed in register $4 and then we copied the content of that register into register $2 to return the value of that register.

2.GetPA
Returns the physical address corresponding to the virtual address
passed as argument. 
	First we check condition that the virtual page number is not larger than the number of entries in the page table, here virtual page number is argument passed in register 4 divided by total page size and machine->pageTableSize givews us number of entries in page table. 
	Secondly, we checked that the page table entry has the valid field set to true indicating that the entry holds a valid physical page number.
	Also, if obtained physical page number i.e  (machine->pageTable[machine->ReadRegister(4)/PageSize].physicalPage) is greater than the number of physical pages then we returned -1.
Otherwise we used machine->Translate to obtain physical address from the virtual address.

3.GetPID
We defined a varible MaxCount of type static int and incremented it every time when a new thread is created and value of the MaxCount is assigned to pid. This ensures every thread gets a unique pid.

4.GetPPID
In this sysscall implementation we are checking if the current thread which is creating the new thread is NULL or not. If it is NULL then we are assigning -1 to ppid of the new thread. Otherwise we are assigning value of the current thread's pid as the ppid of the new thread which is being created.

5.GetNumInstr
We defined a variable instructions in the thread class. When a thread is created we assign instructions=0. And then we are incrementing this variable at every interrupt by adding number of UserTick to it.

6.Time
In this we returned number of totalTicks in register 2.

7.Yield
First we are disabling the interrupt. Then we are finding next thread to run using scheduler->FindNextToRun() if it is not null then we  schedule this thread to run using scheduler->Run(nextThread). We again enable the interrupt.

8.Sleep
First we are taking the argument passed in the register 4. if that argument is 0 then we called yield function on current thread.
Also we made a globaL list sleepQ which is sorted list according to time of wakeup.
We defined a varible nticks in which we stored the time when wee have to wake that sleeping thread. Then we inserted this thread into the sleepQ and then we called sleep function on this current thread. 
Also in the function TimerInterruptHandler() we found out which threads need to be woke up by comparing current time with the time stored in that queue 
(key<=stats->totalTicks) and we moved all this threads to ready queue by using 
 scheduler->ReadyToRun() function.

9.Exec
In this, we first read the filename from mainMemory using the address supplied as argument. We contine reading it until we encounter a NULL. After this, we addrspace for the currently running thread. Then, we call StartProcess on the file to execute it.

10.Exit
First we take exitcode passed as an argument to sys_Exit() call.
We defined arrays:
 a. threadExitArr : which has 3 states, state -1 corresponds to uninitialised threds, state 0 corresponds to threads which are initialised but not yet exited and state 1 to threads which are exited.
 b. threadExitCode : Which stores the exit code for the threads which have exited with index as pid.
 c.sleptby : This array stores the pid of thread due to which the thread has been in the sleep state. This is initialised in the sys_join syscall.

First we check if the thread which we are exiting is the last thread to be exited or not, if that is the case then we call  interrupt->Halt().
Otherwise first we wake the thread which is slept due to the current thread and then call the function currentThread->Finish() on current thread.

11.Join
First we stored the childpid which is passed as ana argument. we have definedd an array "childpidArray" which stores all the child threads pid of the given parent thread. Now we check that whether the pid passed in the argument is legitimate child of the current thread by searching in the childpid array.
If it is not a legitimate child we return -1.
Otherwise we check if that child is exited or not using threadExitArr,
if that child is exited we return its exitcoode stored in threadExitCode array.
If that child is not yet exited then we updated value of array sleptby and then used currentThread->Sleep() to sleep the current thread.

12.Fork
We create a child thread after advancing the program counters.
Then, we create an AddrSpace for the child process and copy the parent's address space to the child's space. (We used function overloading to acheive this).
After this, we duplicate the register state to child process and set the return value for child process to 0 (by storing 0 in Register 2 for child process).
After this, we call StackAllocate on the child process as desciped in assignment. The function passed in argument restores state and then runs the machine.
After this, we add the child process to ReadyToRun. (Interuupt is set of before putting the child process in ReadyToRun and the state is restored later).
Then, we return child's PID for parent process. 
Note : We are also maintaing a global variable for storing the number of pages allocated so far.  When a new process is created, memory is allocated after the memory that has already been allocated.


NOTE : Please ensure that the amount of simulated physical memory is sufficient to run the program. In case not, decrease input size or increase simulated memory.