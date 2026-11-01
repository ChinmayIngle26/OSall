/*
 * disk_common.h - shared code for Problems 20, 21 and 22
 *
 *  Part 1  File allocation : contiguous, linked and indexed allocation on a simulated disk,
 *                            with storage utilization, internal and external fragmentation.
 *  Part 2  Disk scheduling : FCFS, SSTF, SCAN and C-SCAN with total head movement,
 *                            estimated access time and throughput.
 */
#ifndef DISK_COMMON_H
#define DISK_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { D_FCFS, D_SSTF, D_SCAN, D_CSCAN } DiskAlgo;

static inline const char *disk_name(DiskAlgo a)
{
    switch (a) { case D_FCFS: return "FCFS"; case D_SSTF: return "SSTF"; case D_SCAN: return "SCAN"; default: return "C-SCAN"; }
}

/* ======================= Part 1: file allocation ======================= */
#define NBLOCKS 64
#define BLOCK_SIZE 1024                       /* bytes per disk block */
#define PTR_BYTES 4                           /* size of a block pointer */

typedef struct { const char *name; int bytes; } FileSpec;

static int disk_map[NBLOCKS];                 /* 0 = free, otherwise file number (1..) */

static inline void disk_clear(void) { memset(disk_map, 0, sizeof disk_map); }
static inline int blocks_free(void) { int c = 0; for (int i = 0; i < NBLOCKS; i++) c += disk_map[i] == 0; return c; }
static inline void disk_show(void)
{
    printf("  Disk map (. = free, digit = file number):\n  ");
    for (int i = 0; i < NBLOCKS; i++) {
        printf("%c", disk_map[i] ? (char)('0' + disk_map[i] % 10) : '.');
        if (i % 32 == 31) printf("\n  ");
    }
}

/* largest run of consecutive free blocks */
static inline int largest_hole(void)
{
    int best = 0, cur = 0;
    for (int i = 0; i < NBLOCKS; i++) { if (!disk_map[i]) { cur++; if (cur > best) best = cur; } else cur = 0; }
    return best;
}

/* Allocate a file with method m (0 contiguous, 1 linked, 2 indexed). Returns blocks used or -1 on failure. */
static inline int alloc_file(int m, int fid, int bytes, char *info, size_t isz)
{
    int usable = (m == 1) ? BLOCK_SIZE - PTR_BYTES : BLOCK_SIZE;       /* linked: a pointer lives in every block */
    int data = (bytes + usable - 1) / usable;
    int total = data + (m == 2 ? 1 : 0);                               /* indexed: one extra index block */
    info[0] = '\0';
    if (m == 0) {                                                      /* first-fit run of consecutive blocks */
        int start = -1, run = 0;
        for (int i = 0; i < NBLOCKS && start < 0; i++) { if (!disk_map[i]) { if (++run == total) start = i - total + 1; } else run = 0; }
        if (start < 0) { snprintf(info, isz, "needs %d contiguous blocks - FAILED (largest hole %d, total free %d)", total, largest_hole(), blocks_free()); return -1; }
        for (int i = 0; i < total; i++) disk_map[start + i] = fid;
        snprintf(info, isz, "blocks %d..%d (%d blocks)", start, start + total - 1, total);
        return total;
    }
    if (total > blocks_free()) { snprintf(info, isz, "needs %d blocks - FAILED (only %d free)", total, blocks_free()); return -1; }
    int got = 0; size_t len = 0;
    for (int i = 0; i < NBLOCKS && got < total; i++) {                 /* any free block will do */
        if (disk_map[i]) continue;
        disk_map[i] = fid; got++;
        if (len + 8 < isz) len += snprintf(info + len, isz - len, "%s%d", got == 1 ? "" : (m == 1 ? "->" : ","), i);
    }
    if (m == 2 && len + 24 < isz) snprintf(info + len, isz - len, "  (first block = index block)");
    if (m == 1 && len + 16 < isz) snprintf(info + len, isz - len, "  (chain of %d blocks)", total);
    return total;
}

