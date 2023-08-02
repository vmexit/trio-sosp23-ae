#!/bin/bash

sudo -v

./fio.sh 
./fxmark.sh 
./filebench.sh 
./filebench-sp.sh 
./fig5.sh
./dbench.sh

./parse.sh