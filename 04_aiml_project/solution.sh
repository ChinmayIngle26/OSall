#!/bin/bash
# Problem 4: AIML_Project - files, permissions and ownership
# Usage: bash solution.sh

show() { stat -c '%A (%a)  %U:%G  %n' "$@"; }

echo "===== i. Create project directory and files ====="
mkdir -p AIML_Project && cd AIML_Project || exit 1
echo "Student list: Aarav, Isha, Rohan"  > students.txt
echo "Faculty list: Dr. Kulkarni, Prof. Shah" > faculty.txt
echo "Admin list: Mr. Patil, Ms. Joshi"  > admin.txt
printf '#!/bin/bash\necho "Admin script running"\n' > run_admin.sh
ls -l

echo
echo "===== ii. Assign different permissions ====="
chmod 644 students.txt     # owner rw, group r, others r  (everyone may read)
chmod 640 faculty.txt      # owner rw, group r, others none
chmod 600 admin.txt        # owner rw only
chmod 700 run_admin.sh     # owner rwx only
echo "students.txt=644, faculty.txt=640, admin.txt=600, run_admin.sh=700"

echo
echo "===== iii. Ownership and permission details ====="
ls -l
echo "--- stat summary (perm, owner:group, name) ---"
show students.txt faculty.txt admin.txt run_admin.sh
echo "Current user: $(whoami), group: $(id -gn)"

echo
echo "===== iv. Change permissions: symbolic and numeric ====="
echo "-- symbolic --"
chmod g+w students.txt;  echo "g+w students.txt  ->" $(stat -c '%A' students.txt)
chmod o-r students.txt;  echo "o-r students.txt  ->" $(stat -c '%A' students.txt)
chmod u+x faculty.txt;   echo "u+x faculty.txt   ->" $(stat -c '%A' faculty.txt)
chmod go= admin.txt;     echo "go=  admin.txt    ->" $(stat -c '%A' admin.txt)
echo "-- numeric --"
chmod 750 faculty.txt;   echo "750 faculty.txt   ->" $(stat -c '%A (%a)' faculty.txt)
chmod 664 students.txt;  echo "664 students.txt  ->" $(stat -c '%A (%a)' students.txt)
chmod 400 admin.txt;     echo "400 admin.txt     ->" $(stat -c '%A (%a)' admin.txt)

echo
echo "===== v. Effect of read, write and execute permissions ====="
if [ "$(id -u)" -eq 0 ]; then
    echo "NOTE: you are running as root. Root bypasses permission checks, so the"
    echo "      'denied' results below will not appear. Run as a normal user to see them."
fi
echo "-- READ: remove read permission (000) and try to cat --"
chmod 000 faculty.txt
cat faculty.txt 2>&1 || echo ">> Read failed: no 'r' permission"
chmod 640 faculty.txt
cat faculty.txt && echo ">> Read works after restoring 'r'"

echo "-- WRITE: make file read-only (444) and try to append --"
chmod 444 students.txt
( echo "new line" >> students.txt ) 2>&1 || echo ">> Write failed: no 'w' permission"
chmod 644 students.txt
( echo "new line" >> students.txt ) && echo ">> Write works after adding 'w'"

echo "-- EXECUTE: script without x, then with x --"
chmod 600 run_admin.sh
./run_admin.sh 2>&1 || echo ">> Execute failed: no 'x' permission"
chmod 700 run_admin.sh
./run_admin.sh && echo ">> Execute works after adding 'x'"

echo "-- DIRECTORY: x on a directory is needed to enter it --"
mkdir -p secret_dir && echo "inside" > secret_dir/data.txt
chmod 600 secret_dir
cat secret_dir/data.txt 2>&1 || echo ">> Cannot access files: directory has no 'x' permission"
chmod 700 secret_dir
cat secret_dir/data.txt && echo ">> Access works with 'x' on the directory"
chmod -R u+rwx . 2>/dev/null
cd ..
