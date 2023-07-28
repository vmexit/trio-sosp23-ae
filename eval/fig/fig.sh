#!/bin/bash

rm -rf *.eps

for i in `ls *.gp`
do
    gnuplot $i
done




