/*
 * Problem 13: Multithreaded application using POSIX threads
 *
 * Part A: concurrent tasks - several different tasks run in separate threads.
 * Part B: data race demo   - shared counter with and without a mutex.
 * Part C: performance      - count primes below LIMIT using 1,2,4,8,16 threads and
 *                            measure execution time, CPU utilization and memory.
 *
 * Compile: gcc -O2 -Wall -pthread 13_pthreads.c -o 13_pthreads -lm
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/resource.h>

#define LIMIT 3000000L

/* ---------------- Part A: different concurrent tasks ---------------- */
static void *task_sum(void *arg)
{
    long n = *(long *)arg, s = 0;
    for (long i = 1; i <= n; i++) s += i;
    printf("  [Thread: sum 1..%ld]         result = %ld\n", n, s);
    return NULL;
}
static void *task_factorial(void *arg)
{
    int n = *(int *)arg; unsigned long long f = 1;
    for (int i = 2; i <= n; i++) f *= i;
    printf("  [Thread: factorial of %d]    result = %llu\n", n, f);
    return NULL;
}
static void *task_fibonacci(void *arg)
{
    int n = *(int *)arg; unsigned long long a = 0, b = 1;
    for (int i = 0; i < n; i++) { unsigned long long t = a + b; a = b; b = t; }
    printf("  [Thread: Fibonacci(%d)]      result = %llu\n", n, a);
    return NULL;
}
static void *task_hello(void *arg)
{
    printf("  [Thread: hello]              %s (thread id %lu)\n", (char *)arg, (unsigned long)pthread_self());
    return NULL;
}

/* ---------------- Part B: race condition ---------------- */
static long counter_unsafe = 0, counter_safe = 0;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
#define INCS 500000

static void *inc_unsafe(void *a) { (void)a; for (int i = 0; i < INCS; i++) { long t = counter_unsafe; __asm__ __volatile__("" ::: "memory"); counter_unsafe = t + 1; } return NULL; }
static void *inc_safe(void *a)
{
    (void)a;
    for (int i = 0; i < INCS; i++) { pthread_mutex_lock(&counter_lock); counter_safe++; pthread_mutex_unlock(&counter_lock); }
    return NULL;
}

/* ---------------- Part C: prime counting ---------------- */
typedef struct { int id, nthreads; long count; } Work;

static int is_prime(long n)
{
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (long d = 3; d * d <= n; d += 2) if (n % d == 0) return 0;
    return 1;
}
static void *count_primes(void *arg)
{
    Work *w = arg; long c = 0;
    for (long n = w->id; n < LIMIT; n += w->nthreads) c += is_prime(n);   /* interleaved split = balanced load */
    w->count = c;
    return NULL;
}

static double wall(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static double cpu_time(void)
{
    struct rusage r; getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_utime.tv_usec / 1e6 + r.ru_stime.tv_sec + r.ru_stime.tv_usec / 1e6;
}

int main(void)
{
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    printf("=== Part A: concurrent tasks with POSIX threads ===\n");
    pthread_t t[4];
    long n1 = 1000000; int n2 = 15, n3 = 50; char msg[] = "Hello from a POSIX thread";
    pthread_create(&t[0], NULL, task_sum, &n1);
    pthread_create(&t[1], NULL, task_factorial, &n2);
    pthread_create(&t[2], NULL, task_fibonacci, &n3);
    pthread_create(&t[3], NULL, task_hello, msg);
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);

    printf("\n=== Part B: race condition vs mutex (2 threads x %d increments) ===\n", INCS);
    pthread_t a, b;
    pthread_create(&a, NULL, inc_unsafe, NULL); pthread_create(&b, NULL, inc_unsafe, NULL);
    pthread_join(a, NULL); pthread_join(b, NULL);
    printf("  Without mutex: counter = %ld (expected %d)%s\n", counter_unsafe, 2 * INCS,
           counter_unsafe == 2 * INCS ? "  <- no race observed this run (e.g. single CPU)" : "  <- lost updates (race condition)");
    pthread_create(&a, NULL, inc_safe, NULL); pthread_create(&b, NULL, inc_safe, NULL);
    pthread_join(a, NULL); pthread_join(b, NULL);
    printf("  With mutex   : counter = %ld (expected %d)\n", counter_safe, 2 * INCS);

    printf("\n=== Part C: counting primes below %ld, CPUs online = %ld ===\n", LIMIT, ncpu);
    printf("%-8s %-10s %-10s %-10s %-12s %-10s %-10s %-8s\n",
           "Threads", "Primes", "Wall(s)", "CPU(s)", "CPU util %", "Speedup", "MaxRSS KB", "Correct");
    int counts[] = {1, 2, 4, 8, 16};
    double base = 0; long ref = -1;
    for (int k = 0; k < 5; k++) {
        int nt = counts[k];
        pthread_t *th = malloc(nt * sizeof *th);
        Work *w = malloc(nt * sizeof *w);
        double c0 = cpu_time(), t0 = wall();
        for (int i = 0; i < nt; i++) { w[i].id = i; w[i].nthreads = nt; w[i].count = 0; pthread_create(&th[i], NULL, count_primes, &w[i]); }
        long total = 0;
        for (int i = 0; i < nt; i++) { pthread_join(th[i], NULL); total += w[i].count; }
        double el = wall() - t0, cpu = cpu_time() - c0;
        struct rusage ru; getrusage(RUSAGE_SELF, &ru);
        if (k == 0) { base = el; ref = total; }
        printf("%-8d %-10ld %-10.3f %-10.3f %-12.1f %-10.2f %-10ld %-8s\n", nt, total, el, cpu,
               100.0 * cpu / (el * ncpu), base / el, ru.ru_maxrss, total == ref ? "yes" : "NO");
        free(th); free(w);
    }
    printf("\nAnalysis:\n"
           " * Execution time falls as threads are added only up to the number of CPU cores (%ld here);\n"
           "   beyond that extra threads add scheduling/creation overhead and give no further speed-up.\n"
           " * CPU utilization (CPU time / (wall time x cores)) rises towards 100%% when enough threads are\n"
           "   available to keep all cores busy.\n"
           " * Memory grows slowly with the thread count because every thread needs its own stack\n"
           "   (8 MB virtual by default, only touched pages are resident).\n", ncpu);
    return 0;
}
