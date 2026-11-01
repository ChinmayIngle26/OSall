/*
 * ipc_common.h - shared code for Problems 10, 11 and 12
 *
 * Three Inter-Process Communication mechanisms are implemented. In every case a
 * child process (producer) sends NMSG messages of MSGSZ bytes to the parent
 * (consumer). The parent verifies every byte it receives, then reports:
 *   - elapsed (wall-clock) time and throughput          -> communication efficiency
 *   - CPU time spent by both processes                   -> overhead
 *   - peak resident memory of the child                  -> memory utilization
 *
 *   1. Pipe            : pipe(), write(), read()
 *   2. Shared memory   : shmget(), shmat(), POSIX semaphores in shared memory for sync
 *   3. Message queue   : msgget(), msgsnd(), msgrcv()  (System V)
 */
#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

typedef struct {
    const char *name;
    double secs;            /* wall-clock time                      */
    double cpu;             /* user+sys CPU time, parent + child    */
    long   child_rss_kb;    /* peak RSS of child                    */
    double mb_per_s, msgs_per_s;
    int    ok;              /* data verified correctly              */
} IpcResult;

static inline double now_sec(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static inline double tv_sec(struct timeval tv) { return tv.tv_sec + tv.tv_usec / 1e6; }
static inline double self_cpu(void)
{
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    return tv_sec(ru.ru_utime) + tv_sec(ru.ru_stime);
}
static inline void fill_msg(unsigned char *b, int size, int i)
{
    for (int k = 0; k < size; k++) b[k] = (unsigned char)((i + k) & 0xFF);
}
static inline int check_msg(const unsigned char *b, int size, int i)
{
    for (int k = 0; k < size; k++) if (b[k] != (unsigned char)((i + k) & 0xFF)) return 0;
    return 1;
}
static inline IpcResult finalize(const char *name, double t0, double cpu0, pid_t child,
                                 int nmsg, int msgsz, int ok)
{
    int st = 0; struct rusage cr;
    wait4(child, &st, 0, &cr);
    IpcResult r;
    r.name = name; r.secs = now_sec() - t0;
    r.cpu = (self_cpu() - cpu0) + tv_sec(cr.ru_utime) + tv_sec(cr.ru_stime);
    r.child_rss_kb = cr.ru_maxrss;
    r.msgs_per_s = nmsg / r.secs;
    r.mb_per_s = ((double)nmsg * msgsz / (1024.0 * 1024.0)) / r.secs;
    r.ok = ok && WIFEXITED(st) && WEXITSTATUS(st) == 0;
    return r;
}

/* ---------------------------- 1. PIPE ---------------------------- */
static inline IpcResult ipc_pipe(int nmsg, int msgsz)
{
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); exit(1); }
    unsigned char *buf = malloc(msgsz);
    double t0 = now_sec(), c0 = self_cpu();
    pid_t c = fork();
    if (c < 0) { perror("fork"); exit(1); }
    if (c == 0) {                                   /* child: writer */
        close(fd[0]);
        for (int i = 0; i < nmsg; i++) {
            fill_msg(buf, msgsz, i);
            int off = 0;
            while (off < msgsz) {
                ssize_t w = write(fd[1], buf + off, msgsz - off);
                if (w < 0) { if (errno == EINTR) continue; _exit(1); }
                off += (int)w;
            }
        }
        close(fd[1]); _exit(0);
    }
    close(fd[1]);                                   /* parent: reader */
    int ok = 1;
    for (int i = 0; i < nmsg; i++) {
        int off = 0;
        while (off < msgsz) {
            ssize_t r = read(fd[0], buf + off, msgsz - off);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) { ok = 0; goto out; }
            off += (int)r;
        }
        if (!check_msg(buf, msgsz, i)) ok = 0;
    }
out:
    close(fd[0]); free(buf);
    return finalize("Pipe", t0, c0, c, nmsg, msgsz, ok);
}

/* ------------------------ 2. SHARED MEMORY ------------------------ */
/* A ring buffer of SHM_SLOTS slots lives in the shared segment. One producer and one
 * consumer synchronize with two counting semaphores (empty/full), so neither side has to
 * wait for the other after every single message. */
#define SHM_SLOTS 64
typedef struct {
    sem_t empty;            /* counts free slots   (starts at SHM_SLOTS) */
    sem_t full;             /* counts filled slots (starts at 0)         */
    unsigned char data[1];  /* SHM_SLOTS * msgsz bytes follow            */
} ShmRing;

static inline IpcResult ipc_shm(int nmsg, int msgsz)
{
    size_t total = sizeof(ShmRing) + (size_t)SHM_SLOTS * msgsz;
    int id = shmget(IPC_PRIVATE, total, IPC_CREAT | 0600);
    if (id < 0) { perror("shmget"); exit(1); }
    ShmRing *sh = shmat(id, NULL, 0);
    if (sh == (void *)-1) { perror("shmat"); exit(1); }
    sem_init(&sh->empty, 1, SHM_SLOTS);             /* pshared = 1 : shared between processes */
    sem_init(&sh->full, 1, 0);
    unsigned char *buf = malloc(msgsz);
    double t0 = now_sec(), c0 = self_cpu();
    pid_t c = fork();
    if (c < 0) { perror("fork"); exit(1); }
    if (c == 0) {                                   /* child: producer */
        for (int i = 0; i < nmsg; i++) {
            sem_wait(&sh->empty);
            fill_msg(sh->data + (size_t)(i % SHM_SLOTS) * msgsz, msgsz, i);
            sem_post(&sh->full);
        }
        shmdt(sh); _exit(0);
    }
    int ok = 1;                                     /* parent: consumer */
    for (int i = 0; i < nmsg; i++) {
        sem_wait(&sh->full);
        memcpy(buf, sh->data + (size_t)(i % SHM_SLOTS) * msgsz, msgsz);
        sem_post(&sh->empty);
        if (!check_msg(buf, msgsz, i)) ok = 0;
    }
    IpcResult r = finalize("Shared memory", t0, c0, c, nmsg, msgsz, ok);
    sem_destroy(&sh->empty); sem_destroy(&sh->full);
    shmdt(sh); shmctl(id, IPC_RMID, NULL); free(buf);
    return r;
}

