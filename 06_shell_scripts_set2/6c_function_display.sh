#!/bin/bash
# Problem 6(iii): Display a number entered by the user using a function
# Usage: ./6c_function_display.sh [number]

display_number() {
    echo "The number you entered is: $1"
}

if [ $# -ge 1 ]; then num=$1; else read -rp "Enter a number: " num; fi
display_number "$num"
