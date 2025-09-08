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




