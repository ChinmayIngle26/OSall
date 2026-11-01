/*
 * sched_common.h - shared code for Problems 7, 8 and 9
 *
 *  - Process creation/management with system calls: fork(), getpid(), getppid(),
 *    wait4()/waitpid(), _exit()
 *  - CPU scheduling algorithms: FCFS, SJF (non-preemptive), Round Robin
 *  - Analysis: waiting/turnaround/response time, CPU utilization, throughput,
 *    context switches and (simulated) memory utilization.
 *
 *  Usage of the programs that include this header:
 *      ./prog            -> run with built-in sample data
 *      ./prog -i         -> enter your own processes interactively
 */
#ifndef SCHED_COMMON_H
#define SCHED_COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define MAXP 32
#define MAXSEG 512
#define TOTAL_MEM_KB 1024          /* size of simulated main memory */

typedef struct {
    int pid;        /* process number (P1, P2, ...)          */
    int at;         /* arrival time                          */
    int bt;         /* burst time                            */
    int mem;        /* memory needed (KB)                    */
    int rem;        /* remaining burst time                  */
    int ct, tat, wt, rt;
    int started;
} Proc;

typedef struct {
    char   name[32];
    double avg_wt, avg_tat, avg_rt;
    double cpu_util, throughput;
    int    ctx_switches;
    double avg_mem_kb;
    int    peak_mem_kb;
    int    total_time;
} Summary;

static int g_quantum = 3;                       /* Round Robin time quantum */

/* ---------- Gantt chart storage ---------- */
static int g_seg_pid[MAXSEG], g_seg_start[MAXSEG], g_seg_end[MAXSEG], g_nseg;

static inline void gantt_reset(void) { g_nseg = 0; }
static inline void gantt_add(int pid, int start, int end)
{
    if (end <= start) return;
    if (g_nseg > 0 && g_seg_pid[g_nseg - 1] == pid && g_seg_end[g_nseg - 1] == start) {
        g_seg_end[g_nseg - 1] = end;            /* merge consecutive slices */
        return;
    }
    if (g_nseg < MAXSEG) {
        g_seg_pid[g_nseg] = pid; g_seg_start[g_nseg] = start; g_seg_end[g_nseg] = end;
        g_nseg++;
    }
}
static inline void gantt_print(void)
{
    printf("Gantt chart:\n|");
    for (int i = 0; i < g_nseg; i++) {
        if (g_seg_pid[i] == 0) printf(" IDLE |"); else printf(" P%d |", g_seg_pid[i]);
    }
    printf("\n%d", g_seg_start[0]);
    for (int i = 0; i < g_nseg; i++) {
        int w = (g_seg_pid[i] == 0) ? 6 : 4;
        printf("%*d", w + 1, g_seg_end[i]);
    }
    printf("\n");
}

/* ---------- input ---------- */
static inline void load_default(Proc *p, int *n)
{
    int at[] = {0, 1, 2, 3, 4};
    int bt[] = {8, 4, 9, 5, 2};
    int mem[] = {100, 200, 150, 120, 80};
    *n = 5;
    for (int i = 0; i < *n; i++) {
        memset(&p[i], 0, sizeof(Proc));
        p[i].pid = i + 1; p[i].at = at[i]; p[i].bt = bt[i]; p[i].mem = mem[i];
    }
}

static inline void read_processes(Proc *p, int *n)
{
    printf("Enter number of processes (1-%d): ", MAXP);
    if (scanf("%d", n) != 1 || *n < 1 || *n > MAXP) { fprintf(stderr, "Invalid count\n"); exit(1); }
    for (int i = 0; i < *n; i++) {
        memset(&p[i], 0, sizeof(Proc));
        p[i].pid = i + 1;
        printf("P%d  arrival time, burst time, memory(KB): ", i + 1);
        if (scanf("%d %d %d", &p[i].at, &p[i].bt, &p[i].mem) != 3 ||
            p[i].at < 0 || p[i].bt <= 0 || p[i].mem < 0) {
            fprintf(stderr, "Invalid input\n"); exit(1);
        }
    }
    printf("Enter time quantum for Round Robin: ");
    if (scanf("%d", &g_quantum) != 1 || g_quantum <= 0) { fprintf(stderr, "Invalid quantum\n"); exit(1); }
}

static inline void setup(int argc, char **argv, Proc *p, int *n)
{
    if (argc > 1 && strcmp(argv[1], "-i") == 0) read_processes(p, n);
    else load_default(p, n);
}

/* ---------- process creation / management using system calls ---------- */
/*
 * For every simulated process we fork() a real child. The child performs a small
 * CPU-bound loop proportional to its burst time and terminates with _exit(pid).
 * The parent uses wait4() to collect each child's exit status and resource usage
 * (CPU time and peak resident memory).
 */
