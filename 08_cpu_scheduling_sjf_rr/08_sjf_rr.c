/* Problem 8: Process creation with system calls + CPU scheduling (SJF and Round Robin) */
#include "../common/sched_common.h"

int main(int argc, char **argv)
{
    Proc p[MAXP]; int n;
    setup(argc, argv, p, &n);
    process_demo(p, n);
    Summary a = run_sjf(p, n);
    Summary b = run_rr(p, n);
    compare(&a, &b);
    return 0;
}
