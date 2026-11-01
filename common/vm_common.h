/*
 * vm_common.h - shared code for Problems 17, 18 and 19
 *
 *  Part 1  Paging        : logical -> physical address translation with a page table
 *  Part 2  Segmentation  : (segment, offset) -> physical address with base/limit check
 *  Part 3  Virtual memory: demand paging with FIFO / LRU / Optimal page replacement,
 *                          step-by-step trace, comparison for several frame counts,
 *                          hit/fault ratios and effective access time (EAT).
 */
#ifndef VM_COMMON_H
#define VM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXF 16
typedef enum { ALG_FIFO, ALG_LRU, ALG_OPT } Algo;

static inline const char *algo_name(Algo a)
{
    return a == ALG_FIFO ? "FIFO" : a == ALG_LRU ? "LRU" : "Optimal";
}

/* Simulate page replacement. Returns number of page faults. */
static inline int simulate(Algo alg, const int *ref, int n, int nf, int trace)
{
    int frame[MAXF], last_used[MAXF];
    int faults = 0, fifo_next = 0;
    for (int i = 0; i < nf; i++) { frame[i] = -1; last_used[i] = -1; }

    if (trace) {
        printf("\n  %s trace (%d frames)\n  Ref |", algo_name(alg), nf);
        for (int f = 0; f < nf; f++) printf(" F%-2d", f + 1);
        printf("| Result\n  ----+"); for (int f = 0; f < nf; f++) printf("----"); printf("+-------\n");
    }
    for (int t = 0; t < n; t++) {
        int page = ref[t], hit = -1;
        for (int f = 0; f < nf; f++) if (frame[f] == page) { hit = f; break; }
        if (hit >= 0) {
            last_used[hit] = t;
        } else {
            faults++;
            int victim = -1;
            for (int f = 0; f < nf; f++) if (frame[f] == -1) { victim = f; break; }   /* free frame first */
            if (victim < 0) {
                if (alg == ALG_FIFO) {
                    victim = fifo_next; fifo_next = (fifo_next + 1) % nf;
                } else if (alg == ALG_LRU) {
                    victim = 0;
                    for (int f = 1; f < nf; f++) if (last_used[f] < last_used[victim]) victim = f;
                } else {                                    /* Optimal: farthest next use */
                    int far = -1; victim = 0;
                    for (int f = 0; f < nf; f++) {
                        int nxt = n;                        /* n = never used again */
                        for (int k = t + 1; k < n; k++) if (ref[k] == frame[f]) { nxt = k; break; }
                        if (nxt > far) { far = nxt; victim = f; }
                    }
                }
            } else if (alg == ALG_FIFO) {
                fifo_next = (victim + 1) % nf;              /* keep FIFO pointer after initial fill */
            }
            frame[victim] = page; last_used[victim] = t;
        }
        if (trace) {
            printf("  %3d |", page);
            for (int f = 0; f < nf; f++) { if (frame[f] < 0) printf("  - "); else printf(" %2d ", frame[f]); }
            printf("| %s\n", hit >= 0 ? "Hit" : "FAULT");
        }
    }
    return faults;
}

/* ---------------- Part 1: paging ---------------- */
static inline void demo_paging(void)
{
    const int page_size = 1024;                 /* 1 KB pages */
    int page_table[8] = {5, 2, 7, -1, 0, 3, -1, 1};      /* page -> frame, -1 = not in memory */
    unsigned logical[] = {100, 1500, 2100, 3200, 4100, 5300, 6200, 7100, 9000};
    printf("\n=================== Part 1: Paging (page size %d bytes) ===================\n", page_size);
    printf("Page table: "); for (int i = 0; i < 8; i++) { if (page_table[i] >= 0) printf("[%d->F%d] ", i, page_table[i]); else printf("[%d->disk] ", i); }
    printf("\n%-10s %-6s %-8s %-8s %-10s\n", "Logical", "Page", "Offset", "Frame", "Physical");
    for (unsigned k = 0; k < sizeof logical / sizeof logical[0]; k++) {
        unsigned la = logical[k], pg = la / page_size, off = la % page_size;
        if (pg >= 8) printf("%-10u %-6u %-8u %-8s %-10s  (invalid page number -> segmentation fault)\n", la, pg, off, "-", "-");
        else if (page_table[pg] < 0) printf("%-10u %-6u %-8u %-8s %-10s  (PAGE FAULT: page not in memory)\n", la, pg, off, "-", "-");
        else printf("%-10u %-6u %-8u %-8d %-10u\n", la, pg, off, page_table[pg], page_table[pg] * page_size + off);
    }
    printf("Formula: physical = frame * page_size + offset; page = logical / page_size; offset = logical %% page_size\n");
}

