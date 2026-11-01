#!/bin/bash
# Problem 5(ii): Check whether a number is positive, negative or zero (if-elif-else)
# Usage: ./5b_positive_negative_zero.sh [number]

if [ $# -ge 1 ]; then n=$1; else read -rp "Enter a number: " n; fi
if ! [[ $n =~ ^[+-]?[0-9]+$ ]]; then echo "Error: please enter an integer." >&2; exit 1; fi

if [ "$n" -gt 0 ]; then
    echo "$n is POSITIVE"
elif [ "$n" -lt 0 ]; then
    echo "$n is NEGATIVE"
else
    echo "The number is ZERO"
fi
