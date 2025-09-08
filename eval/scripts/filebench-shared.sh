#!/bin/bash

sudo ndctl create-namespace -f -e namespace0.0 --mode="devdax"

cd ../..

cd arckfs
./compile.sh
cd fsutils
./filebench_run.sh
mkdir -p ../../eval/data/filebench-shared
cp throughput_results.txt ../../eval/data/filebench-shared/arckfs.csv

cd ../..

cd arckfsplus
./compile.sh
cd fsutils
./filebench_run.sh
mkdir -p ../../eval/data/filebench-shared
cp throughput_results.txt ../../eval/data/filebench-shared/arckfs-plus.csv