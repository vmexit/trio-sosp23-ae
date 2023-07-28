#!/bin/bash

sudo -v

./fio.sh 
./fxmark.sh 
./filebench.sh 
./filebench-sp.sh 

./parse.sh