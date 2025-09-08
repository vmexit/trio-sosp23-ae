#!/bin/bash

taskset -c 1-1 ./mkdir /sufs/1/
echo "mkdir #1 done"

taskset -c 1-1 ./mkdir /sufs/1/2/
echo "mkdir #2 done"
taskset -c 1-1 ./touch /sufs/1/2/file.dat
echo "touch #1 done"

taskset -c 1-1 ./mkdir /sufs/3/
echo "mkdir #3 done"

taskset -c 1-1 ./mv /sufs/1/2/ /sufs/3/2/
echo "mv done"

#gdb --args "./mv" /sufs/1/2/ /sufs/3/2/
