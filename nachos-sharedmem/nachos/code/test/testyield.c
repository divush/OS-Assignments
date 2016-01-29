#include "syscall.h"

int
main()
{
    int x, i;

    x = system_Fork();
    for (i=0; i<5; i++) {
       system_PrintString("*** thread ");
       system_PrintInt(system_GetPID());
       system_PrintString(" looped ");
       system_PrintInt(i);
       system_PrintString(" times.\n");
       system_Yield();
    }
    if (x != 0) {
       system_PrintString("Before join.\n");
       system_Join(x);
       system_PrintString("After join.\n");
    }
    return 0;
}
