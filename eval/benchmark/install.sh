#!/bin/bash

sudo -v 
bm=(fio-3.32 fxmark leveldb leveldb-sufs)

for i in ${bm[@]}
do
    cd $i
    ./compile.sh
    ret=$?

    if [ $ret -eq 1 ]
    then
        echo "$i *not* installed"
        exit 1
    fi

    cd -
done

echo "All succeed!"
exit 0