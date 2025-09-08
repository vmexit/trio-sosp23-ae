#!/bin/bash

sudo -v

./single-thread.sh
./fxmark.sh 
./filebench-shared.sh 
./sharing-cost.sh