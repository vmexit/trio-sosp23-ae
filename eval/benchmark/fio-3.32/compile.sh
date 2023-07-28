#!/bin/bash

sudo -v

make
sudo make install

echo "----------------------------------------------------------------"
echo "Checking"

which fio fio-sufs
ret=$?

if [ $ret -eq 0 ]
then
    echo "Fio installed successfully!"
    echo "----------------------------------------------------------------"
    exit 0

else
    echo "Fio *not* installed"
    echo "----------------------------------------------------------------"
    exit 1
fi

