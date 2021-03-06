// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List; 
    
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    if(thread->getStatus() == RUNNING)
    {
        int burst = stats->totalTicks-curr_cpu_burst_start_time;
        if(burst > 0){
            stats->cpu_busy_time+= burst;
            stats->burst_count++;
            if(burst < stats->burst_min)
                stats->burst_min=burst;
            if(burst > stats->burst_max)
                stats->burst_max=burst;
            if(sched_algo == NP_SJF){
                int error = burst - thread->expected_tau;
                if(error<0){
                    stats->sjf_error += (error*-1);
                }else{
                    stats->sjf_error += error;
                }
                thread->expected_tau = (int)(0.5*burst + 0.5*thread->expected_tau);
            }
        }
    }

    thread->setStatus(READY);
    thread->ready_queue_wait_start=stats->totalTicks;
    readyList->Append((void *)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    if (sched_algo== UNIX) {
       return (Thread *)readyList->GetBestThread();
    }
    if (sched_algo == NP_SJF){
        return (Thread *)readyList->GetBestThread_SJF();
    }
    else {
       return (Thread *)readyList->Remove();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;

    curr_cpu_burst_start_time=stats->totalTicks;
    stats->wait_time+= stats->totalTicks-nextThread->ready_queue_wait_start;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    _SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Tail
//      This is the portion of Scheduler::Run after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
Scheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

void Scheduler::NewThreadPriority()
{
    int i, curentCpuUsage, curr_thread_priority;
    int burst=stats->totalTicks-curr_cpu_burst_start_time;
    if (burst > 0){
        for(i=0;i<thread_index;i++){
            if(!exitThreadArray[i]){
                if( i!= currentThread->GetPID() ){
                    curentCpuUsage = threadArray[i]->cpuusage;
                    curentCpuUsage = curentCpuUsage/2;
                    curr_thread_priority = threadArray[i]->basePriority + (curentCpuUsage/2);
                    threadArray[i]->cpuusage = curentCpuUsage;
                    threadArray[i]->priority = curr_thread_priority;
                }else if( i == currentThread->GetPID()){
                    curentCpuUsage = (currentThread->cpuusage+burst)/2;
                    curr_thread_priority = currentThread->basePriority + (curentCpuUsage/2);
                    currentThread->cpuusage = curentCpuUsage;
                    currentThread->priority = curr_thread_priority;
                }
            }
        }
    }
}