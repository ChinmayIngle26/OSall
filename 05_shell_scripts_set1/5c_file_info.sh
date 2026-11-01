#!/bin/bash
# Problem 5(iii): Check whether a file exists; display its size and basic information
# Usage: ./5c_file_info.sh [filename]

if [ $# -ge 1 ]; then f=$1; else read -rp "Enter file name/path: " f; fi

if [ -e "$f" ]; then
    echo "File '$f' EXISTS."
    if   [ -f "$f" ]; then echo "Type        : regular file"
    elif [ -d "$f" ]; then echo "Type        : directory"
    else                   echo "Type        : other (special) file"
    fi
    echo "Size        : $(stat -c %s "$f") bytes"
    echo "Permissions : $(stat -c '%A (%a)' "$f")"
    echo "Owner:Group : $(stat -c '%U:%G' "$f")"
    echo "Last modified: $(stat -c %y "$f")"
    echo "Inode       : $(stat -c %i "$f")"
    echo "Hard links  : $(stat -c %h "$f")"
    [ -r "$f" ] && echo "Readable    : yes" || echo "Readable    : no"
    [ -w "$f" ] && echo "Writable    : yes" || echo "Writable    : no"
    [ -x "$f" ] && echo "Executable  : yes" || echo "Executable  : no"
    if [ -f "$f" ]; then
        echo "Lines/Words/Chars: $(wc -lwc < "$f")"
    fi
    echo "--- ls -l ---"; ls -l "$f"
else
    echo "File '$f' does NOT exist."
    exit 1
fi
