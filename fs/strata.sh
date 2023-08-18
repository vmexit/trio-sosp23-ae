#!/bin/bash

cd strata
./compile.sh
ret=$?

if [ $ret -eq 0 ]
then 
    echo "Strata installed successfully!"
else
    echo "Strata *not* installed"
    stat=1
fi
