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
//#include "machine.h"
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
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;
   
    /*System Call Halt*/
    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
   /*System Call PrintInt*/
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
       //printf("If this works..................\n");
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

   /*System Call Print_Char*/
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

   /*System Call PrintString*/
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
          //printf("En 1.......\n");
	         writeDone->P() ;
           //printf("En 2.......\n");
          console->PutChar(*(char*)&memval);
          //printf("En 3.......\n");
          vaddr++;
          //printf("En 4.......\n");
          machine->ReadMem(vaddr, 1, &memval);
         // printf("En 5.......\n");
       }
       // Advance program counters.
       //printf("Divyanshu\n");
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

    /*System Call GetReg*/
    else if ((which == SyscallException) && (type == syscall_GetReg)) {
      int regno = machine->ReadRegister(4);
      int valreg = machine->ReadRegister(regno);
      machine->WriteRegister(2,valreg);
      //Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

   /*System Call GetPA*/
    else if ((which == SyscallException) && (type == syscall_GetPA)) {
	/*The flag varible was extra precaution added at the beginning. however over time, the code got shorter and simpler. Incase flag is not removed in the final verion, consider that to be the consequence of the laziness of the programmer and excuse him(me) for the same.*/
        unsigned virtAddr = machine->ReadRegister(4);
        unsigned int vpn, offset;
        unsigned int pageFrame;
        int i, flag=0;  //copied variables from machine->Translate() in /nachos/code/machine/translate.cc
	      //machine->TranslationEntry *entry;
        vpn = virtAddr/PageSize;	//virtual page number
	      offset = virtAddr%PageSize;	
      	TranslationEntry *entry = &(machine->pageTable[vpn]);	//the pointer to the page table entry
      	pageFrame = entry->physicalPage;         //physicalPage field of the pageTable entry
      	int physicalAddress = pageFrame*PageSize + offset;   //calculating PhysAddr just in case..
      	int retval=-1;					//return value for error
      	if (vpn > PageSize){		//vpn exceeded page size!
      		flag=1;					//set flag for error
      		machine->WriteRegister(2, retval);
      	} 
      	else if(pageFrame > 32){		//physical page number exceeded number of physical pages! value of NumPhysPages obtained from /machine/machine.h
      		flag=1;						//set flag for error
      		machine->WriteRegister(2, retval);
      	}
      	else if(!(entry->valid)){		//invalid entry
      				flag=1;			//set flag for error
      				machine->WriteRegister(2, retval);
      	}
      	if(flag==0)					//if no error
      		machine->Translate(virtAddr, &physicalAddress, 1, FALSE);     //Translate does the job for us. Read Only mode.. send ans to
                                                              //addr of var "physicalAddress" and send me 1 byte.. just for safekeeping
            //Advance program counters.
             machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
             machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
             machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    /*System Call GetPID*/
    else if((which == SyscallException) && (type == syscall_GetPID))
	{
		int currpid = currentThread->PID();	//updating field as instructed
		machine->WriteRegister(2, currpid);	
		
		//Advance Program Counters
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}

   /*System Call GetPPID*/
   else if((which == SyscallException) && (type == syscall_GetPPID))
	{
		int currppid = currentThread->PPID();	 //updating field as instructed
		machine->WriteRegister(2, currppid);
		
		//Advance Program Counters
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}
	/*System Call GetNumInstr*/

	else if((which == SyscallException) && (type == syscall_NumInstr))
    	{
		int numberexecuted = machine->InstrExecuted;
		machine->WriteRegister(2, numberexecuted);
		//Advance Program Counters
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}

  /*System Call Yield*/
    else if ((which == SyscallException) && (type == syscall_Yield))
    {
        //Advance Program Counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        currentThread->YieldCPU();
      }

    /*System Call Time*/

   else if((which == SyscallException) && (type == syscall_Time))
    	{
		int ticks=stats->totalTicks; //found stats->totalTicks in system.cc
		machine->WriteRegister(2, ticks);
		//Advance Program Counters
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}
   
   /*System Call Sleep*/

   else if((which == SyscallException) && (type == syscall_Sleep))
	{
		unsigned int ticks = machine->ReadRegister(2);		//After how much time do I want to wakt this thread up.
		int wakeuptime = ticks + stats->totalTicks; 		//Instead of using ticks and decrememting everytime
															//We use absolute time of wakeup defined thus.
		//Advance Program Counters before sleeping/yielding
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
		if(ticks == 0){
			currentThread->YieldCPU();		//Give CPU to another thread.
		}
		//Enqueue (SortedInsert) current thread(currentThread) into SleepQueue with sortkey as ticks. 
		//Then put thread to sleep like done in other files.
		else{
			SleepQueue->SortedInsert(currentThread, wakeuptime);

			IntStatus oldLevel = interrupt->SetLevel(IntOff); //Turn off interrupts as instructed before PutThreadToSleep()
			currentThread->PutThreadToSleep();		//Putting Thread to Sleep...
			(void) interrupt->SetLevel(oldLevel);	//Reset interrupt status.
		}
		/*Check the rest of the code on ...../nachos/code/threads/system.cc under TimerInterruptHandler()*/
	}

  /*System Call Exec*/

  else if((which == SyscallException) && (type == syscall_Exec))
  {
    //NachOSThread *newthread;
    char newfilename[50];    //will store filename
    int i;          //index
    /*Copying and modifying code from printstring to translate integer val to newfilename*/
    int memaddr = machine->ReadRegister(4);
       machine->ReadMem(memaddr, 1, &memval);   //read first.
       while ((*(char*)&memval) != '\0') {    //If null.. exit
          newfilename[i] = memval;  //write char read to array pos.
          i++;    //next index
          memaddr++;   //next addr
          machine->ReadMem(memaddr, 1, &memval);    //read next...
       }
       newfilename[i] = memval; //Put the \0 at the end...


    /*We have the filename in newfilename with a \0 in the end. Now use code from StartProcess()....*/
    OpenFile *newexecutable = fileSystem->Open(newfilename);    //open the file.
    AddrSpace *newaddrspace;  //new address space variable initialized

    if (newexecutable == NULL)    //if file contains nothing.
      return;

    newaddrspace = new AddrSpace(newexecutable);        //create new address space
    currentThread->space = newaddrspace;                //overlay the address space of the current thread.

    delete newexecutable;                           //close file

    newaddrspace->InitRegisters();              //Initialize Registers.
    newaddrspace->RestoreState();               //Rload page table register               

    //currentThread = newthread;

    machine->Run();   //syscall complete since machine->Run() never returns.
    ASSERT(FALSE); //shouldn't reach here but just in case...
  }

  /*System Call Exit*/
  /*Exit Status are 0:terminated, 1:parent has not called join, 2:parent is waiting for child to finish*/
  else if((which == SyscallException) && (type == syscall_Exit))
  {
    int status = machine->ReadRegister(4);
    status = 0;     //set thread to be terminated

    NachOSThread *parentThread = currentThread->parent;

    if(parentThread->PID() == 0)
      interrupt->Halt();
    if(parentThread != NULL && parentThread->PID()!=0)   //If parent has not exited!
    {
      int parentchildstatus = parentThread->childstatusget(currentThread->PID());   //see if parent was waiting for child in it's array
      parentThread->childstatusset(currentThread->PID(), status);     //change entry in parent's log.
      parentThread->childcount--;
      if(parentchildstatus == 2)  //if parent was waiting...
      {
        IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
        scheduler->ReadyToRun(parentThread);
        (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
      } //Nothing to do since       
    }
    
    currentThread->FinishThread();      //kill this thread finally.
  }


  /*System Call Join*/
  else if((which == SyscallException) && (type == syscall_Join))
  {
    int childpid = machine->ReadRegister(4);

    if(currentThread->childindex(childpid) == -1)       //if child not found, advance program counters and return -1.
    {
      //Advance Program Counters
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

      machine->WriteRegister(2, -1);
    }

    //if we reach here, pid belongs to child of thread!
    int status = currentThread->childstatusget(childpid);     //get child status
    if(status == 1)                 //if parent was not waiting....
    {
      currentThread->childstatusset(childpid, 2);       //set child parent entry to child waiting....

      //Put Current Thread to sleep
      IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
      scheduler->ReadyToRun(currentThread);
      (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
    }

    //if parent was waiting..
    status = currentThread->childstatusget(childpid);     //We need to get status again as this thread may have been sleeping..
    while(status == 2)        //now wait for the child to complete...
    {

      IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
      scheduler->ReadyToRun(currentThread);
      (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts

      //after waking up, see if child has changed status...
      status = currentThread->childstatusget(childpid);
    }

    //we reach here only if the parent has finished waiting one way or another
    //Now return child status...
    machine->WriteRegister(2, status);

    //Advance Program Counters
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
  }

  /*System Call Fork*/
  else if((which == SyscallException) && (type == syscall_Fork)) 
  {
    //Advance Program Counters
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

    NachOSThread *childThread = new NachOSThread("child");       //child thread created...
    childThread->parent = currentThread;
    int childpid = childThread->PID();
    currentThread->AddChildEntry(childpid);

    //Get number of pages of parent and the staring physical page number. This defines parents address space.
    unsigned int ParentNumPages = currentThread->space->GetNumPages();          //Parent's number of pages
    unsigned int ParentStartIndexPA = currentThread->space->GetStartingPA();     //Parent's starting physical page number

    unsigned int ParentindexPA = ParentNumPages * PageSize;
    childThread->space = new AddrSpace(ParentNumPages, ParentindexPA);

    //Address Space Set up. Now set up child registers.
    machine->WriteRegister(2, 0);     //return 0 for child;
    childThread->SaveUserState();     //save curr register state for child

    machine->WriteRegister(2, childpid);    //parent gets child's pid as return value

    childThread->ThreadStackAllocate(&ForkFunction, 111);       //allocate child stack

    //schedule child thread
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts
    scheduler->ReadyToRun(childThread);
    (void) interrupt->SetLevel(oldLevel); // re-enable interrupts
  }

	/*Exception Not Found!*/
	else {
	 printf("Unexpected user mode exception %d %d\n", which, type);
	 ASSERT(FALSE);
	}
}
