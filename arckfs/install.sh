#!/bin/bash

sudo -v

sudo rmmod sufs

sudo insmod kfs/sufs.ko pm_nr=1 
# sufs_kfs_dele_thrds=12

sudo chmod 666 /dev/supremefs 

sudo ./fsutils/init