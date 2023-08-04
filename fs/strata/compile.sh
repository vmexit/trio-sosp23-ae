#!/bin/bash
FXMARK_BIN_PATH="../../eval/benchmark/fxmark/bin/"

sudo -v 
dirs=(libfs/lib libfs libfs/tests kernfs kernfs/tests)

sudo apt install libnl-3-dev libnl-route-3-dev librdmacm-dev

for i in ${dirs[@]}
do
    cd $i
    make clean 
    make -j
    cd -
done

rm -rf $FXMARK_BIN_PATH/nvml_lib/
cp -a libfs/lib/nvml/src/nondebug/ $FXMARK_BIN_PATH/nvml_lib/
ret=$?

if [ $ret -ne 0 ]
then
    echo "NVML build failed"
    exit 1
fi 


rm -rf $FXMARK_BIN_PATH/strata_build/
cp -a kernfs/build $FXMARK_BIN_PATH/strata_build/
ret=$?

if [ $ret -ne 0 ]
then
    echo "Kernfs build failed"
    exit 1
fi 

cp -a libfs/lib/jemalloc-4.5.0/lib/libjemalloc.so.2 $FXMARK_BIN_PATH/
ret=$?

if [ $ret -ne 0 ]
then
    echo "Jemalloc build failed"
    exit 1
fi 


cp -a kernfs/tests/kernfs $FXMARK_BIN_PATH/strata_kfs
ret=$?

if [ $ret -ne 0 ]
then
    echo "KernFS build failed"
    exit 1
fi 

cp -a libfs/build/libmlfs.so $FXMARK_BIN_PATH/
ret=$?

if [ $ret -ne 0 ]
then
    echo "LibFS build failed"
    exit 1
fi 


sudo cp -a libfs/bin/mkfs.mlfs /usr/local/bin/
ret=$?

if [ $ret -ne 0 ]
then
    echo "LibFS build failed"
    exit 1
fi

echo "Install strata succeed!"
exit 0
