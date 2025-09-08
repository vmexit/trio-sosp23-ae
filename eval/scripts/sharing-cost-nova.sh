#!/bin/bash

# sudo ndctl create-namespace -f -e namespace0.0 --mode="fsdax"

mkdir -p ../../eval/data/sharing-cost
mkdir -p /mnt/pmem0

cd util
make

# 4KB-write-2MB
echo "== 4KB-write-2MB ==" | tee -a ../../../eval/data/sharing-cost/nova.log

for i in {1..1}; do
    echo "Iteration $i"

    sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem0
    ./run-share-regular-nova.sh 2097152 | tee -a ../../../eval/data/sharing-cost/nova.log
    sudo umount /mnt/pmem0

    echo "Iteration $i done"
    echo "-----------------"
done

echo "== 4KB-write-1GB ==" | tee -a ../../../eval/data/sharing-cost/nova.log
# 4KB-write-1GB
for i in {1..1}; do
    echo "Iteration $i"

    sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem0
    ./run-share-regular-nova.sh 1073741824 | tee -a ../../../eval/data/sharing-cost/nova.log
    sudo umount /mnt/pmem0

    echo "Iteration $i done"
    echo "-----------------"
done

# create 10, create 100
echo "== create ==" | tee -a ../../../eval/data/sharing-cost/nova.log
for i in {1..5}; do
    echo "Iteration $i"

    sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem0
    ./run-share-dir-tg.sh | tee -a ../../../eval/data/sharing-cost/nova.log
    sudo umount /mnt/pmem0

    echo "Iteration $i done"
    echo "-----------------"
done