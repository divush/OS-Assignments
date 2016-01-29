/* Start.s 
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we don't want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main". 
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0		
	jal	system_Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is 
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl system_Halt
	.ent	system_Halt
system_Halt:
	addiu $2,$0,syscall_Halt
	syscall
	j	$31
	.end system_Halt

	.globl system_Exit
	.ent	system_Exit
system_Exit:
	addiu $2,$0,syscall_Exit
	syscall
	j	$31
	.end system_Exit

	.globl system_Exec
	.ent	system_Exec
system_Exec:
	addiu $2,$0,syscall_Exec
	syscall
	j	$31
	.end system_Exec

	.globl system_Join
	.ent	system_Join
system_Join:
	addiu $2,$0,syscall_Join
	syscall
	j	$31
	.end system_Join

	.globl system_Create
	.ent	system_Create
system_Create:
	addiu $2,$0,syscall_Create
	syscall
	j	$31
	.end system_Create

	.globl system_Open
	.ent	system_Open
system_Open:
	addiu $2,$0,syscall_Open
	syscall
	j	$31
	.end system_Open

	.globl system_Read
	.ent	system_Read
system_Read:
	addiu $2,$0,syscall_Read
	syscall
	j	$31
	.end system_Read

	.globl system_Write
	.ent	system_Write
system_Write:
	addiu $2,$0,syscall_Write
	syscall
	j	$31
	.end system_Write

	.globl system_Close
	.ent	system_Close
system_Close:
	addiu $2,$0,syscall_Close
	syscall
	j	$31
	.end system_Close

	.globl system_Fork
	.ent	system_Fork
system_Fork:
	addiu $2,$0,syscall_Fork
	syscall
	j	$31
	.end system_Fork

	.globl system_Yield
	.ent	system_Yield
system_Yield:
	addiu $2,$0,syscall_Yield
	syscall
	j	$31
	.end system_Yield

	.globl system_PrintInt
	.ent    system_PrintInt
system_PrintInt:
        addiu $2,$0,syscall_PrintInt
        syscall
        j       $31
	.end system_PrintInt

	.globl system_PrintChar
	.ent    system_PrintChar
system_PrintChar:
        addiu $2,$0,syscall_PrintChar
        syscall
        j       $31
	.end system_PrintChar

	.globl system_PrintString
	.ent    system_PrintString
system_PrintString:
        addiu $2,$0,syscall_PrintString
        syscall
        j       $31
	.end system_PrintString

	.globl system_GetReg
	.ent    system_GetReg
system_GetReg:
	addiu $2,$0,syscall_GetReg
	syscall
	j       $31
	.end system_GetReg

	.globl system_GetPA
	.ent    system_GetPA
system_GetPA:
	addiu $2,$0,syscall_GetPA
	syscall
	j       $31
	.end system_GetPA

	.globl system_GetPID
	.ent    system_GetPID
system_GetPID:
	addiu $2,$0,syscall_GetPID
	syscall
	j       $31
	.end system_GetPID

	.globl system_GetPPID
	.ent    system_GetPPID
system_GetPPID:
	addiu $2,$0,syscall_GetPPID
	syscall
	j       $31
	.end system_GetPPID

	.globl system_Sleep
	.ent    system_Sleep
system_Sleep:
	addiu $2,$0,syscall_Sleep
	syscall
	j       $31
	.end system_Sleep

	.globl system_GetTime
	.ent    system_GetTime
system_GetTime:
	addiu $2,$0,syscall_Time
	syscall
	j       $31
	.end system_GetTime

	.globl system_GetNumInstr
	.ent    system_GetNumInstr
system_GetNumInstr:
	addiu $2,$0,syscall_NumInstr
	syscall
	j	$31
	.end system_GetNumInstr

	.globl system_PrintIntHex
	.ent    system_PrintIntHex
system_PrintIntHex:
	addiu $2,$0,syscall_PrintIntHex
	syscall
	j	$31
	.end system_PrintIntHex

/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main

