#!/bin/bash
# Problem 6(i): Factorial of a number using for, while and until loops
# Usage: ./6a_factorial.sh [number]     (0 <= number <= 20 fits in 64-bit integers)

if [ $# -ge 1 ]; then n=$1; else read -rp "Enter a non-negative integer: " n; fi
if ! [[ $n =~ ^[0-9]+$ ]]; then echo "Error: enter a non-negative integer." >&2; exit 1; fi
if [ "$n" -gt 20 ]; then echo "Error: numbers above 20 overflow 64-bit integers." >&2; exit 1; fi

# for loop
fact=1
for ((i = 2; i <= n; i++)); do fact=$((fact * i)); done
echo "Factorial of $n using for   loop = $fact"

# while loop
fact=1; i=1
while [ $i -le "$n" ]; do fact=$((fact * i)); i=$((i + 1)); done
echo "Factorial of $n using while loop = $fact"

# until loop
fact=1; i=1
until [ $i -gt "$n" ]; do fact=$((fact * i)); i=$((i + 1)); done
echo "Factorial of $n using until loop = $fact"
