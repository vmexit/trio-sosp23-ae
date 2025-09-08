#!/bin/bash
#SIZE=2097152
SIZE=1073741824



echo "Creating files"
taskset -c 1-1 ./create /sufs/share.txt $SIZE 

echo "Writing to files: P1"
taskset -c 1-1 ./write-file /sufs/share.txt "b" 5000000 $SIZE 0 &

echo "Writing to files: P2"
taskset -c 2-2 ./write-file /sufs/share.txt "c" 5000000 $SIZE 4096 &

wait


