#!/bin/bash
# Problem 1: Basic file and directory operations using Linux command-line utilities
# Usage: bash solution.sh      (OS_Lab is created inside the current directory)

echo "===== i. Current working directory and listing (including hidden files) ====="
pwd
ls -la

echo
echo "===== ii. Create OS_Lab with three subdirectories ====="
mkdir -p OS_Lab/dir1 OS_Lab/dir2 OS_Lab/dir3
ls -l OS_Lab

echo
echo "===== iii. Create text files, write content, display contents ====="
echo "Hello, this is file one." > OS_Lab/dir1/file1.txt          # '>' overwrites / creates
cat > OS_Lab/dir1/file2.txt <<EOF
Operating Systems Lab
Linux command line practice
EOF
echo "A second line appended to file one." >> OS_Lab/dir1/file1.txt   # '>>' appends
touch OS_Lab/dir1/empty.txt                                       # empty file
echo "--- file1.txt ---"; cat OS_Lab/dir1/file1.txt
echo "--- file2.txt ---"; cat OS_Lab/dir1/file2.txt
echo "--- first line of file2.txt (head) ---"; head -n 1 OS_Lab/dir1/file2.txt
echo "--- line numbers (cat -n) ---"; cat -n OS_Lab/dir1/file2.txt

echo
echo "===== iv. Copy, rename, move and delete files ====="
cp OS_Lab/dir1/file1.txt OS_Lab/dir2/file1_copy.txt && echo "Copied file1.txt -> dir2/file1_copy.txt"
mv OS_Lab/dir2/file1_copy.txt OS_Lab/dir2/renamed.txt && echo "Renamed file1_copy.txt -> renamed.txt"
mv OS_Lab/dir1/file2.txt OS_Lab/dir3/ && echo "Moved file2.txt -> dir3/"
rm -v OS_Lab/dir1/empty.txt
echo "--- Tree after operations ---"
find OS_Lab | sort

echo
echo "===== v. Search for a file and display its size and information ====="
echo "Searching for file2.txt ..."
found=$(find OS_Lab -type f -name "file2.txt")
echo "Found: $found"
if [ -n "$found" ]; then
    echo "--- ls -lh ---";   ls -lh "$found"
    echo "--- size in bytes (wc -c) ---"; wc -c < "$found"
    echo "--- stat ---";     stat "$found"
    echo "--- file type ---"; file "$found" 2>/dev/null || echo "(file command not available)"
fi
echo "--- Disk usage of OS_Lab ---"
du -sh OS_Lab
