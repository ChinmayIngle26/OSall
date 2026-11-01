#!/bin/bash
# Problem 5(i): Multiplication table using for, while and until loops
# Usage: ./5a_multiplication_table.sh [number]

if [ $# -ge 1 ]; then n=$1; else read -rp "Enter a number: " n; fi
if ! [[ $n =~ ^-?[0-9]+$ ]]; then echo "Error: please enter an integer." >&2; exit 1; fi

echo "--- Using for loop ---"
for ((i = 1; i <= 10; i++)); do
    echo "$n x $i = $((n * i))"
done

echo "--- Using while loop ---"
i=1
while [ $i -le 10 ]; do
    echo "$n x $i = $((n * i))"
    i=$((i + 1))
done

echo "--- Using until loop ---"
i=1
until [ $i -gt 10 ]; do
    echo "$n x $i = $((n * i))"
    i=$((i + 1))
done
