#!/bin/bash
# Builds everything and runs every solution once (non-interactive). Output goes to test_output/.
cd "$(dirname "$0")" || exit 1
make -s || { echo "Build failed"; exit 1; }
mkdir -p test_output; WORK=$(mktemp -d); ROOT=$PWD; pass=0; fail=0
check() { if [ $? -eq 0 ]; then echo "PASS  $1"; pass=$((pass+1)); else echo "FAIL  $1"; fail=$((fail+1)); fi; }

for d in 01_file_directory_operations 02_permissions_processes_resources 03_student_records 04_aiml_project; do
    (cd "$WORK" && bash "$ROOT/$d/solution.sh") > test_output/$d.txt 2>&1; check "$d"
done
bash 05_shell_scripts_set1/5a_multiplication_table.sh 5   > test_output/05a.txt 2>&1; check 05a
bash 05_shell_scripts_set1/5b_positive_negative_zero.sh -3 > test_output/05b.txt 2>&1; check 05b
bash 05_shell_scripts_set1/5c_file_info.sh Makefile        > test_output/05c.txt 2>&1; check 05c
bash 06_shell_scripts_set2/6a_factorial.sh 6               > test_output/06a.txt 2>&1; check 06a
bash 06_shell_scripts_set2/6b_even_odd.sh 9                > test_output/06b.txt 2>&1; check 06b
bash 06_shell_scripts_set2/6c_function_display.sh 77       > test_output/06c.txt 2>&1; check 06c
for p in $(grep -oE '[0-9]{2}_[a-z_]+/[0-9]{2}_[a-z_]+' Makefile | sort -u); do
    timeout 120 ./$p > test_output/$(basename $p).txt 2>&1; check "$(basename $p)"
done
rm -rf "$WORK"
echo "Passed: $pass   Failed: $fail   (outputs saved in test_output/)"
