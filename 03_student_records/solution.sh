#!/bin/bash
# Problem 3: Student_Records directory management
# Usage: bash solution.sh

echo "===== i. Create separate directories for FY, SY, TE and BE ====="
mkdir -p Student_Records/{FY,SY,TE,BE}
ls -l Student_Records

echo
echo "===== ii. Create text files with student names and roll numbers ====="
cat > Student_Records/FY/fy_students.txt <<EOF
Roll No   Name
101       Aarav Sharma
102       Isha Patil
103       Rohan Deshmukh
EOF
cat > Student_Records/SY/sy_students.txt <<EOF
Roll No   Name
201       Neha Kulkarni
202       Siddharth Joshi
203       Priya Nair
EOF
cat > Student_Records/TE/te_students.txt <<EOF
Roll No   Name
301       Karan Mehta
302       Sneha Pawar
303       Vikram Singh
EOF
cat > Student_Records/BE/be_students.txt <<EOF
Roll No   Name
401       Anjali Rao
402       Tanmay Gokhale
403       Meera Iyer
EOF
for f in Student_Records/*/*.txt; do echo "--- $f ---"; cat "$f"; done

echo
echo "===== iii. Copy selected files into another directory ====="
mkdir -p Student_Records/Selected
cp Student_Records/FY/fy_students.txt Student_Records/TE/te_students.txt Student_Records/Selected/
ls -l Student_Records/Selected

echo
echo "===== iv. Rename and delete specified files ====="
mv Student_Records/SY/sy_students.txt Student_Records/SY/sy_students_2025.txt && echo "Renamed sy_students.txt -> sy_students_2025.txt"
rm -v Student_Records/Selected/fy_students.txt
rm -v Student_Records/BE/be_students.txt

echo
echo "===== v. Complete directory structure ====="
if command -v tree >/dev/null 2>&1; then
    tree Student_Records
else
    echo "(tree not installed - using find)"
    find Student_Records | sort | sed -e 's;[^/]*/;|____;g;s;____|;  |;g'
fi
