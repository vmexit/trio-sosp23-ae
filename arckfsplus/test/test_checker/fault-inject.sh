#!/bin/bash
for i in {0..16}; do
    echo "run fault injection i=$i"
    ./test-checker.sh "$i"
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "test-checker.sh failed with exit code $rc for i=$i"
    fi
done

for i in {0..15}; do
    for ((j=i+1; j<=16; j++)); do
        # these combination crashses libFS
        if [[ $i -eq 14 && ( $j -eq 15 || $j -eq 16 ) ]]; then
            continue
        fi
        echo "run fault injection i=$i j=$j"
        ./test-checker.sh "$i" "$j"
        rc=$?
        if [ $rc -ne 0 ]; then
            echo "run-checker.sh failed with exit code $rc for i=$i j=$j"
        fi
    done
done



