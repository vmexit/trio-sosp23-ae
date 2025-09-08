#!/bin/bash

sudo dmesg -C

# Unload the module if already loaded
sudo rmmod sufs.ko || true
# Insert the module
sudo insmod /usr/lib/modules/$(uname -r)/sufs.ko pm_nr=2 || true

# Set permissions for the device
sudo chmod 666 /dev/supremefs

# Initialize the filesystem
sudo init-sufs

if [ "$1" -eq 1 ]; then 
    sudo rm -rf /tmp/checker_ready
    checker-sufs &

    # Spin until /tmp/checker_ready contains "ready"
    while true; do
        if [ -f /tmp/checker_ready ] && grep -q 'ready' /tmp/checker_ready; then
                break
        fi
        sleep 0.1
    done
    echo "checker is up"
fi

taskset -c 0-0 ./touch /sufs/1.dat
taskset -c 0-0 ./touch /sufs/2.dat
taskset -c 0-0 ./touch /sufs/3.dat
taskset -c 0-0 ./touch /sufs/4.dat
taskset -c 0-0 ./touch /sufs/5.dat

if [ "$1" -eq 1 ]; then 
    sudo kill $(pgrep -f checker-sufs)
    sleep 1
    while pgrep -f checker-sufs > /dev/null; do
	    sudo kill $(pgrep -f checker-sufs)
	    sleep 1
    done
fi


# gdb --args "./mv" /sufs/1/2/ /sufs/3/2/
