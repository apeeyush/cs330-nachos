// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "stats.h"
#include "system.h"
//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    cpu_busy_time=0;
    wait_time=0;
    start_time=0;
    burst_max=0;
    burst_min=100000000;
    burst_count=0;
    end_time=0;
    total_completion_time=0;  
    max_completion_time=0;
    min_completion_time=100000000; 
    num_thread=0;
    sum_square_completion_time = 0;
    sjf_error = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
    printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks, 
	idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd, 
	numPacketsSent);
    printf("Total CPU busy time: %d\n", cpu_busy_time);
   
    printf("Total execution time: %d\n", end_time-start_time);
    printf("CPU Utilization: %f\n", (float)cpu_busy_time/(end_time-start_time));
    printf("Maximum cpu burst time: %d\n", burst_max);
    printf("Minimum cpu burst time: %d\n", burst_min);
    printf("Avg cpu burst time: %f\n", (float)cpu_busy_time/burst_count);
    printf("Avg wait time: %f\n", (float)wait_time/(num_thread+1));
    printf("Nonzero burst count: %d\n", burst_count);
    printf("Maximum completion time: %d\n", max_completion_time);
    printf("Minimum  completion time: %d\n", min_completion_time);
    printf("Average completion time: %f\n", (float)total_completion_time/num_thread);
    printf("sum_square_completion_time: %lld\n",sum_square_completion_time);
    printf("total_completion_time: %f\n",(float)(total_completion_time/num_thread*total_completion_time) );
    float temp= (float)(sum_square_completion_time - (total_completion_time/num_thread*total_completion_time));
    printf("Variance: %f\n", (float)temp/num_thread); 
    if(sched_algo == NP_SJF){
        printf("SJF estimated error = %f", ((float)sjf_error)/cpu_busy_time*100);
    }
}
