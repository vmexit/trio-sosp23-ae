#!/bin/bash
sudo kill $(pgrep -f checker-sufs)
sleep 1
while pgrep -f checker-sufs > /dev/null; do
	sudo kill $(pgrep -f checker-sufs)
	sleep 1
done