/* ------------------------ 3. MESSAGE QUEUE ------------------------ */
static inline IpcResult ipc_msgq(int nmsg, int msgsz)
{
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (qid < 0) { perror("msgget"); exit(1); }
    struct mbuf { long mtype; unsigned char mtext[1]; };
    struct mbuf *m = malloc(sizeof(long) + msgsz);
    m->mtype = 1;
    double t0 = now_sec(), c0 = self_cpu();
    pid_t c = fork();
    if (c < 0) { perror("fork"); exit(1); }
    if (c == 0) {                                   /* child: sender */
        for (int i = 0; i < nmsg; i++) {
            fill_msg(m->mtext, msgsz, i);
            if (msgsnd(qid, m, msgsz, 0) < 0) { perror("msgsnd"); _exit(1); }
        }
        _exit(0);
    }
    int ok = 1;                                     /* parent: receiver */
    for (int i = 0; i < nmsg; i++) {
        ssize_t n = msgrcv(qid, m, msgsz, 1, 0);
        if (n != msgsz) { if (n < 0) perror("msgrcv"); ok = 0; break; }
        if (!check_msg(m->mtext, msgsz, i)) ok = 0;
    }
    IpcResult r = finalize("Message queue", t0, c0, c, nmsg, msgsz, ok);
    msgctl(qid, IPC_RMID, NULL); free(m);
    return r;
}

/* --------------------------- driver --------------------------- */
typedef IpcResult (*ipc_fn)(int, int);

static inline void ipc_print(const IpcResult *r, int msgsz)
{
    printf("%-14s %6d B  %8.4f s  %9.0f msg/s  %8.2f MB/s  cpu %7.4f s  child RSS %5ld KB  %s\n",
           r->name, msgsz, r->secs, r->msgs_per_s, r->mb_per_s, r->cpu, r->child_rss_kb,
           r->ok ? "data OK" : "DATA ERROR");
}

static inline void ipc_notes(const char *which)
{
    if (strstr(which, "Pipe"))
        printf(" * Pipe: unidirectional byte stream in a kernel buffer (64 KB on Linux). Data is copied\n"
               "   user->kernel->user (two copies). No explicit sync needed - read() blocks when empty and\n"
               "   write() blocks when full. Only related processes can share an anonymous pipe.\n");
    if (strstr(which, "Shared"))
        printf(" * Shared memory: one physical region mapped into both address spaces, so no kernel copy\n"
               "   on the data path (data is only copied by the programs themselves). The programmer must provide\n"
               "   synchronization (semaphores here). The segment (64 slots here) is reserved RAM until\n"
               "   shmctl(IPC_RMID). Speed depends on how well synchronization cost is amortised.\n");
    if (strstr(which, "Message"))
        printf(" * Message queue: kernel-managed list of discrete, typed messages (message boundaries are\n"
               "   preserved, receivers can select by type). Each message is copied in and out of the kernel\n"
               "   and the queue has a size limit (msg_qbytes), so it is slower than shared memory.\n");
}

/* Compare two mechanisms for several message sizes */
static inline void ipc_lab(ipc_fn a, ipc_fn b)
{
    const int sizes[] = {64, 1024, 4096};
    const int nmsg = 20000;
    printf("Each test: child sends %d messages to the parent; parent verifies all data.\n\n", nmsg);
    printf("%-14s %8s  %10s  %15s  %13s  %12s  %s\n", "Mechanism", "MsgSize", "Time", "Rate", "Bandwidth", "CPU", "Memory/Status");
    double ta[3], tb[3]; const char *na = NULL, *nb = NULL;
    for (int s = 0; s < 3; s++) {
        IpcResult ra = a(nmsg, sizes[s]); ipc_print(&ra, sizes[s]); ta[s] = ra.mb_per_s; na = ra.name;
        IpcResult rb = b(nmsg, sizes[s]); ipc_print(&rb, sizes[s]); tb[s] = rb.mb_per_s; nb = rb.name;
        printf("\n");
    }
    printf("Analysis\n--------\n");
    for (int s = 0; s < 3; s++)
        printf(" * %5d-byte messages: %s is %.2fx %s than %s\n", sizes[s],
               ta[s] >= tb[s] ? na : nb, ta[s] >= tb[s] ? ta[s] / tb[s] : tb[s] / ta[s], "faster",
               ta[s] >= tb[s] ? nb : na);
    ipc_notes(na); ipc_notes(nb);
    printf(" * Resource sharing: pipes and message queues transfer *copies* of data (processes stay isolated);\n"
           "   shared memory shares the *same* pages, which is efficient but needs synchronization to avoid races.\n");
}

#endif
