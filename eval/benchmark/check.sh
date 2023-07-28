#!/bin/bash

binary=(fio fxmark/bin/create-file fxmark/bin/fxmark filebench filebench-fd filebench-kvs-sufs)

sufs_binary=(fio-sufs create-file-sufs fxmark/bin/fxmark-sufs filebench-sufs filebench-fd-sufs filebench-fd-sufs-fd filebench-kvs-sufs)

for i in ${binary[@]}
do
    which $i
    ret=$?

    if [ $ret -ne 0 ]
    then 
        echo "$i *not* installed"
        echo "----------------------------------------------------------------"
        exit 1
    fi
done 

for i in ${sufs_binary[@]}
do
    echo "================================"
    $i 
    ret=$?

    if [ $ret -eq 139 ]
    then
        echo "$i not able to run. Please use the prebuilt one."
        if [ $i != "fxmark/bin/fxmark-sufs" ]
        then 
            echo "sudo cp -a prebuilt/$i /usr/local/bin/"  
        else
            echo "sudo cp -a prebuilt/$i fxmark/bin/"
        fi 
        echo "================================" 
        exit 1
    else
        echo "$i segfaults as expected"
        echo "================================"
        echo ""
        echo ""
    fi 
done 


echo "All passed!"
