/*
 * Problem 15: Readers-Writers problem using semaphores and mutex locks
 *
 * Version A (naive)    : one mutex guards the data -> readers also exclude each other.
 * Version B (readers-preference, classic solution):
 *        mutex  protects read_count
 *        wrt    semaphore giving writers exclusive access
 *        first reader locks wrt, last reader releases it -> many readers can read together.
 *
 * Each read/write "work" is simulated with usleep() (like disk or network access).
 * The program shows the maximum number of readers that were inside at once and compares times.
 *
 * Compile: gcc -O2 -Wall -pthread 15_readers_writers.c -o 15_readers_writers
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define WORK_US 200

static int shared_data = 0;

/* Version B objects */
static pthread_mutex_t rc_mutex = PTHREAD_MUTEX_INITIALIZER;   /* protects read_count */
static sem_t wrt;                                              /* writer exclusion    */
static int read_count = 0, active_readers = 0, max_readers = 0;
/* Version A object */
static pthread_mutex_t big_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stat_lock = PTHREAD_MUTEX_INITIALIZER;

static int use_rw;                 /* 1 = version B, 0 = version A */
static int ops;                    /* operations per thread        */
static long total_reads = 0, total_writes = 0;
static int verbose = 0;

static void note_enter(void)
{
    pthread_mutex_lock(&stat_lock);
    active_readers++; if (active_readers > max_readers) max_readers = active_readers;
    pthread_mutex_unlock(&stat_lock);
}
static void note_leave(void) { pthread_mutex_lock(&stat_lock); active_readers--; pthread_mutex_unlock(&stat_lock); }

static void *reader(void *arg)
{
    int id = (int)(long)arg;
    for (int i = 0; i < ops; i++) {
        if (use_rw) {
            pthread_mutex_lock(&rc_mutex);
            if (++read_count == 1) sem_wait(&wrt);     /* first reader blocks writers */
            pthread_mutex_unlock(&rc_mutex);
        } else pthread_mutex_lock(&big_lock);

        note_enter();
        int v = shared_data;                           /* ---- critical section (read) ---- */
        usleep(WORK_US);
        if (verbose) printf("  Reader %d read value %d\n", id, v);
        note_leave();

        if (use_rw) {
            pthread_mutex_lock(&rc_mutex);
            if (--read_count == 0) sem_post(&wrt);     /* last reader lets writers in */
            pthread_mutex_unlock(&rc_mutex);
        } else pthread_mutex_unlock(&big_lock);
        pthread_mutex_lock(&stat_lock); total_reads++; pthread_mutex_unlock(&stat_lock);
    }
    return NULL;
}

static void *writer(void *arg)
{
    int id = (int)(long)arg;
    for (int i = 0; i < ops; i++) {
        if (use_rw) sem_wait(&wrt); else pthread_mutex_lock(&big_lock);
        int v = shared_data + 1;                       /* ---- critical section (write) --- */
        usleep(WORK_US);
        shared_data = v;
        if (verbose) printf("  Writer %d wrote value %d\n", id, v);
        if (use_rw) sem_post(&wrt); else pthread_mutex_unlock(&big_lock);
        pthread_mutex_lock(&stat_lock); total_writes++; pthread_mutex_unlock(&stat_lock);
    }
    return NULL;
}

static double wall(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

static double run(int rw, int nr, int nw, int nops, int v, int *correct)
{
    use_rw = rw; ops = nops; verbose = v;
    shared_data = 0; read_count = 0; active_readers = 0; max_readers = 0; total_reads = total_writes = 0;
    pthread_t *r = malloc(nr * sizeof *r), *w = malloc(nw * sizeof *w);
    double t0 = wall();
    for (long i = 0; i < nw; i++) pthread_create(&w[i], NULL, writer, (void *)i);
    for (long i = 0; i < nr; i++) pthread_create(&r[i], NULL, reader, (void *)i);
    for (int i = 0; i < nw; i++) pthread_join(w[i], NULL);
    for (int i = 0; i < nr; i++) pthread_join(r[i], NULL);
    double el = wall() - t0;
    *correct = (shared_data == nw * nops) && total_reads == (long)nr * nops && total_writes == (long)nw * nops;
    free(r); free(w);
    return el;
}

int main(void)
{
    int ok;
    sem_init(&wrt, 0, 1);

    printf("=== Part 1: short trace (readers-preference, 2 readers, 1 writer, 3 ops each) ===\n");
    run(1, 2, 1, 3, 1, &ok);
    printf("Final value = %d (expected 3) -> %s\n\n", shared_data, ok ? "correct" : "ERROR");

    printf("=== Part 2: performance (5 readers, 2 writers, 150 operations each, %d us work/op) ===\n", WORK_US);
    printf("%-34s %-10s %-14s %-10s %-8s\n", "Version", "Time(s)", "Max readers", "Ops/sec", "Correct");
    int nr = 5, nw = 2, nops = 150;
    double ta = run(0, nr, nw, nops, 0, &ok);
    printf("%-34s %-10.3f %-14d %-10.0f %-8s\n", "A: single mutex (no read sharing)", ta, max_readers,
           (nr + nw) * (double)nops / ta, ok ? "yes" : "NO");
    double tb = run(1, nr, nw, nops, 0, &ok);
    printf("%-34s %-10.3f %-14d %-10.0f %-8s\n", "B: readers-preference semaphores", tb, max_readers,
           (nr + nw) * (double)nops / tb, ok ? "yes" : "NO");
    printf("\nSpeed-up of B over A: %.2fx\n", ta / tb);

    printf("\nAnalysis:\n"
           " * Writers always get exclusive access, so the final value equals the number of writes (no lost updates).\n"
           " * In version B several readers are inside the critical section at once (see 'Max readers'),\n"
           "   which improves throughput for read-heavy workloads compared with a single mutex.\n"
           " * Drawback of readers-preference: a continuous stream of readers can starve writers.\n"
           "   Writers-preference or fair (turnstile) solutions avoid starvation at some cost in concurrency.\n");
    sem_destroy(&wrt);
    return 0;
}
