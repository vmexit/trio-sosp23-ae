#!/bin/bash
#NUM=10
NUM=100

echo "Creating files"
taskset -c 1-1 ./create-file /sufs/ $NUM 


