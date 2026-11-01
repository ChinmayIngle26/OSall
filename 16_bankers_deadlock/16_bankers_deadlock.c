/*
 * Problem 16: Banker's Algorithm (deadlock avoidance) and Deadlock Detection
 *
 * Part 1: Safety algorithm        - is the system in a safe state? print a safe sequence.
 * Part 2: Resource-request algorithm - grant or deny requests so the system stays safe.
 * Part 3: Deadlock detection      - two scenarios (no deadlock / deadlock).
 * Part 4: Analysis of resource allocation and utilization for each scenario.
 *
 * Compile: gcc -O2 -Wall 16_bankers_deadlock.c -o 16_bankers_deadlock
 * Run    : ./16_bankers_deadlock            (built-in data)
 *          ./16_bankers_deadlock -i         (enter your own data for the Banker's algorithm)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXP 10
#define MAXR 10

typedef struct {
    int n, m;
    int alloc[MAXP][MAXR], max[MAXP][MAXR], need[MAXP][MAXR], avail[MAXR];
} System;

static void compute_need(System *s)
{
    for (int i = 0; i < s->n; i++)
        for (int j = 0; j < s->m; j++) s->need[i][j] = s->max[i][j] - s->alloc[i][j];
}

static void print_state(const System *s)
{
    printf("Process | Allocation | Max       | Need\n");
    for (int i = 0; i < s->n; i++) {
        printf("  P%-4d |", i);
        printf(" ");  for (int j = 0; j < s->m; j++) printf("%-3d", s->alloc[i][j]);
        printf(" |  ");   for (int j = 0; j < s->m; j++) printf("%-3d", s->max[i][j]);
        printf(" |  ");   for (int j = 0; j < s->m; j++) printf("%-3d", s->need[i][j]);
        printf("\n");
    }
    printf("Available: "); for (int j = 0; j < s->m; j++) printf("%d ", s->avail[j]);
    printf("\n");
}

/* Safety algorithm. Returns 1 if safe and fills seq[] with a safe sequence. */
static int is_safe(const System *s, int *seq)
{
    int work[MAXR], finish[MAXP] = {0}, count = 0;
    memcpy(work, s->avail, sizeof work);
    while (count < s->n) {
        int found = 0;
        for (int i = 0; i < s->n; i++) {
            if (finish[i]) continue;
            int ok = 1;
            for (int j = 0; j < s->m; j++) if (s->need[i][j] > work[j]) { ok = 0; break; }
            if (ok) {                                   /* Pi can finish: it returns its resources */
                for (int j = 0; j < s->m; j++) work[j] += s->alloc[i][j];
                finish[i] = 1; seq[count++] = i; found = 1;
            }
        }
        if (!found) return 0;                           /* nobody can proceed -> unsafe */
    }
    return 1;
}

static void print_seq(const int *seq, int n)
{
    for (int i = 0; i < n; i++) printf("P%d%s", seq[i], i < n - 1 ? " -> " : "\n");
}

/* Resource-request algorithm for process p. Returns 1 if the request was granted. */
static int request_resources(System *s, int p, const int *req)
{
    printf("\nRequest by P%d: ", p); for (int j = 0; j < s->m; j++) printf("%d ", req[j]);
    printf("\n");
    for (int j = 0; j < s->m; j++)
        if (req[j] > s->need[p][j]) { printf("  -> DENIED: process exceeded its maximum claim.\n"); return 0; }
    for (int j = 0; j < s->m; j++)
        if (req[j] > s->avail[j]) { printf("  -> WAIT: resources not available right now.\n"); return 0; }
    System t = *s;                                      /* pretend to allocate */
    for (int j = 0; j < s->m; j++) { t.avail[j] -= req[j]; t.alloc[p][j] += req[j]; t.need[p][j] -= req[j]; }
    int seq[MAXP];
    if (is_safe(&t, seq)) {
        *s = t;
        printf("  -> GRANTED. System stays safe, safe sequence: "); print_seq(seq, s->n);
        return 1;
    }
    printf("  -> DENIED: granting it would lead to an UNSAFE state.\n");
    return 0;
}

/* ---------------- deadlock detection (single instance or multiple instance types) ---------------- */
typedef struct {
    int n, m;
    int alloc[MAXP][MAXR], request[MAXP][MAXR], avail[MAXR], total[MAXR];
} DSystem;

/* returns number of deadlocked processes; deadlocked[i]=1 for each of them */
static int detect_deadlock(const DSystem *d, int *deadlocked, int *order, int *norder)
{
    int work[MAXR], finish[MAXP];
    memcpy(work, d->avail, sizeof work);
    for (int i = 0; i < d->n; i++) {
        int holds = 0; for (int j = 0; j < d->m; j++) holds |= d->alloc[i][j];
        finish[i] = !holds;                             /* a process holding nothing cannot be deadlocked */
    }
    *norder = 0;
    int progress = 1;
    while (progress) {
        progress = 0;
        for (int i = 0; i < d->n; i++) {
            if (finish[i]) continue;
            int ok = 1;
            for (int j = 0; j < d->m; j++) if (d->request[i][j] > work[j]) { ok = 0; break; }
            if (ok) {
                for (int j = 0; j < d->m; j++) work[j] += d->alloc[i][j];
                finish[i] = 1; order[(*norder)++] = i; progress = 1;
            }
        }
    }
    int cnt = 0;
    for (int i = 0; i < d->n; i++) { deadlocked[i] = !finish[i]; cnt += deadlocked[i]; }
    return cnt;
}

