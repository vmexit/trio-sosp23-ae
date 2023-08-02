#!/bin/bash

sudo -v

./fio.sh 1
./fxmark.sh 1
./filebench.sh 1 
./filebench-sp.sh
./fig5.sh
./dbench.sh

./parse.sh 1