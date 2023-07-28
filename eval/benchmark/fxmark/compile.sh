#!/bin/bash

make clean && make -j
ret=$?

echo "----------------------------------------------------------------"
echo "Checking"

if [ $ret -eq 0 ]
then
    echo "Fxmark installed successfully!"
    echo "----------------------------------------------------------------"
    exit 0

else
    echo "Fxmark *not* installed"
    echo "----------------------------------------------------------------"
    exit 1
fi