static void detection_scenario(const char *title, DSystem *d)
{
    printf("\n--- %s ---\n", title);
    printf("Process | Allocation | Request\n");
    for (int i = 0; i < d->n; i++) {
        printf("  P%-4d |  ", i);
        for (int j = 0; j < d->m; j++) printf("%-3d", d->alloc[i][j]);
        printf("|  ");
        for (int j = 0; j < d->m; j++) printf("%-3d", d->request[i][j]);
        printf("\n");
    }
    printf("Available: "); for (int j = 0; j < d->m; j++) printf("%d ", d->avail[j]); printf("\n");

    int dead[MAXP], order[MAXP], no;
    clock_t c0 = clock();
    int cnt = detect_deadlock(d, dead, order, &no);
    double us = 1e6 * (clock() - c0) / CLOCKS_PER_SEC;
    if (cnt == 0) { printf("Result: NO deadlock. Possible completion order: "); print_seq(order, no); }
    else {
        printf("Result: DEADLOCK detected! Deadlocked processes:");
        for (int i = 0; i < d->n; i++) if (dead[i]) printf(" P%d", i);
        printf("\n");
    }
    /* analysis */
    printf("Analysis: ");
    for (int j = 0; j < d->m; j++) {
        int used = 0; for (int i = 0; i < d->n; i++) used += d->alloc[i][j];
        printf("R%d %d/%d allocated (%.0f%%)  ", j, used, d->total[j], 100.0 * used / d->total[j]);
    }
    printf("\n          processes blocked = %d of %d, detection time = %.1f us\n", cnt, d->n, us);
    if (cnt) printf("          Recovery options: terminate deadlocked processes one by one, or preempt resources and roll back.\n");
}

int main(int argc, char **argv)
{
    System s;
    memset(&s, 0, sizeof s);

    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        printf("Number of processes: "); if (scanf("%d", &s.n) != 1 || s.n < 1 || s.n > MAXP) return 1;
        printf("Number of resource types: "); if (scanf("%d", &s.m) != 1 || s.m < 1 || s.m > MAXR) return 1;
        printf("Allocation matrix (%d x %d):\n", s.n, s.m);
        for (int i = 0; i < s.n; i++) for (int j = 0; j < s.m; j++) if (scanf("%d", &s.alloc[i][j]) != 1) return 1;
        printf("Max matrix (%d x %d):\n", s.n, s.m);
        for (int i = 0; i < s.n; i++) for (int j = 0; j < s.m; j++) if (scanf("%d", &s.max[i][j]) != 1) return 1;
        printf("Available vector (%d values): ", s.m);
        for (int j = 0; j < s.m; j++) if (scanf("%d", &s.avail[j]) != 1) return 1;
    } else {                                            /* classic textbook example */
        int alloc[5][3] = {{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
        int max[5][3]   = {{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};
        int avail[3]    = {3,3,2};
        s.n = 5; s.m = 3;
        for (int i = 0; i < 5; i++) for (int j = 0; j < 3; j++) { s.alloc[i][j] = alloc[i][j]; s.max[i][j] = max[i][j]; }
        for (int j = 0; j < 3; j++) s.avail[j] = avail[j];
    }
    compute_need(&s);
    for (int i = 0; i < s.n; i++) for (int j = 0; j < s.m; j++)
        if (s.need[i][j] < 0) { fprintf(stderr, "Error: Max < Allocation for P%d\n", i); return 1; }

    printf("=============== Part 1: Banker's safety algorithm ===============\n");
    print_state(&s);
    int seq[MAXP];
    if (is_safe(&s, seq)) { printf("\nSystem is in a SAFE state. Safe sequence: "); print_seq(seq, s.n); }
    else printf("\nSystem is in an UNSAFE state (deadlock possible).\n");

    if (argc > 1 && strcmp(argv[1], "-i") == 0) return 0;

    printf("\n=============== Part 2: Resource-request algorithm ===============");
    int r1[3] = {1,0,2}, r2[3] = {3,3,0}, r3[3] = {0,2,0};
    request_resources(&s, 1, r1);       /* granted, still safe                       */
    request_resources(&s, 4, r2);       /* denied: resources not available           */
    request_resources(&s, 0, r3);       /* available but would make state unsafe     */
    printf("\nState after the requests:\n"); print_state(&s);

    printf("\n=============== Part 3: Deadlock detection ===============");
    DSystem d1 = {.n = 5, .m = 3,
        .alloc   = {{0,1,0},{2,0,0},{3,0,3},{2,1,1},{0,0,2}},
        .request = {{0,0,0},{2,0,2},{0,0,0},{1,0,0},{0,0,2}},
        .avail = {0,0,0}, .total = {7,2,6}};
    detection_scenario("Scenario A: no deadlock", &d1);
    DSystem d2 = d1;                                    /* P2 now also requests one more instance of C */
    d2.request[2][2] = 1;
    detection_scenario("Scenario B: P2 requests one more C -> deadlock", &d2);

    printf("\n=============== Part 4: Performance discussion ===============\n"
           " * Banker's algorithm (avoidance) runs on every request: O(m*n^2). It never lets a deadlock happen,\n"
           "   but it may delay requests and keep resource utilization lower because it must keep a safe reserve.\n"
           " * Detection (+ recovery) lets the system run with high utilization and only pays the O(m*n^2) cost\n"
           "   when it is invoked, but the deadlocked processes are blocked until recovery happens.\n"
           " * Scenario A vs B shows how one extra pending request turns a healthy allocation into a deadlock.\n");
    return 0;
}
