#!/bin/bash
#NUM=10
NUM=101

# echo "Creating files"
taskset -c 1-1 ./create-file-tg /mnt/pmem0 $NUM 