static inline void file_allocation_demo(void)
{
    FileSpec files[] = {{"A.txt", 5000}, {"B.dat", 2500}, {"C.log", 9000}, {"D.cfg", 700}, {"E.bin", 4100}, {"F.mp3", 12000}};
    FileSpec big = {"G.iso", 31000};
    int nf = sizeof files / sizeof files[0];
    const char *method[] = {"Contiguous (first fit)", "Linked", "Indexed"};
    char info[256];

    printf("\n==================== Part 1: File allocation methods ====================\n");
    printf("Disk: %d blocks x %d bytes = %d bytes. Files:", NBLOCKS, BLOCK_SIZE, NBLOCKS * BLOCK_SIZE);
    for (int i = 0; i < nf; i++) printf(" %s(%dB)", files[i].name, files[i].bytes);
    printf("\nScenario: store the 6 files, delete B.dat, D.cfg and E.bin, then store %s (%d bytes).\n", big.name, big.bytes);

    for (int m = 0; m < 3; m++) {
        disk_clear();
        long payload = 0; 
        printf("\n--- %s allocation ---\n", method[m]);
        for (int f = 0; f < nf; f++) {
            int r = alloc_file(m, f + 1, files[f].bytes, info, sizeof info);
            printf("  %-6s -> %s\n", files[f].name, info);
            if (r > 0) payload += files[f].bytes;
        }
        disk_show();
        int used = NBLOCKS - blocks_free();
        printf("  After storing 6 files: %d/%d blocks used (%.1f%%), useful data %ld bytes, storage efficiency %.1f%%\n",
               used, NBLOCKS, 100.0 * used / NBLOCKS, payload, 100.0 * payload / ((double)used * BLOCK_SIZE));
        printf("  Internal fragmentation + pointer/index overhead = %ld bytes\n", (long)used * BLOCK_SIZE - payload);

        /* delete B.dat (file 2), D.cfg (file 4) and E.bin (file 5) */
        for (int i = 0; i < NBLOCKS; i++) if (disk_map[i] == 2 || disk_map[i] == 4 || disk_map[i] == 5) disk_map[i] = 0;
        printf("  Deleted B.dat, D.cfg and E.bin -> free blocks: %d, largest contiguous hole: %d\n", blocks_free(), largest_hole());
        int r = alloc_file(m, 7, big.bytes, info, sizeof info);
        printf("  %-6s -> %s\n", big.name, info);
        if (r < 0 && m == 0 && blocks_free() >= (big.bytes + BLOCK_SIZE - 1) / BLOCK_SIZE)
            printf("  => EXTERNAL FRAGMENTATION: enough free blocks in total, but not in one contiguous run.\n");
        else if (r > 0 && m != 0)
            printf("  => Stored successfully: blocks need not be adjacent, so there is no external fragmentation.\n");
        disk_show();
        printf("  Access: %s\n", m == 0 ? "sequential and direct access are fast (one seek), but files cannot grow easily and holes appear"
                                  : m == 1 ? "good for sequential access; random access to block k needs k pointer reads; a damaged pointer loses the rest of the file"
                                           : "direct access through the index block (1 extra read); no external fragmentation; index block is overhead for tiny files");
    }
}

/* ======================= Part 2: disk scheduling ======================= */
#define MAXREQ 64

