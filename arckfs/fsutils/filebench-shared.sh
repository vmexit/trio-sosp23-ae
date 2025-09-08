#!/bin/bash

for threads in 1 2 4 8 12 16 20 24
do  
    cd ..
    sudo ./install.sh
    cd fsutils
    sudo taskset -a -c 0-23 ./webproxy "$threads"
done

for threads in 1 2 4 8 12 16 20 24
do  
    cd ..
    sudo ./install.sh
    cd fsutils
    sudo taskset -a -c 0-23 ./varmail "$threads"
done