/* ---------------- Part 2: segmentation ---------------- */
static inline void demo_segmentation(void)
{
    struct { const char *name; int base, limit; } seg[] = {
        {"Code", 1400, 1000}, {"Data", 6300, 400}, {"Stack", 4300, 1100}, {"Heap", 3200, 800}
    };
    int req[][2] = {{0, 53}, {1, 399}, {2, 1099}, {3, 852}, {1, 400}, {4, 10}};
    printf("\n=================== Part 2: Segmentation ===================\n");
    printf("Segment table: ");
    for (int i = 0; i < 4; i++) printf("[%d %s base=%d limit=%d] ", i, seg[i].name, seg[i].base, seg[i].limit);
    printf("\n%-9s %-8s %-10s\n", "Segment", "Offset", "Physical");
    for (unsigned k = 0; k < sizeof req / sizeof req[0]; k++) {
        int s = req[k][0], o = req[k][1];
        if (s < 0 || s >= 4) printf("%-9d %-8d TRAP: invalid segment number\n", s, o);
        else if (o >= seg[s].limit) printf("%-9d %-8d TRAP: offset >= limit (%d) -> addressing error\n", s, o, seg[s].limit);
        else printf("%-9d %-8d %-10d\n", s, o, seg[s].base + o);
    }
}

/* ---------------- Part 3: virtual memory ---------------- */
static inline void compare_on(const char *title, const int *ref, int n, Algo a, Algo b, int trace_frames)
{
    const double MEM_NS = 100.0, FAULT_NS = 8e6;      /* 100 ns memory access, 8 ms page-fault service */
    printf("\n--- Reference string: %s ---\n  ", title);
    for (int i = 0; i < n; i++) printf("%d ", ref[i]);
    printf("\n");
    if (trace_frames) { simulate(a, ref, n, trace_frames, 1); simulate(b, ref, n, trace_frames, 1); }

    printf("\n  %-7s | %-9s %-6s %-7s %-9s %-12s | %-9s %-6s %-7s %-9s %-12s\n", "Frames",
           algo_name(a), "Hits", "Faults", "Hit %", "EAT (ns)", algo_name(b), "Hits", "Faults", "Hit %", "EAT (ns)");
    int win_a = 0, win_b = 0, ties = 0;
    for (int f = 1; f <= 7; f++) {
        int fa = simulate(a, ref, n, f, 0), fb = simulate(b, ref, n, f, 0);
        double pa = (double)fa / n, pb = (double)fb / n;
        printf("  %-7d | %-9s %-6d %-7d %-9.1f %-12.0f | %-9s %-6d %-7d %-9.1f %-12.0f\n", f, "",
               n - fa, fa, 100.0 * (1 - pa), (1 - pa) * MEM_NS + pa * FAULT_NS,
               "", n - fb, fb, 100.0 * (1 - pb), (1 - pb) * MEM_NS + pb * FAULT_NS);
        if (fa < fb) win_a++; else if (fb < fa) win_b++; else ties++;
    }
    printf("  => %s had fewer faults in %d cases, %s in %d cases, %d ties.\n",
           algo_name(a), win_a, algo_name(b), win_b, ties);
}

static inline void run_vm_lab(Algo a, Algo b)
{
    int ref1[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};          /* classic string        */
    int ref2[] = {1,2,3,4,1,2,5,1,2,3,4,5};                          /* shows Belady's anomaly */
    int ref3[] = {1,2,3,4,2,1,5,6,2,1,2,3,7,6,3,2,1,2,3,6};          /* mix of locality        */

    demo_paging();
    demo_segmentation();

    printf("\n=================== Part 3: Virtual memory - %s vs %s ===================", algo_name(a), algo_name(b));
    compare_on("classic (20 references)", ref1, 20, a, b, 3);
    compare_on("Belady string (12 references)", ref2, 12, a, b, 0);
    compare_on("locality string (20 references)", ref3, 20, a, b, 0);

    printf("\nAnalysis:\n"
           " * Page-fault ratio is the key metric: EAT = (1-p) x memory time + p x fault service time,\n"
           "   so even a small fault rate dominates (fault service ~8 ms vs ~100 ns for a hit).\n"
           " * FIFO is simple but ignores usage, and can suffer Belady's anomaly: more frames can give MORE\n"
           "   faults (see the Belady string with 3 vs 4 frames).\n"
           " * LRU approximates optimal using the past; it is a stack algorithm, so it never shows Belady's\n"
           "   anomaly, but needs hardware/software support to track recency.\n"
           " * Optimal gives the minimum possible faults but needs future knowledge, so it is only a benchmark.\n"
           " * With enough frames to hold the working set, all algorithms converge to the same (compulsory) faults.\n");
}

#endif