static inline void process_demo(const Proc *p, int n)
{
    printf("\n================ Process creation using system calls ================\n");
    printf("Parent PID = %d\n", getpid());
    pid_t kids[MAXP];
    for (int i = 0; i < n; i++) {
        fflush(stdout);
        pid_t c = fork();
        if (c < 0) { perror("fork"); exit(1); }
        if (c == 0) {                                   /* ----- child ----- */
            volatile unsigned long x = 0;
            for (unsigned long k = 0; k < 2000000UL * (unsigned long)p[i].bt; k++) x += k;
            printf("  Child for P%d: pid=%d ppid=%d burst=%d\n", p[i].pid, getpid(), getppid(), p[i].bt);
            fflush(stdout);
            _exit(p[i].pid);
        }
        kids[i] = c;                                    /* ----- parent ---- */
    }
    for (int i = 0; i < n; i++) {
        int status; struct rusage ru;
        pid_t r = wait4(kids[i], &status, 0, &ru);
        if (r < 0) { perror("wait4"); continue; }
        printf("  Reaped pid=%d (P%d) exit=%d  user=%ld.%06lds  peak RSS=%ld KB\n",
               r, p[i].pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1,
               (long)ru.ru_utime.tv_sec, (long)ru.ru_utime.tv_usec, ru.ru_maxrss);
    }
}

/* ---------- reporting ---------- */
static int cmp_arrival(const void *a, const void *b)
{
    const Proc *x = a, *y = b;
    if (x->at != y->at) return x->at - y->at;
    return x->pid - y->pid;
}

static inline Summary finish(const char *name, Proc *s, int n)
{
    Summary r; memset(&r, 0, sizeof r);
    snprintf(r.name, sizeof r.name, "%s", name);
    { char *cut = strstr(r.name, " ("); if (cut) *cut = '\0'; }   /* short label for comparison table */

    printf("\n---------------- %s ----------------\n", name);
    gantt_print();
    printf("\n%-5s %-4s %-4s %-5s %-4s %-4s %-4s %-4s\n", "PID", "AT", "BT", "MEM", "CT", "TAT", "WT", "RT");
    double twt = 0, ttat = 0, trt = 0; int busy = 0, end = 0;
    for (int i = 0; i < n; i++) {
        s[i].tat = s[i].ct - s[i].at;
        s[i].wt  = s[i].tat - s[i].bt;
        twt += s[i].wt; ttat += s[i].tat; trt += s[i].rt; busy += s[i].bt;
        if (s[i].ct > end) end = s[i].ct;
        printf("P%-4d %-4d %-4d %-5d %-4d %-4d %-4d %-4d\n",
               s[i].pid, s[i].at, s[i].bt, s[i].mem, s[i].ct, s[i].tat, s[i].wt, s[i].rt);
    }
    int first = s[0].at;
    for (int i = 0; i < n; i++) if (s[i].at < first) first = s[i].at;
    int span = end - first; if (span <= 0) span = 1;

    /* memory in use at each time unit = processes that have arrived and not yet completed */
    long mem_area = 0; int peak = 0;
    for (int t = first; t < end; t++) {
        int m = 0;
        for (int i = 0; i < n; i++) if (s[i].at <= t && t < s[i].ct) m += s[i].mem;
        mem_area += m; if (m > peak) peak = m;
    }
    int cs = 0;
    for (int i = 1; i < g_nseg; i++) if (g_seg_pid[i] != 0 && g_seg_pid[i - 1] != 0) cs++;

    r.avg_wt = twt / n; r.avg_tat = ttat / n; r.avg_rt = trt / n;
    r.cpu_util = 100.0 * busy / span; r.throughput = (double)n / span;
    r.ctx_switches = cs; r.avg_mem_kb = (double)mem_area / span; r.peak_mem_kb = peak; r.total_time = span;

    printf("\nAverage waiting time    : %.2f\n", r.avg_wt);
    printf("Average turnaround time : %.2f\n", r.avg_tat);
    printf("Average response time   : %.2f\n", r.avg_rt);
    printf("CPU utilization         : %.2f %%\n", r.cpu_util);
    printf("Throughput              : %.3f processes/unit time\n", r.throughput);
    printf("Context switches        : %d\n", r.ctx_switches);
    printf("Memory: average in use  : %.1f KB (%.1f %% of %d KB), peak %d KB (%.1f %%)\n",
           r.avg_mem_kb, 100.0 * r.avg_mem_kb / TOTAL_MEM_KB, TOTAL_MEM_KB,
           r.peak_mem_kb, 100.0 * r.peak_mem_kb / TOTAL_MEM_KB);
    return r;
}

/* ---------- FCFS ---------- */
static inline Summary run_fcfs(const Proc *in, int n)
{
    Proc s[MAXP]; memcpy(s, in, n * sizeof(Proc));
    qsort(s, n, sizeof(Proc), cmp_arrival);
    gantt_reset();
    int time = s[0].at;
    if (time > 0) gantt_add(0, 0, time);
    for (int i = 0; i < n; i++) {
        if (time < s[i].at) { gantt_add(0, time, s[i].at); time = s[i].at; }
        s[i].rt = time - s[i].at;
        gantt_add(s[i].pid, time, time + s[i].bt);
        time += s[i].bt; s[i].ct = time;
    }
    return finish("FCFS (First Come First Served)", s, n);
}

