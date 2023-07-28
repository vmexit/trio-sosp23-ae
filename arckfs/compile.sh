#!/bin/bash

sudo -v

cd kfs
make clean && make -j && make install
cd - 

cd libfs
make clean && make -j && make install
cd - 

cd libfsfd
make clean && make -j && make install
cd - 

cd libfskv
make clean && make -j && make install
cd - 

cd fsutils
make clean && make -j && make install
cd - 


