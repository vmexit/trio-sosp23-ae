#!/bin/bash
set -e

# Unload the module if already loaded
sudo rmmod sufs.ko || true
# Insert the module
sudo insmod /usr/lib/modules/$(uname -r)/sufs.ko pm_nr=2 || true

# Set permissions for the device
sudo chmod 666 /dev/supremefs

# Initialize the filesystem
sudo init-sufs

export sufs_alloc_cpu=48
export sufs_alloc_numa=-1
export sufs_init_alloc_size=65536
export sufs_alloc_pin_cpu=1


#Modify the ROPATH to your own path!
ROPATH="/home/trio/trio-sosp23-ae/"

ARGS="--type MRPL --ncore 48 --nbg 0 --duration 10 --directio 0 --root /sufs/ \
      --delegation_threads 0 --delegation_sockets 2 --delegate 0 --rcore 0"

if [ "$1" -eq 1 ]; then
    gdb --args "$ROPATH/eval/benchmark/fxmark/bin/fxmark-sufs" $ARGS
elif [ "$1" -eq 2 ]; then
    perf record -e cycles,instructions -g -o perf.data "$ROPATH/eval/benchmark/fxmark/bin/fxmark-sufs" $ARGS
else
    "$ROPATH/eval/benchmark/fxmark/bin/fxmark-sufs" $ARGS
fi

