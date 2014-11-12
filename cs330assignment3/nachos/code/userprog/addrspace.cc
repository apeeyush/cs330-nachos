// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

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

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

//    ASSERT(numPages+numPagesAllocated <= NumPhysPages);		// check we're not trying
										// to run anything too big --
										// at least until we have
										// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = -1;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    pageTable[i].is_shared = FALSE;
    pageTable[i].is_changed = FALSE;
    }

    currentThread->fallMem = new char[size];
}


AddrSpace::AddrSpace(AddrSpace *parentSpace, int child_pid)
{
    numPages = parentSpace->GetNumPages();
    unsigned i, size = numPages * PageSize;

    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];

    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = FALSE;
        pageTable[i].is_changed = FALSE;
    }
}


//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------
void
AddrSpace::AddrSpaceInitialize(AddrSpace *parentSpace, int child_pid)
{
    numPages = parentSpace->GetNumPages();
    unsigned i, size = numPages * PageSize;

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                        numPages, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];

    strcpy(exec_filename, parentSpace->exec_filename);
    noffH = parentSpace->noffH;

    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].is_shared = parentPageTable[i].is_shared;
        if(parentPageTable[i].is_shared == FALSE){
            if(parentPageTable[i].valid == TRUE){
                int page_to_replace = FindNextPage(parentPageTable[i].physicalPage);
                pageTable[i].physicalPage = page_to_replace ;
                // SetUp Back pointers
                phy_to_pte[pageTable[i].physicalPage] = &pageTable[i];
                phy_to_pid[pageTable[i].physicalPage] = child_pid;
                // Copy the contents
                unsigned localStartAddrParent = parentPageTable[i].physicalPage*PageSize;
                unsigned localStartAddrChild = pageTable[i].physicalPage*PageSize;
                for (int j=0; j<PageSize; j++) {
                    machine->mainMemory[localStartAddrChild+j] = machine->mainMemory[localStartAddrParent+j];
                }
                DEBUG('F', "Allocating Memory Completed!!\n\n");
            }else{
                pageTable[i].physicalPage = -1;
            }
        }else{
            pageTable[i].physicalPage = parentPageTable[i].physicalPage;
        }
        pageTable[i].valid = parentPageTable[i].valid;
        pageTable[i].use = parentPageTable[i].use;
        pageTable[i].dirty = parentPageTable[i].dirty;
        pageTable[i].readOnly = parentPageTable[i].readOnly;
        pageTable[i].is_changed = parentPageTable[i].is_changed;
        for(int k=0;k<PageSize;k++){
            threadArray[child_pid]->fallMem[i*PageSize+k]=currentThread->fallMem[i*PageSize+k];
        }
    }
    DEBUG('P', "Exiting Fork!!\n\n");
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    for(int i=0; i<numPages;i++){
        if(pageTable[i].valid){
            phy_to_pte[pageTable[i].physicalPage] = NULL;
            phy_to_pid[pageTable[i].physicalPage] = -1;
        }
    }
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}
void
AddrSpace::SetNumPages(int new_num_pages)
{
    numPages=new_num_pages;
}


TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}

void
AddrSpace::SetPageTable(TranslationEntry* new_page_table)
{
    pageTable=new_page_table;
}

int
AddrSpace::PageReplacement(int parentPhyPage){
    DEBUG('F', "Memory Full!! Need Page Replacement\n");
    int *phy_page_to_replace;
    TranslationEntry *page_entry;
    if (page_replacement_algo == RANDOM){
        int tmp = Random()%NumPhysPages;
        while(parentPhyPage == tmp || phy_to_pte[tmp]->is_shared){
            tmp = Random()%NumPhysPages;
        }
        phy_page_to_replace = &tmp;
    }else if(page_replacement_algo == FIFO){

    }
    
    DEBUG('L', "Page to replace : %d, Page PID : %d \n", *phy_page_to_replace, phy_to_pid[*phy_page_to_replace]);
    page_entry = phy_to_pte[*phy_page_to_replace];
    page_entry->valid = FALSE;
    int other_pid = phy_to_pid[*phy_page_to_replace];
    Thread *thread = threadArray[other_pid];
    if(page_entry->dirty) {
        DEBUG('F', "Copying parent data in backup...");
        page_entry->is_changed = TRUE;
        for(int j=0; j<PageSize; j++) {
            thread->fallMem[page_entry->virtualPage*PageSize+j] = machine->mainMemory[page_entry->physicalPage*PageSize+j];
        }
    }
    return page_entry->physicalPage;
    DEBUG('F', "Getting Out after successfull replacement from Fork!!\n");
}

int
AddrSpace::FindNextPage(int parentPhyPage){
    if(numPagesAllocated == NumPhysPages && unallocated_pages->IsEmpty() ){
        int page_to_replace = PageReplacement(parentPhyPage);
        return page_to_replace;
    }else{
        int *phy_page_num = (int *)unallocated_pages->Remove();
        if (phy_page_num != NULL){
            return *phy_page_num;
        }else{
            numPagesAllocated++;
            return numPagesAllocated-1;
        }
    }       
}