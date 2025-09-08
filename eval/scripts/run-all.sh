#!/bin/bash

sudo -v

sudo ./single-thread.sh
sudo ./fxmark.sh 
sudo ./filebench-shared.sh 
sudo ./sharing-cost.sh