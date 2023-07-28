#!/bin/bash
  
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
sudo cp -a db_bench /usr/local/bin

echo "----------------------------------------------------------------"
echo "Checking"

which db_bench 
ret=$?

if [ $ret -eq 0 ]
then
    echo "leveldb installed successfully!"
    echo "----------------------------------------------------------------"
    exit 0

else
    echo "leveldb *not* installed"
    echo "----------------------------------------------------------------"
    exit 1
fi
