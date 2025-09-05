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

sudo rm -rf /tmp/checker_ready


checker-sufs &

# Spin until /tmp/checker_ready contains "ready"
while true; do
	if [ -f /tmp/checker_ready ] && grep -q 'ready' /tmp/checker_ready; then
		break
	fi
	sleep 0.1
done

#ls-sufs /sufs/

./test_checker $1 $2
test_checker_exit=$?

sudo kill $(pgrep -f checker-sufs)
sleep 1
while pgrep -f checker-sufs > /dev/null; do
	sudo kill $(pgrep -f checker-sufs)
	sleep 1
done

exit $test_checker_exit



