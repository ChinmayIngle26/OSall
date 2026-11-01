/*
 * Problem 14: Producer-Consumer (bounded buffer) using semaphores and a mutex
 *
 *   empty : counts free slots      (initial value = BUFFER size)
 *   full  : counts occupied slots  (initial value = 0)
 *   mutex : protects the buffer indices (mutual exclusion)
 *
 * Part 1 prints a short trace. Part 2 evaluates performance for several buffer sizes and
 * numbers of producers/consumers, and verifies that no item is lost or duplicated.
 *
 * Compile: gcc -O2 -Wall -pthread 14_producer_consumer.c -o 14_producer_consumer
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int *buf; int size, in, out;
    sem_t empty, full;
    pthread_mutex_t mutex;
    int verbose;
    long per_producer, per_consumer;
    int nprod, ncons;
    long long produced_sum, consumed_sum;      /* protected by sum_lock */
    pthread_mutex_t sum_lock;
} Shared;

typedef struct { Shared *s; int id; } Arg;

static void *producer(void *p)
{
    Arg *a = p; Shared *s = a->s;
    long long local = 0;
    for (long k = 0; k < s->per_producer; k++) {
        int item = (int)(a->id * s->per_producer + k);
        sem_wait(&s->empty);                       /* wait for a free slot   */
        pthread_mutex_lock(&s->mutex);             /* enter critical section */
        s->buf[s->in] = item;
        if (s->verbose) printf("  Producer %d put item %-3d at slot %d\n", a->id, item, s->in);
        s->in = (s->in + 1) % s->size;
        pthread_mutex_unlock(&s->mutex);
        sem_post(&s->full);                        /* signal an item is ready */
        local += item;
    }
    pthread_mutex_lock(&s->sum_lock); s->produced_sum += local; pthread_mutex_unlock(&s->sum_lock);
    return NULL;
}

static void *consumer(void *p)
{
    Arg *a = p; Shared *s = a->s;
    long long local = 0;
    for (long k = 0; k < s->per_consumer; k++) {
        sem_wait(&s->full);                        /* wait for an item       */
        pthread_mutex_lock(&s->mutex);
        int item = s->buf[s->out];
        if (s->verbose) printf("  Consumer %d got item %-3d from slot %d\n", a->id, item, s->out);
        s->out = (s->out + 1) % s->size;
        pthread_mutex_unlock(&s->mutex);
        sem_post(&s->empty);                       /* signal a slot is free  */
        local += item;
    }
    pthread_mutex_lock(&s->sum_lock); s->consumed_sum += local; pthread_mutex_unlock(&s->sum_lock);
    return NULL;
}

static double wall(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

/* returns elapsed seconds; total items must be divisible by nprod and ncons */
static double run(int bufsize, int nprod, int ncons, long total, int verbose, int *ok)
{
    Shared s = {0};
    s.buf = malloc(bufsize * sizeof(int)); s.size = bufsize; s.verbose = verbose;
    s.nprod = nprod; s.ncons = ncons;
    s.per_producer = total / nprod; s.per_consumer = total / ncons;
    sem_init(&s.empty, 0, bufsize); sem_init(&s.full, 0, 0);
    pthread_mutex_init(&s.mutex, NULL); pthread_mutex_init(&s.sum_lock, NULL);

    pthread_t *pt = malloc(nprod * sizeof *pt), *ct = malloc(ncons * sizeof *ct);
    Arg *pa = malloc(nprod * sizeof *pa), *ca = malloc(ncons * sizeof *ca);
    double t0 = wall();
    for (int i = 0; i < nprod; i++) { pa[i].s = &s; pa[i].id = i; pthread_create(&pt[i], NULL, producer, &pa[i]); }
    for (int i = 0; i < ncons; i++) { ca[i].s = &s; ca[i].id = i; pthread_create(&ct[i], NULL, consumer, &ca[i]); }
    for (int i = 0; i < nprod; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < ncons; i++) pthread_join(ct[i], NULL);
    double el = wall() - t0;

    *ok = (s.produced_sum == s.consumed_sum) && (s.in == s.out);
    sem_destroy(&s.empty); sem_destroy(&s.full);
    pthread_mutex_destroy(&s.mutex); pthread_mutex_destroy(&s.sum_lock);
    free(s.buf); free(pt); free(ct); free(pa); free(ca);
    return el;
}

int main(void)
{
    int ok;
    printf("=== Part 1: trace (buffer = 3, 1 producer, 1 consumer, 8 items) ===\n");
    run(3, 1, 1, 8, 1, &ok);
    printf("Result: %s\n\n", ok ? "all items produced were consumed (correct)" : "ERROR");

    printf("=== Part 2: performance evaluation (120000 items) ===\n");
    printf("%-8s %-10s %-10s %-10s %-14s %-8s\n", "Buffer", "Producers", "Consumers", "Time(s)", "Items/second", "Correct");
    int cfg[][3] = {{1,1,1},{5,1,1},{50,1,1},{5,3,3},{50,3,3},{500,4,4},{50,4,2},{50,2,4}};
    long total = 120000;
    for (unsigned i = 0; i < sizeof cfg / sizeof cfg[0]; i++) {
        double t = run(cfg[i][0], cfg[i][1], cfg[i][2], total, 0, &ok);
        printf("%-8d %-10d %-10d %-10.4f %-14.0f %-8s\n", cfg[i][0], cfg[i][1], cfg[i][2], t, total / t, ok ? "yes" : "NO");
    }
    printf("\nAnalysis:\n"
           " * 'empty' and 'full' semaphores make producers sleep when the buffer is full and consumers sleep\n"
           "   when it is empty - no busy waiting, no lost or duplicated items (sums match).\n"
           " * The mutex serialises access to in/out indices, so the buffer is never corrupted.\n"
           " * A tiny buffer (size 1) forces strict alternation and many context switches -> lowest throughput.\n"
           "   Larger buffers decouple producers and consumers, so throughput improves, then levels off.\n"
           " * More threads increase contention on the single mutex, so throughput does not grow linearly.\n");
    return 0;
}
