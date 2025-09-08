!/bin/bash

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