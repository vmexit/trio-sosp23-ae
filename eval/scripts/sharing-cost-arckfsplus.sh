#!/bin/bash

sudo ndctl create-namespace -f -e namespace0.0 --mode="devdax"

mkdir -p ../../eval/data/sharing-cost

cd ../../arckfsplus/

./compile.sh

cd fsutils

# 4KB-write-2MB
echo "== 4KB-write-2MB ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus.log

for i in {1..1}; do
    echo "Iteration $i"

    ./run-checker.sh
    ./run-share-regular.sh 2097152 | tee -a ../../eval/data/sharing-cost/arckfs-plus.log
    ./cleanup-checker.sh 

    echo "Iteration $i done"
    echo "-----------------"
done

echo "== 4KB-write-1GB ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus.log
# 4KB-write-1GB
for i in {1..1}; do
    echo "Iteration $i"

    ./run-checker.sh
    ./run-share-regular.sh 1073741824 | tee -a ../../eval/data/sharing-cost/arckfs-plus.log
    ./cleanup-checker.sh 

    echo "Iteration $i done"
    echo "-----------------"
done

# create 10, create 100
echo "== create ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus.log
for i in {1..5}; do
    echo "Iteration $i"

    ./run-checker.sh
    ./run-share-dir.sh | tee -a ../../eval/data/sharing-cost/arckfs-plus.log
    ./cleanup-checker.sh 

    echo "Iteration $i done"
    echo "-----------------"
done