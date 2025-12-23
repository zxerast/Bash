#!/usr/bin/env bash

echo "===== PHASE 1: BASICS ====="
echo test
pwd
cd /
pwd
cd - || exit 1

echo
echo "===== PHASE 2: VARIABLES ====="
export A=123
export B=xyz
echo "$A $B"
echo "$A$B"
unset A
echo "${A:-fallback}"

echo
echo "===== PHASE 3: PIPES ====="
echo "5 4 3 2 1" | tr ' ' '\n' | sort | tr '\n' ' ' | wc -w

echo
echo "===== PHASE 4: REDIRECTS ====="
echo first > /tmp/test1.txt
echo second > /tmp/test1.txt
cat /tmp/test1.txt
rm /tmp/test1.txt

echo
echo "===== PHASE 5: REDIRECT + PIPE ====="
echo hello world | wc -w > /tmp/test2.txt
cat /tmp/test2.txt
rm /tmp/test2.txt

echo
echo "===== PHASE 6: BACKGROUND PIPE ====="
sleep 2 | echo pipe & 
jobs
wait

echo
echo "===== PHASE 7: MANY BACKGROUND JOBS ====="
sleep 5 &
sleep 4 &
sleep 3 &
sleep 2 &
sleep 1 &
jobs

echo
echo "===== PHASE 8: WAIT ALL ====="
wait
jobs

echo
echo "===== PHASE 9: STOP / CONT ====="
sleep 10 &
PID=$!
jobs
kill -STOP $PID
jobs
kill -CONT $PID
jobs
wait $PID

echo
echo "===== PHASE 10: FG / BG ====="
sleep 10 &
jobs
bg
jobs
fg

echo
echo "===== PHASE 11: SIGINT FOREGROUND ====="
sleep 5 &
PID=$!
sleep 1
kill -INT $PID
wait

echo
echo "===== PHASE 12: PIPE + SIGINT ====="
yes | head -n 100000 > /dev/null &
PID=$!
sleep 1
kill -INT $PID
wait

echo
echo "===== PHASE 13: EXEC REPLACE ====="
bash -c 'exec echo EXEC_OK'

echo
echo "===== PHASE 14: EXIT STATUS ====="
false
echo "status=$?"
true
echo "status=$?"

echo
echo "===== PHASE 15: STDIN REDIRECT ====="
echo input > /tmp/in.txt
cat < /tmp/in.txt
rm /tmp/in.txt

echo
echo "===== PHASE 16: COMMAND NOT FOUND ====="
nonexistent_command_123
echo "status=$?"

echo
echo "===== PHASE 17: LONG PIPELINE ====="
seq 1 100 | grep 50 | wc -l

echo
echo "===== PHASE 18: RACE WAIT ====="
sleep 1 &
sleep 1 &
sleep 1 &
wait

echo
echo "===== DONE ====="

