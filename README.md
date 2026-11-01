# Operating Systems Lab (OSL) - Solutions to all 23 problems

Tested on Linux (Ubuntu, gcc, bash). C programs use POSIX APIs (fork, pipes, System V IPC, pthreads),
so they need Linux (or WSL on Windows). Docker (problem 23) needs Docker installed.

## Quick start
```bash
unzip OSL_Solutions.zip && cd OSL_Solutions
make                  # compiles all C programs (problems 7-22)
bash run_all_tests.sh # optional: builds and runs everything, saving output in test_output/
```
Shell-script problems are run directly, e.g. `bash 01_file_directory_operations/solution.sh`
(they create their demo files/folders in the *current* directory, so run them from a scratch folder).

## Index
| # | Topic | How to run |
|---|-------|-----------|
| 1 | File and directory operations | `bash 01_file_directory_operations/solution.sh` |
| 2 | Permissions, ownership, processes, resources | `bash 02_permissions_processes_resources/solution.sh` |
| 3 | Student_Records directory | `bash 03_student_records/solution.sh` |
| 4 | AIML_Project permissions | `bash 04_aiml_project/solution.sh` (run as a normal user to see "Permission denied" effects) |
| 5 | Shell scripts: table, +/-/0, file info | `05_shell_scripts_set1/5a_..., 5b_..., 5c_...` (number/file as argument, or prompted) |
| 6 | Shell scripts: factorial, even/odd, function | `06_shell_scripts_set2/6a_..., 6b_..., 6c_...` |
| 7 | fork() + CPU scheduling FCFS, SJF | `./07_cpu_scheduling_fcfs_sjf/07_fcfs_sjf [-i]` |
| 8 | fork() + CPU scheduling SJF, Round Robin | `./08_cpu_scheduling_sjf_rr/08_sjf_rr [-i]` |
| 9 | fork() + CPU scheduling FCFS, Round Robin | `./09_cpu_scheduling_fcfs_rr/09_fcfs_rr [-i]` |
| 10 | IPC: pipes + shared memory | `./10_ipc_pipes_sharedmemory/10_pipe_shm` |
| 11 | IPC: pipes + message queues | `./11_ipc_pipes_messagequeues/11_pipe_msgq` |
| 12 | IPC: shared memory + message queues | `./12_ipc_sharedmemory_messagequeues/12_shm_msgq` |
| 13 | POSIX threads, performance vs thread count | `./13_posix_threads/13_pthreads` |
| 14 | Producer-Consumer (semaphores + mutex) | `./14_producer_consumer/14_producer_consumer` |
| 15 | Readers-Writers (semaphores + mutex) | `./15_readers_writers/15_readers_writers` |
| 16 | Banker's algorithm + deadlock detection | `./16_bankers_deadlock/16_bankers_deadlock [-i]` |
| 17 | Paging/segmentation/VM: FIFO, LRU | `./17_vm_fifo_lru/17_vm_fifo_lru` |
| 18 | Paging/segmentation/VM: FIFO, Optimal | `./18_vm_fifo_optimal/18_vm_fifo_optimal` |
| 19 | Paging/segmentation/VM: LRU, Optimal | `./19_vm_lru_optimal/19_vm_lru_optimal` |
| 20 | File allocation + disk scheduling FCFS, SSTF | `./20_disk_fcfs_sstf/20_disk_fcfs_sstf` |
| 21 | File allocation + disk scheduling FCFS, SCAN | `./21_disk_fcfs_scan/21_disk_fcfs_scan` |
| 22 | File allocation + disk scheduling FCFS, C-SCAN | `./22_disk_fcfs_cscan/22_disk_fcfs_cscan` |
| 23 | AI/ML app in Docker + monitoring | `cd 23_docker_aiml_deployment && bash run_and_monitor.sh` |

## Layout notes
* `common/` holds the shared headers used by the paired problems (scheduling, IPC, virtual memory, disk), so each
  problem's `.c` file is short and only chooses which two algorithms/mechanisms to compare.
* Every program prints the results AND an analysis section (waiting/turnaround time, throughput, memory use, etc.).
* Problem 13's task text in the PDF was cut off after "under different thread"; it is implemented as measuring
  CPU utilization, memory and execution time for 1, 2, 4, 8 and 16 threads.
* Speed-up in problem 13 and the numbers in problems 10-15 depend on your machine (number of CPU cores).

## Tips for viva / report
* Be ready to explain: fork/wait, the three scheduling algorithms and their Gantt charts, why shared memory needs
  semaphores, the Banker's safe sequence, Belady's anomaly (FIFO with 3 vs 4 frames), and SCAN vs C-SCAN head movement.
