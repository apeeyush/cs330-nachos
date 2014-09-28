// system.h 
// All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

#define MAX_THREAD_COUNT 1000

#define NP        1                    // Non-preemptive scheduling algorithm
#define NP_SJF    2                    // Non-preemptive with burst estimation (Non-preemptive shortest next CPU burst first)
#define RR        3                    // Round Robin
#define UNIX      4                    // Unix scheduler


extern int sched_algo;                 // Variable for storing scheduling algorithm to use

extern int TimerTicks;                  // (average) time between timer interrupts


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv);  // Initialization,
                  // called before anything else
extern void Cleanup();           // Cleanup, called when
                  // Nachos is done.

extern Thread *currentThread;       // the thread holding the CPU
extern Thread *threadToBeDestroyed;       // the thread that just finished
extern Scheduler *scheduler;        // the ready list
extern Interrupt *interrupt;        // interrupt status
extern Statistics *stats;        // performance metrics
extern Timer *timer;          // the hardware alarm clock
extern unsigned numPagesAllocated;              // number of physical frames allocated

extern Thread *threadArray[];          // Array of thread pointers
extern unsigned thread_index;                  // Index into this array (also used to assign unique pid)
extern bool initializedConsoleSemaphores;       // Used to initialize the semaphores for console I/O exactly once
extern bool exitThreadArray[];          // Marks exited threads
extern int curr_cpu_burst_start_time;
class TimeSortedWaitQueue {             // Needed to implement syscall_Sleep
private:
   Thread *t;                           // Thread pointer of the sleeping thread
   unsigned when;                       // When to wake up
   TimeSortedWaitQueue *next;           // Build the list

public:
   TimeSortedWaitQueue (Thread *th,unsigned w) { t = th; when = w; next = NULL; }
   ~TimeSortedWaitQueue (void) {}

   Thread *GetThread (void) { return t; }
   unsigned GetWhen (void) { return when; }
   TimeSortedWaitQueue *GetNext(void) { return next; }
   void SetNext (TimeSortedWaitQueue *n) { next = n; }
};

extern TimeSortedWaitQueue *sleepQueueHead;

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;   // user program memory and registers
#endif

#ifdef FILESYS_NEEDED      // FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H