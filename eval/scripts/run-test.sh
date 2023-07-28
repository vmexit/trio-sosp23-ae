#!/bin/bash

sudo -v

./fio.sh 1
./fxmark.sh 1
./filebench.sh 1 
./filebench-sp.sh 1

./parse.sh 1