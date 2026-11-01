# Build all C programs:  make        Remove binaries:  make clean
CC     = gcc
CFLAGS = -O2 -Wall -Wextra
LDLIBS = -pthread -lm

PROGS = 07_cpu_scheduling_fcfs_sjf/07_fcfs_sjf \
        08_cpu_scheduling_sjf_rr/08_sjf_rr \
        09_cpu_scheduling_fcfs_rr/09_fcfs_rr \
        10_ipc_pipes_sharedmemory/10_pipe_shm \
        11_ipc_pipes_messagequeues/11_pipe_msgq \
        12_ipc_sharedmemory_messagequeues/12_shm_msgq \
        13_posix_threads/13_pthreads \
        14_producer_consumer/14_producer_consumer \
        15_readers_writers/15_readers_writers \
        16_bankers_deadlock/16_bankers_deadlock \
        17_vm_fifo_lru/17_vm_fifo_lru \
        18_vm_fifo_optimal/18_vm_fifo_optimal \
        19_vm_lru_optimal/19_vm_lru_optimal \
        20_disk_fcfs_sstf/20_disk_fcfs_sstf \
        21_disk_fcfs_scan/21_disk_fcfs_scan \
        22_disk_fcfs_cscan/22_disk_fcfs_cscan

all: $(PROGS)

%: %.c $(wildcard common/*.h)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(PROGS)

.PHONY: all clean
