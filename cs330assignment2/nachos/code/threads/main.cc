// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"


// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);

//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

void
BatchStartFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

void run_thread(char* executable){
	OpenFile *inFile = fileSystem->Open(executable);
    if (inFile == NULL) {
    	printf("Unable to open file %s\n", executable);
        return;
    }
    Thread *child = new Thread("batch_thread");
    child->space = new AddrSpace (inFile);
    delete inFile;
    child->space->InitRegisters();             // set the initial register values
    child->SaveUserState ();
    child->StackAllocate (BatchStartFunction, 0);
    child->Schedule ();
}

int
main(int argc, char **argv)
{
    int argCount;			// the number of arguments 
					// for a particular command

    DEBUG('t', "Entering main");
    (void) Initialize(argc, argv);
    
#ifdef THREADS
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // print copyright
            printf (copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) {        	// run a user program
	    ASSERT(argc > 1);
            StartProcess(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-c")) {      // test the console
	    if (argc == 1)
	        ConsoleTest(NULL, NULL);
	    else {
		ASSERT(argc > 2);
	        ConsoleTest(*(argv + 1), *(argv + 2));
	        argCount = 3;
	    }
	    interrupt->Halt();		// once we start the console, then 
					// Nachos will loop forever waiting 
					// for console input
	} else if(!strcmp(*argv, "-F")){		// Assignment 2 -- Batch
            ASSERT (argc > 1);
            argCount = 2;

	    	char *FileName = *(argv+1);
	    	OpenFile *openFile = fileSystem->Open(FileName);

		    if (openFile == NULL) {
				printf("Unable to open file %s\n", FileName);
				delete openFile;
			}
			else{
				char *buffer = new char[1000];
				int numBytes = openFile->Read(buffer, 1500);
			   	buffer[numBytes] = '\0';
			   	char *buf_pos = buffer;
			   	// Read Scheduling Algorithm and store it in the global variable
				sched_algo = 0;
				while (*buf_pos != '\n' && *buf_pos != '\0'){
					sched_algo = 10*sched_algo + (*buf_pos - '0');
					buf_pos++;
				}
				// Read batch processes and schedule them
			   	while(*buf_pos != '\0'){
			   		while (*buf_pos != '\0' && *buf_pos != '\n'){
			   			char executable[1000];
				   		int exec_priority = 0;
			   			int pos = 0;
			   			while (*buf_pos != ' ' && *buf_pos != '\n'){
				   			executable[pos] = *buf_pos;
				   			buf_pos++; pos++;
				   		}
				   		executable[pos] = '\0';
				   		if(*buf_pos == '\n'){
				   			exec_priority = 100;
				   		}else{
				   			buf_pos++;
							while (*buf_pos != '\0' && *buf_pos != '\n'){
				   				exec_priority = exec_priority*10 + (*buf_pos - '0');
				   				buf_pos++;
				   			}
				   		}
				   		printf("%s %d\n", executable, exec_priority);
				   		run_thread(executable);
			   		}
			   		if (*buf_pos == '\n'){
				   		buf_pos++;
			   		}
		   		}
		    	delete [] buffer;
		    	delete openFile;	// close file
			}
		int i, exitcode;
		exitcode = 0;
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
#endif // USER_PROGRAM
#ifdef FILESYS
	if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
	    ASSERT(argc > 2);
	    Copy(*(argv + 1), *(argv + 2));
	    argCount = 3;
	} else if (!strcmp(*argv, "-p")) {	// print a Nachos file
	    ASSERT(argc > 1);
	    Print(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-r")) {	// remove Nachos file
	    ASSERT(argc > 1);
	    fileSystem->Remove(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-l")) {	// list Nachos directory
            fileSystem->List();
	} else if (!strcmp(*argv, "-D")) {	// print entire filesystem
            fileSystem->Print();
	} else if (!strcmp(*argv, "-t")) {	// performance test
            PerformanceTest();
	}
#endif // FILESYS
#ifdef NETWORK
        if (!strcmp(*argv, "-o")) {
	    ASSERT(argc > 1);
            Delay(2); 				// delay for 2 seconds
						// to give the user time to 
						// start up another nachos
            MailTest(atoi(*(argv + 1)));
            argCount = 2;
        }
#endif // NETWORK
    }

    currentThread->Finish();	// NOTE: if the procedure "main" 
				// returns, then the program "nachos"
				// will exit (as any other normal program
				// would).  But there may be other
				// threads on the ready list.  We switch
				// to those threads by saying that the
				// "main" thread is finished, preventing
				// it from returning.
    return(0);			// Not reached...
}
