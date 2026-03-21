#!/bin/bash
make
if [ $? -ne 0 ]; then
    echo "Make failed"
    exit 1
fi

echo "=== Case 1: Simple modification ==="
echo -n "Hello World!" > orig.txt
echo -n "Hello WORLD!" > mod.txt
./patcher diff orig.txt mod.txt patch1.dat
./patcher patch orig.txt patch1.dat res1.txt
./patcher verify mod.txt res1.txt
echo "Patch Content:"
cat patch1.dat
echo ""

echo "=== Case 2: Multiple changes ==="
echo -n "The quick brown fox jumps over the lazy dog." > orig2.txt
echo >> orig2.txt
echo -n "Now is the time for all good men to come to the aid." >> orig2.txt

echo -n "The QUICK brown fox jumps over the LAZY dog." > mod2.txt
echo >> mod2.txt
echo -n "Now is the TIME for all good men to come to the aid." >> mod2.txt

./patcher diff orig2.txt mod2.txt patch2.dat
./patcher patch orig2.txt patch2.dat res2.txt
./patcher verify mod2.txt res2.txt
echo "Patch Content:"
cat patch2.dat
echo ""

echo "=== Case 3: Append ==="
echo -n "Original content" > orig3.txt
echo -n "Original content" > mod3.txt
echo -e "\nAPPENDED DATA HERE!" >> mod3.txt

./patcher diff orig3.txt mod3.txt patch3.dat
./patcher patch orig3.txt patch3.dat res3.txt
./patcher verify mod3.txt res3.txt
echo "Patch Content:"
cat patch3.dat
echo ""

echo "=== Case 4: Truncate ==="
echo -n "This is a long file with lots of content here." > orig4.txt
echo -n "This is a short file" > mod4.txt

./patcher diff orig4.txt mod4.txt patch4.dat
./patcher patch orig4.txt patch4.dat res4.txt
./patcher verify mod4.txt res4.txt
echo "Patch Content:"
cat patch4.dat
echo ""

