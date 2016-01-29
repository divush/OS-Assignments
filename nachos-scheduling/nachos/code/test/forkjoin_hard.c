#include "syscall.h"

int
main()
{
    int x, y=6;
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
       y++;
       system_PrintString("Child y=");
       system_PrintInt(y);
       system_PrintChar('\n');
       x = system_Fork();
       system_Exec("../test/printtest");
       if (x == 0) {
          system_PrintString("Child PID: ");
          system_PrintInt(system_GetPID());
          system_PrintChar('\n');
          y++;
          system_PrintString("Child2 y=");
          system_PrintInt(y);
          system_PrintChar('\n');
          system_Exit(20);
       }
       else {
          system_PrintString("Parent after fork waiting for child: ");
          system_PrintInt(x);
          system_PrintChar('\n');
          system_PrintString("Parent2 join value: ");
          system_PrintInt(system_Join(x));
          system_PrintChar('\n');
          system_PrintString("Parent2 y=");
          system_PrintInt(y);
          system_PrintChar('\n');
          system_Exit(10);
       }
    }
    else {
       system_PrintString("Parent after fork waiting for child: ");
       system_PrintInt(x);
       system_PrintChar('\n');
       system_PrintString("Parent2 join value: ");
       system_PrintInt(system_Join(x));
       system_PrintChar('\n');
       system_PrintString("Parent y=");
       system_PrintInt(y);
       system_PrintChar('\n');
       system_Exit(1);
    }
    return 0;
}
