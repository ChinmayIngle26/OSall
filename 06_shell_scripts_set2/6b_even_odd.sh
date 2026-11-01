#!/bin/bash
# Problem 6(ii): Check whether a number is even or odd (if-else)
# Usage: ./6b_even_odd.sh [number]

if [ $# -ge 1 ]; then n=$1; else read -rp "Enter an integer: " n; fi
if ! [[ $n =~ ^[+-]?[0-9]+$ ]]; then echo "Error: please enter an integer." >&2; exit 1; fi

if [ $((n % 2)) -eq 0 ]; then
    echo "$n is EVEN"
else
    echo "$n is ODD"
fi
