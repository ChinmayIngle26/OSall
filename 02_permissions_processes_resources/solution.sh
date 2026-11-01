#!/bin/bash
# Problem 2: File permissions, ownership, process monitoring and resource usage
# Usage: bash solution.sh

# helper: run a command only if it is installed
run() { if command -v "$1" >/dev/null 2>&1; then "$@"; else echo "($1 is not installed on this system)"; fi; }

mkdir -p perm_demo && cd perm_demo || exit 1

echo "===== i. Create files with different permissions ====="
touch file_a.txt file_b.txt file_c.sh
echo '#!/bin/bash' > file_c.sh; echo 'echo "script executed"' >> file_c.sh
chmod 644 file_a.txt        # rw-r--r--
chmod 600 file_b.txt        # rw-------
chmod 755 file_c.sh         # rwxr-xr-x
ls -l
echo "--- Detailed (stat) ---"
stat -c '%A (%a)  owner=%U group=%G  %n' file_a.txt file_b.txt file_c.sh

echo
echo "===== ii. Modify permissions: symbolic and numeric modes ====="
echo "-- Symbolic mode --"
chmod u+x file_a.txt;   echo "u+x  file_a.txt ->" $(stat -c '%A' file_a.txt)
chmod g+w file_a.txt;   echo "g+w  file_a.txt ->" $(stat -c '%A' file_a.txt)
chmod o-r file_a.txt;   echo "o-r  file_a.txt ->" $(stat -c '%A' file_a.txt)
chmod a=r file_a.txt;   echo "a=r  file_a.txt ->" $(stat -c '%A' file_a.txt)
chmod u=rw,g=r,o= file_b.txt; echo "u=rw,g=r,o=  file_b.txt ->" $(stat -c '%A' file_b.txt)
echo "-- Numeric (octal) mode --"
chmod 777 file_b.txt;   echo "777 file_b.txt ->" $(stat -c '%A (%a)' file_b.txt)
chmod 640 file_b.txt;   echo "640 file_b.txt ->" $(stat -c '%A (%a)' file_b.txt)
chmod 700 file_c.sh;    echo "700 file_c.sh  ->" $(stat -c '%A (%a)' file_c.sh)
echo "Run the script (needs x permission):"; ./file_c.sh

echo
echo "===== iii. Current user, group and ownership change ====="
echo "User  : $(whoami)   (uid=$(id -u))"
echo "Group : $(id -gn)   (gid=$(id -g))"
echo "All groups: $(id -Gn)"
id
echo "-- Change group to my own primary group (always allowed) --"
chgrp "$(id -gn)" file_a.txt && ls -l file_a.txt
echo "-- Change owner (needs administrator privileges) --"
if [ "$(id -u)" -eq 0 ]; then
    chown nobody file_a.txt 2>/dev/null && echo "Owner changed to nobody (running as root)"
    ls -l file_a.txt
elif command -v sudo >/dev/null 2>&1 && sudo -n true 2>/dev/null; then
    sudo chown root:root file_a.txt && echo "Owner changed to root:root using sudo"
    ls -l file_a.txt
else
    echo "Administrator privileges are not available -> skipping chown."
    echo "With privileges you would run:  sudo chown root:root file_a.txt"
fi

echo
echo "===== iv. Running processes and their PIDs ====="
echo "PID of this shell : $$"
echo "PPID of this shell: $PPID"
echo "--- ps -ef (first 15 lines) ---"
ps -ef | head -15
echo "--- ps with chosen columns, sorted by CPU ---"
ps -eo pid,ppid,user,%cpu,%mem,comm --sort=-%cpu | head -10
echo "--- PID of a given process name (pgrep bash) ---"
run pgrep bash

echo
echo "===== v. CPU, memory and disk-space usage ====="
echo "--- uptime / load average ---"; run uptime
echo "--- top (one snapshot) ---";    if command -v top >/dev/null 2>&1; then top -b -n 1 | head -15; else echo "(top not installed)"; fi
echo "--- memory (free -h) ---";      run free -h
echo "--- vmstat (3 samples) ---";    run vmstat 1 3
echo "--- disk space (df -h) ---";    df -h | head -10
echo "--- directory sizes (du) ---";  du -sh . 2>/dev/null
echo "--- CPU info ---";              grep -m1 'model name' /proc/cpuinfo; echo "CPU cores: $(nproc)"

cd .. && rm -rf perm_demo
