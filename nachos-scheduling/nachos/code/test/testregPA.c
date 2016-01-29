#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;

    system_PrintString("Starting physical address of array: ");
    system_PrintInt(system_GetPA((unsigned)array));
    system_PrintChar('\n');
    system_PrintString("Physical address of array[50]: ");
    system_PrintInt(system_GetPA(&array[50]));
    system_PrintChar('\n');
    system_PrintString("Current physical address of stack top: ");
    system_PrintInt(system_GetPA(system_GetReg(29)));
    system_PrintChar('\n');
    for (i=0; i<SIZE; i++) array[i] = i;
    system_PrintString("We are currently at PC: ");
    system_PrintIntHex(system_GetReg(34));
    system_PrintChar('\n');
    for (i=0; i<SIZE; i++) sum += array[i];
    system_PrintString("Total sum: ");
    system_PrintInt(sum);
    system_PrintChar('\n');
    system_Exit(0);
    return 0;
}
