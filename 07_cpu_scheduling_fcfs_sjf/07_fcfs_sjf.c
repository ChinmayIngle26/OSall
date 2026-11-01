/* Problem 7: Process creation with system calls + CPU scheduling (FCFS and SJF) */
#include "../common/sched_common.h"

int main(int argc, char **argv)
{
    Proc p[MAXP]; int n;
    setup(argc, argv, p, &n);
    process_demo(p, n);
    Summary a = run_fcfs(p, n);
    Summary b = run_sjf(p, n);
    compare(&a, &b);
    return 0;
}
