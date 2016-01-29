#include "syscall.h"
#define OUTER_BOUND 4
#define SIZE 100

int
main()
{
    int array[SIZE], i, k, sum, pid=system_GetPID()-1;
    unsigned start_time, end_time;
    
    start_time = system_GetTime();
    for (k=0; k<OUTER_BOUND; k++) {
       for (i=0; i<SIZE; i++) sum += array[i];
       system_PrintInt(pid);
       system_PrintInt(pid);
    }
    end_time = system_GetTime();
    system_PrintChar('\n');
    system_PrintString("Total sum: ");
    system_PrintInt(sum);
    system_PrintChar('\n');
    system_PrintString("Start time: ");
    system_PrintInt(start_time);
    system_PrintString(", End time: ");
    system_PrintInt(end_time);
    system_PrintString(", Total time: ");
    system_PrintInt(end_time-start_time);
    system_PrintChar('\n');
    return 0;
}
