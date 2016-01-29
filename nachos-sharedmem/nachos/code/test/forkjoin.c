#include "syscall.h"

int
main()
{
    int x;
    int sleep_start, sleep_end;

    system_PrintString("Parent PID: ");
    system_PrintInt(system_GetPID());
    system_PrintChar('\n');
    x = system_Fork();
    if (x == 0) {
       system_PrintString("Child PID: ");
       system_PrintInt(system_GetPID());
       system_PrintChar('\n');
       system_PrintString("Child's parent PID: ");
       system_PrintInt(system_GetPPID());
       system_PrintChar('\n');
       sleep_start = system_GetTime();
       system_Sleep(100);
       sleep_end = system_GetTime();
       system_PrintString("Child called sleep at time: ");
       system_PrintInt(sleep_start);
       system_PrintChar('\n');
       system_PrintString("Child returned from sleep at time: ");
       system_PrintInt(sleep_end);
       system_PrintChar('\n');
       system_PrintString("Child executed ");
       system_PrintInt(system_GetNumInstr());
       system_PrintString(" instructions.\n");
    }
    else {
       system_PrintString("Parent after fork waiting for child: ");
       system_PrintInt(x);
       system_PrintChar('\n');
       system_Join(x);
       system_PrintString("Parent executed ");
       system_PrintInt(system_GetNumInstr());
       system_PrintString(" instructions.\n");
    }
    return 0;
}
