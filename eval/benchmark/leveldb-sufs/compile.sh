#!/bin/bash
  
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
sudo cp -a db_bench_sufs /usr/local/bin

echo "----------------------------------------------------------------"
echo "Checking"

which db_bench_sufs
ret=$?

if [ $ret -eq 0 ]
then
    echo "leveldb-sufs installed successfully!"
    echo "----------------------------------------------------------------"
    exit 0

else
    echo "leveldb-sufs *not* installed"
    echo "----------------------------------------------------------------"
    exit 1
fi
