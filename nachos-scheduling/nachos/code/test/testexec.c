#include "syscall.h"

int
main()
{
    system_PrintString("Before calling Exec.\n");
    system_Exec("../test/vectorsum");
    system_PrintString("Returned from Exec.\n"); // Should never return
    return 0;
}