static int cmp_int(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

/* Builds the head path (starting at head). Returns path length; sets *jump to the C-SCAN return distance. */
static inline int schedule_path(DiskAlgo alg, const int *req, int n, int head, int maxcyl, int dir_up, int *path, int *jump)
{
    int len = 0; *jump = 0;
    path[len++] = head;
    if (alg == D_FCFS) {
        for (int i = 0; i < n; i++) path[len++] = req[i];
    } else if (alg == D_SSTF) {
        int done[MAXREQ] = {0}, cur = head;
        for (int k = 0; k < n; k++) {
            int best = -1, bd = 1 << 30;
            for (int i = 0; i < n; i++) if (!done[i] && abs(req[i] - cur) < bd) { bd = abs(req[i] - cur); best = i; }
            done[best] = 1; cur = req[best]; path[len++] = cur;
        }
    } else {
        int s[MAXREQ]; memcpy(s, req, n * sizeof(int)); qsort(s, n, sizeof(int), cmp_int);
        int split = 0; while (split < n && s[split] < head) split++;           /* s[0..split-1] below head */
        if (alg == D_SCAN) {
            if (dir_up) {
                for (int i = split; i < n; i++) path[len++] = s[i];
                if (split > 0) { if (path[len - 1] != maxcyl) path[len++] = maxcyl; for (int i = split - 1; i >= 0; i--) path[len++] = s[i]; }
            } else {
                for (int i = split - 1; i >= 0; i--) path[len++] = s[i];
                if (split < n) { if (path[len - 1] != 0) path[len++] = 0; for (int i = split; i < n; i++) path[len++] = s[i]; }
            }
        } else {                                                                /* C-SCAN: always sweep upward, then wrap to 0 */
            for (int i = split; i < n; i++) path[len++] = s[i];
            if (split > 0) {
                if (path[len - 1] != maxcyl) path[len++] = maxcyl;
                path[len++] = 0; *jump = maxcyl;                                /* return sweep (no requests served) */
                for (int i = 0; i < split; i++) path[len++] = s[i];
            }
        }
    }
    return len;
}

typedef struct { int movement; double avg_seek; double access_ms; double throughput; } DiskStat;

static inline DiskStat run_disk(DiskAlgo alg, const int *req, int n, int head, int maxcyl, int dir_up)
{
    /* timing model: 0.1 ms per cylinder moved, 4.17 ms average rotational latency (7200 rpm), 0.1 ms transfer */
    const double SEEK_PER_CYL = 0.1, ROT_MS = 4.17, XFER_MS = 0.1;
    int path[4 * MAXREQ], jump, len = schedule_path(alg, req, n, head, maxcyl, dir_up, path, &jump);
    int mv = 0;
    printf("\n  %s%s: ", disk_name(alg), alg == D_SCAN ? (dir_up ? " (towards higher cylinders first)" : " (towards cylinder 0 first)") : "");
    for (int i = 0; i < len; i++) {
        printf("%d", path[i]);
        if (i < len - 1) { mv += abs(path[i + 1] - path[i]); printf(" -> "); }
    }
    DiskStat d;
    d.movement = mv;
    d.avg_seek = (double)mv / n;
    d.access_ms = mv * SEEK_PER_CYL + n * (ROT_MS + XFER_MS);
    d.throughput = n / (d.access_ms / 1000.0);
    printf("\n     total head movement = %d cylinders", mv);
    if (jump) printf(" (includes %d-cylinder return sweep; %d if the return is not counted)", jump, mv - jump);
    printf("\n     average seek = %.2f cyl/request, total access time = %.2f ms, throughput = %.1f requests/s\n",
           d.avg_seek, d.access_ms, d.throughput);
    return d;
}

static inline void run_disk_lab(DiskAlgo a, DiskAlgo b)
{
    int req[] = {98, 183, 37, 122, 14, 124, 65, 67};
    int n = sizeof req / sizeof req[0], head = 53, maxcyl = 199;

    file_allocation_demo();

    printf("\n==================== Part 2: Disk scheduling - %s vs %s ====================\n", disk_name(a), disk_name(b));
    printf("Disk with cylinders 0..%d, initial head position %d\nRequest queue: ", maxcyl, head);
    for (int i = 0; i < n; i++) printf("%d ", req[i]);
    printf("\n");
    DiskStat sa = run_disk(a, req, n, head, maxcyl, 0);          /* SCAN direction: towards 0 first */
    DiskStat sb = run_disk(b, req, n, head, maxcyl, 0);

    /* second workload: heavier random queue to see behaviour on larger input (fixed seed -> repeatable) */
    int req2[32]; srand(42);
    for (int i = 0; i < 32; i++) req2[i] = rand() % (maxcyl + 1);
    printf("\n--- Second workload: 32 pseudo-random requests (seed 42), head at 100 ---\n");
    DiskStat ta = run_disk(a, req2, 32, 100, maxcyl, 1);
    DiskStat tb = run_disk(b, req2, 32, 100, maxcyl, 1);

    printf("\n==================== Comparison ====================\n");
    printf("%-26s %-14s %-14s\n", "Metric", disk_name(a), disk_name(b));
    printf("%-26s %-14d %-14d\n", "Head movement (8 req)", sa.movement, sb.movement);
    printf("%-26s %-14.2f %-14.2f\n", "Access time ms (8 req)", sa.access_ms, sb.access_ms);
    printf("%-26s %-14.1f %-14.1f\n", "Throughput req/s (8 req)", sa.throughput, sb.throughput);
    printf("%-26s %-14d %-14d\n", "Head movement (32 req)", ta.movement, tb.movement);
    printf("%-26s %-14.2f %-14.2f\n", "Access time ms (32 req)", ta.access_ms, tb.access_ms);
    printf("%-26s %-14.1f %-14.1f\n", "Throughput req/s (32 req)", ta.throughput, tb.throughput);
    printf("\nAnalysis:\n"
           " * FCFS is fair and simple but ignores head position -> the largest head movement on random queues.\n"
           " * SSTF greedily picks the nearest request: far less movement, but distant requests can starve.\n"
           " * SCAN (elevator) sweeps in one direction then reverses: bounded waiting, good under heavy load.\n"
           " * C-SCAN sweeps one way and jumps back: more uniform waiting time than SCAN at the price of a long return seek.\n"
           " * Throughput rises as total head movement falls; rotational latency and transfer time are the same for all.\n"
           " * Allocation: contiguous gives the fastest access but suffers external fragmentation; linked wastes pointer\n"
           "   space and is slow for random access; indexed supports direct access with one index block of overhead.\n");
}

#endif
