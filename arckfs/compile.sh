#!/bin/bash

sudo -v

subdirs=(checker libfs kfs libfsfd libfskv fsutils)

for i in ${subdirs[@]}
do 
    cd $i
    make clean && make -j && make install
    ret=$?
    cd -

    if [ $ret -eq 0 ]
    then
        echo "$i installed successfully!"
    else 
        echo "$i not installed!"
        exit 1
    fi 
done  

echo "ArckFS installed successfully!"


