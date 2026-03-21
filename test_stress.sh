#!/bin/bash
make > /dev/null 2>&1

function check_result {
    if [ $? -ne 0 ]; then
        echo "FAIL: $1"
        exit 1
    else
        echo "PASS: $1"
    fi
}

echo "=== Stress Test Suite ==="

# 1. Empty Files
echo "--- Test: Empty Files ---"
touch empty1.dat
touch empty2.dat
./patcher diff empty1.dat empty2.dat patch_empty.dat
check_result "Diff empty files"
./patcher patch empty1.dat patch_empty.dat res_empty.dat
check_result "Patch empty files"
./patcher verify empty2.dat res_empty.dat
check_result "Verify empty files"

# 2. Empty Original to Non-Empty
echo "--- Test: Empty -> Content ---"
echo "New Content" > not_empty.dat
./patcher diff empty1.dat not_empty.dat patch_e2n.dat
./patcher patch empty1.dat patch_e2n.dat res_e2n.dat
./patcher verify not_empty.dat res_e2n.dat
check_result "Patch apply empty->content"

# 3. Non-Empty to Empty
echo "--- Test: Content -> Empty ---"
./patcher diff not_empty.dat empty1.dat patch_n2e.dat
./patcher patch not_empty.dat patch_n2e.dat res_n2e.dat
./patcher verify empty1.dat res_n2e.dat
check_result "Patch apply content->empty"

# 4. Binary Data
echo "--- Test: Binary Data ---"
# Create 1KB binary file
head -c 1024 /dev/urandom > bin_orig.dat
cp bin_orig.dat bin_mod.dat
# Change bytes at offset 500
printf "\xDE\xAD\xBE\xEF" | dd of=bin_mod.dat bs=1 seek=500 count=4 conv=notrunc 2>/dev/null
./patcher diff bin_orig.dat bin_mod.dat patch_bin.dat
./patcher patch bin_orig.dat patch_bin.dat res_bin.dat
./patcher verify bin_mod.dat res_bin.dat
check_result "Binary file patching"

# 5. Large File (Performance)
echo "--- Test: Large File (1MB) ---"
# 1MB file
head -c 1048576 /dev/urandom > large_orig.dat
cp large_orig.dat large_mod.dat
# Change byte at 500000
printf "X" | dd of=large_mod.dat bs=1 seek=500000 count=1 conv=notrunc 2>/dev/null

start_time=$(date +%s)
./patcher diff large_orig.dat large_mod.dat patch_large.dat
end_time=$(date +%s)
echo "Diff Time: $((end_time - start_time)) seconds"

./patcher patch large_orig.dat patch_large.dat res_large.dat
./patcher verify large_mod.dat res_large.dat
check_result "Large file (1MB) verification"

# 6. Many Changes
echo "--- Test: Many Small Changes ---"
cp large_orig.dat many_mod.dat
# Introduce 100 changes
for i in {1..100}; do
    offset=$((i * 1000))
    printf "C" | dd of=many_mod.dat bs=1 seek=$offset count=1 conv=notrunc 2>/dev/null
done

./patcher diff large_orig.dat many_mod.dat patch_many.dat
./patcher patch large_orig.dat patch_many.dat res_many.dat
./patcher verify many_mod.dat res_many.dat
check_result "Many changes verification"

# 7. Newline Special Handling
echo "--- Test: Newline Handling ---"
echo -e "Line1\nLine2" > nl_orig.txt
echo -e "Line1\nLine2\n" > nl_mod.txt # Add trailing newline
./patcher diff nl_orig.txt nl_mod.txt patch_nl.dat
# Check if \n is escaped in patch file
grep "\\\\n" patch_nl.dat > /dev/null
if [ $? -eq 0 ]; then
    echo "PASS: Newline detected and escaped"
else
    echo "FAIL: Newline not escaped properly in patch"
    cat patch_nl.dat
fi
./patcher patch nl_orig.txt patch_nl.dat res_nl.txt
./patcher verify nl_mod.txt res_nl.txt
check_result "Newline addition verification"

echo "=== All Stress Tests Completed ==="
rm -f empty*.dat not_empty.dat bin*.dat large*.dat many*.dat patch*.dat res*.dat nl*.txt
