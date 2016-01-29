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
#include "addrspace.h"
#include "synchop.h"

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
// int ctr=0;
// char string[1000][100];

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
    unsigned Sharedsize;
    int givenkey;   // Used in SemGet
    int givenadjust;  //Used in SemOp
    int givensemid;   //Used in SemOp
    int givencondid;
    int givenop;
    char* a;
    unsigned newsemid;  //Used in SemCtl
    int givencommand;   //Used in SemCtl
    int newval;        //Used in SemCtl
    unsigned SharedMemoryStart; //used in Shm_allocate
    int NumberofSharedpages;
    char buffer[1024];		// Used in syscall_Exec
    int waitpid;		// Used in syscall_Join
    int whichChild;		// Used in syscall_Join
    NachOSThread *child;		// Used by syscall_Fork
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
       while(!machine->ReadMem(vaddr, 1, &memval));
       i = 0;
       while ((*(char*)&memval) != '\0') {
          buffer[i] = (*(char*)&memval);
          i++;
          vaddr++;
          while(!machine->ReadMem(vaddr, 1, &memval));
       }
       buffer[i] = (*(char*)&memval);
       StartProcess(buffer);
    }
    else if ((which == SyscallException) && (type == syscall_Join)) {
       waitpid = machine->ReadRegister(4);
       // printf("Entered Join.............\n");
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
       // printf("Leaving Join.....................\n");
    }
    else if ((which == SyscallException) && (type == syscall_Fork)) {
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       // char *newname = currentThread->filename;
       // for (int i = 0;  newname[i]!='\0'; ++i)
       // {
       //    string[ctr][i] = newname[i];
       // }
       child = new NachOSThread("Forked Thread", GET_NICE_FROM_PARENT);
       // ctr++;
       // printf("Addrspace before!\n");
       child->space = new AddrSpace (currentThread->space);  // Duplicates the address space
       // printf("Addrspace after.\n");
       child->SaveUserState ();		     		      // Duplicate the register set
       child->ResetReturnValue ();			     // Sets the return register to zero
       child->ThreadStackAllocate (ForkStartFunction, 0);	// Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->GetPID());		// Return value for parent
       // printf("Exiting FORK()\n");
    }
    else if ((which == SyscallException) && (type == syscall_Yield)) {
       currentThread->YieldCPU();
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
       while(!machine->ReadMem(vaddr, 1, &memval));
       
       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          while(!machine->ReadMem(vaddr, 1, &memval));
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
          currentThread->YieldCPU();
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
    }

    /////////////////////// My code //////////////////////////////////////////////////////////////////////////////
    else if ((which == SyscallException) && (type == syscall_ShmAllocate)) {
	// printf("Im running\n");
      /*My code starts here*/
      Sharedsize=machine->ReadRegister(4);
      // first, set up the translation 
      int NumberofSharedpages=0;
      
      AddrSpace* NewSpace;
      currentThread->space->Addrspace(Sharedsize, &NumberofSharedpages, currentThread->space);  //Created new page table.
      // printf("Return from AddrSpace\n");
      // currentThread->space=NewSpace; 
      TranslationEntry * pT = currentThread->space->GetPageTable();
      // printf("physicalPage %d %d\n",pT[22].physicalPage*PageSize, pT[22].virtualPage*PageSize);
      // printf("number of pages %d\n", currentThread->space->GetNumPages());
            // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

       machine->WriteRegister(2, NumberofSharedpages);           //return the address of the first shared page
    } 
    else if ((which == SyscallException) && (type == syscall_SemGet)) {
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // (void) interrupt->SetLevel(oldLevel);
       givenkey=machine->ReadRegister(4);
       i=0;

       while(1){                     //search for the key with the given key
        printf("HIHIHI\n");
        if (i==10)                                          //If the key is not present in the array then break.
        {
          break;/* code */
        }
        if(arsem[i].VALID==0){                                          
          i++;
          continue;
        }
        if(arsem[i].VALID==1){
         if(givenkey == arsem[i].key){                      //If u found the key return i
          break;
         } 
         else{
          i++;
          continue;
         }
        }
       } 

       if (i==10)                                           // after break create a new sem node with
       {                                                    // the key and insert it into the array
        for(i=0; i<10; i++){
          // printf("printing arsem[%d] = %d\n", i, arsem[i].VALID);
          if(arsem[i].VALID==1){
            continue;
          }

          if(arsem[i].VALID==0){
            arsem[i].key=givenkey;
            arsem[i].s= new Semaphore(a, 1);
            arsem[i].VALID= 1;
            break;
          }
        } 
       }

       (void) interrupt->SetLevel(oldLevel);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       machine->WriteRegister(2, i);
    }

    else if ((which == SyscallException) && (type == syscall_SemOp)) {
       
       int givenadjust=machine->ReadRegister(5);
       int givensemid=machine->ReadRegister(4);

       //if -1 is passed then a P() function is implemented on the semaphore
       if(givensemid>=10){
        machine->WriteRegister(2,-1);
       }
       else{
        machine->WriteRegister(2,0);
        if(givenadjust==-1){
          arsem[givensemid].s->P();
        }

       //if 1 is passed then a V() function is implemented on the semaphore

        if(givenadjust==1){
          arsem[givensemid].s->V();
        }
      }

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if ((which == SyscallException) && (type == syscall_SemCtl)) {
       // IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // (void) interrupt->SetLevel(oldLevel);
       newsemid=(unsigned)machine->ReadRegister(4); 
       givencommand= machine->ReadRegister(5);  
       int semaddr = machine->ReadRegister(6);  
       while(!machine->ReadMem(semaddr, 4, &newval));
       // printf("Conversion complete\n");
       if (givencommand==0)
       {
        arsem[newsemid].VALID==0; /* code */
        delete arsem[newsemid].s;
        arsem[newsemid].s==NULL;
       }

       if (givencommand==1)
       {
        newval=arsem[newsemid].s->getvalue(); /* code */
      }

       if (givencommand==2)
       {
         // printf("Setting value for arsem[newsemid].s\n");
         arsem[newsemid].s->setvalue(newval);
         // printf("arsem[newsemid].s is %d\n",arsem[newsemid].s->getvalue()); /* code */
       }

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if ((which == SyscallException) && (type == syscall_CondGet)) {
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // (void) interrupt->SetLevel(oldLevel);
       givenkey=machine->ReadRegister(4);
       i=0;

       while(1){   
       if (i==10)                                          //If the key is not present in the array then break.
        {
          break;/* code */
        }        
                                      //search for the key with the given key
         if(arcond[i].VALID==0){
          i++;
          continue;
        }
        if(arcond[i].VALID==1){
         if(givenkey == arcond[i].key){
          // printf("Key match for %d\n", i);
          break;
         } 
         else{
          i++;
          continue;
         }
        }
      }
       if (i==10)                                           // after break create a new sem node with
       {                                                    // the key and insert it into the array
        for(i=0; i<10; i++){
          // printf("VALID  of %d is %d\n", i, arcond[i].VALID);
          if(arcond[i].VALID==1){
            continue;
          }
          if(arcond[i].VALID==0){
            arcond[i].key=givenkey;
            arcond[i].c= new Condition(a);
            // printf("Setting VALID for %d\n", i);
            arcond[i].VALID= 1;
            break;
          }
        } 
       }
	
       // IntStatus oldLevel = interrupt->SetLevel(IntOff);
      (void) interrupt->SetLevel(oldLevel);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       // IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // (void) interrupt->SetLevel(oldLevel);
       machine->WriteRegister(2, i);
    }

    else if ((which == SyscallException) && (type == syscall_CondOp)) {
       
       int givenop=machine->ReadRegister(5);
       int givencondid=machine->ReadRegister(4);
       int givensemid=machine->ReadRegister(6);

       
       if (givencondid>=10 || givensemid>=10)
       {
         machine->WriteRegister(2,-1);
       }
       else
       {
        machine->WriteRegister(2,0);    //It was succesful
        if(givenop==0)
        {
          arcond[givencondid].c->Wait(arsem[givensemid].s);
        }

        //if 1 is passed then a Signal() function is implemented on the semaphore
        //if 2 is passed then a Broadcast() function is implemented on the semaphore
        if(givenop==1)
        {
          arcond[givencondid].c->Signal();
        }

        if(givenop==2)
        {
          arcond[givencondid].c->Broadcast();
        }
      }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if ((which == SyscallException) && (type == syscall_CondRemove)) {
      int givencondid = machine->ReadRegister(4);
      
      if(givencondid>=10 || arcond[givencondid].VALID==0){
        machine->WriteRegister(2, -1);
      }else{
        arcond[givencondid].VALID = 0;
        delete arcond[givencondid].c;
        arcond[givencondid].c = NULL;
      }

      // Advance program counters.
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if (which == PageFaultException){
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // (void) interrupt->SetLevel(oldLevel);
      // printf("PageFaultException..............\n");

      stats->numPageFaults++;
      unsigned virtualpagenumber;
      TranslationEntry *PageTable;
      int addr;

      addr=machine->ReadRegister(39);
      virtualpagenumber=addr/PageSize;
      PageTable = currentThread->space->GetPageTable();

      OpenFile *exec = fileSystem->Open(currentThread->filename);
      // printf("CURRENT THREAD FILENAME : %s\n", currentThread->filename);

      NachOSThread* thread=currentThread;
      

      PageTable[virtualpagenumber].physicalPage = numPagesAllocated;
      PageTable[virtualpagenumber].valid = TRUE;
      bzero(&machine->mainMemory[numPagesAllocated*PageSize], PageSize);

      exec->ReadAt(&(machine->mainMemory[numPagesAllocated * PageSize]), PageSize, virtualpagenumber*PageSize+40);
    
      // delete exec;  
      numPagesAllocated++;
      // IntStatus oldLevel = interrupt->SetLevel(IntOff);
      // printf("CURRENT THREAD FILENAME : %s\n", currentThread->filename);
      (void) interrupt->SetLevel(oldLevel);
      // printf("CURRENT THREAD FILENAME : %s\n", currentThread->filename);
      currentThread->SortedInsertInWaitQueue(1000 + stats->totalTicks); 
    }     
      
      
       else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
