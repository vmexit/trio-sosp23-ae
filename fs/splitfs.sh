#!/bin/bash

cd splitfs && make clean && make && make install

ret=$?

if [ "$ret" -eq 0 ]
then
    echo "Install SplitFS succeed!"
else 
    echo "Install SplitFS failed!"
fi 
