#!/bin/bash

sudo ndctl create-namespace -f -e namespace0.0 --mode="devdax"

mkdir -p ../../eval/data/sharing-cost

cd ../../arckfsplus/

./compile.sh

cd fsutils

# 4KB-write-2MB
echo "== 4KB-write-2MB ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log

for i in {1..1}; do
    echo "Iteration $i"

    cd ..
    ./install.sh
    cd fsutils
    ./run-share-regular-tg.sh 2097152 | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log

    echo "Iteration $i done"
    echo "-----------------"
done

echo "== 4KB-write-1GB ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log
# 4KB-write-1GB
for i in {1..1}; do
    echo "Iteration $i"

    cd ..
    ./install.sh
    cd fsutils
    ./run-share-regular-tg.sh 1073741824 | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log

    echo "Iteration $i done"
    echo "-----------------"
done

# create 10, create 100
echo "== create ==" | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log
for i in {1..5}; do
    echo "Iteration $i"

    cd ..
    ./install.sh
    cd fsutils
    ./run-share-dir-tg.sh | tee -a ../../eval/data/sharing-cost/arckfs-plus-tg.log

    echo "Iteration $i done"
    echo "-----------------"
done

sudo rmmod sufs