/* ---------- SJF (non-preemptive) ---------- */
static inline Summary run_sjf(const Proc *in, int n)
{
    Proc s[MAXP]; memcpy(s, in, n * sizeof(Proc));
    int done[MAXP] = {0}, finished = 0, time = 0;
    gantt_reset();
    int minat = s[0].at; for (int i = 0; i < n; i++) if (s[i].at < minat) minat = s[i].at;
    if (minat > 0) { gantt_add(0, 0, minat); time = minat; }
    while (finished < n) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (!done[i] && s[i].at <= time &&
                (best < 0 || s[i].bt < s[best].bt || (s[i].bt == s[best].bt && s[i].at < s[best].at)))
                best = i;
        if (best < 0) {                              /* CPU idle until next arrival */
            int next = -1;
            for (int i = 0; i < n; i++) if (!done[i] && (next < 0 || s[i].at < s[next].at)) next = i;
            gantt_add(0, time, s[next].at); time = s[next].at; continue;
        }
        s[best].rt = time - s[best].at;
        gantt_add(s[best].pid, time, time + s[best].bt);
        time += s[best].bt; s[best].ct = time; done[best] = 1; finished++;
    }
    return finish("SJF (Shortest Job First, non-preemptive)", s, n);
}

/* ---------- Round Robin ---------- */
static inline Summary run_rr(const Proc *in, int n)
{
    Proc s[MAXP]; memcpy(s, in, n * sizeof(Proc));
    qsort(s, n, sizeof(Proc), cmp_arrival);
    for (int i = 0; i < n; i++) { s[i].rem = s[i].bt; s[i].started = 0; }
    int q[MAXP], head = 0, size = 0, next = 0, done = 0, time = 0;
    gantt_reset();
    if (s[0].at > 0) { gantt_add(0, 0, s[0].at); time = s[0].at; }
    while (done < n) {
        while (next < n && s[next].at <= time) { q[(head + size) % MAXP] = next++; size++; }
        if (size == 0) {                             /* idle until next arrival */
            gantt_add(0, time, s[next].at); time = s[next].at; continue;
        }
        int i = q[head]; head = (head + 1) % MAXP; size--;
        if (!s[i].started) { s[i].started = 1; s[i].rt = time - s[i].at; }
        int run = s[i].rem < g_quantum ? s[i].rem : g_quantum;
        gantt_add(s[i].pid, time, time + run);
        time += run; s[i].rem -= run;
        while (next < n && s[next].at <= time) { q[(head + size) % MAXP] = next++; size++; }
        if (s[i].rem > 0) { q[(head + size) % MAXP] = i; size++; }
        else { s[i].ct = time; done++; }
    }
    char name[48]; snprintf(name, sizeof name, "Round Robin (quantum = %d)", g_quantum);
    Summary r = finish(name, s, n);
    snprintf(r.name, sizeof r.name, "RR (q=%d)", g_quantum);
    return r;
}

/* ---------- comparison ---------- */
static inline void compare(const Summary *a, const Summary *b)
{
    printf("\n================ Comparison and analysis ================\n");
    printf("%-26s %-22s %-22s\n", "Metric", a->name, b->name);
    printf("%-26s %-22.2f %-22.2f\n", "Avg waiting time",    a->avg_wt,  b->avg_wt);
    printf("%-26s %-22.2f %-22.2f\n", "Avg turnaround time", a->avg_tat, b->avg_tat);
    printf("%-26s %-22.2f %-22.2f\n", "Avg response time",   a->avg_rt,  b->avg_rt);
    printf("%-26s %-22.2f %-22.2f\n", "CPU utilization (%)", a->cpu_util, b->cpu_util);
    printf("%-26s %-22.3f %-22.3f\n", "Throughput",          a->throughput, b->throughput);
    printf("%-26s %-22d %-22d\n",     "Context switches",    a->ctx_switches, b->ctx_switches);
    printf("%-26s %-22.1f %-22.1f\n", "Avg memory in use (KB)", a->avg_mem_kb, b->avg_mem_kb);
    printf("%-26s %-22d %-22d\n",     "Peak memory (KB)",    a->peak_mem_kb, b->peak_mem_kb);
    printf("\nConclusion:\n");
    printf(" * Lower average waiting time  : %s\n", a->avg_wt  <= b->avg_wt  ? a->name : b->name);
    printf(" * Lower average response time : %s\n", a->avg_rt  <= b->avg_rt  ? a->name : b->name);
    printf(" * Fewer context switches      : %s\n", a->ctx_switches <= b->ctx_switches ? a->name : b->name);
    printf(" * Lower average memory in use : %s (processes finish sooner so their memory is released sooner)\n",
           a->avg_mem_kb <= b->avg_mem_kb ? a->name : b->name);
    printf(" * FCFS is simple but suffers from the convoy effect; SJF minimises average waiting time\n"
           "   but can starve long jobs; Round Robin gives the best response time and fairness at\n"
           "   the cost of more context switches (overhead) and usually higher average waiting time.\n");
}

#endif
