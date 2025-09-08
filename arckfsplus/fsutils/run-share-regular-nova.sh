#!/bin/bash
# SIZE=2097152
SIZE=${1:-1073741824}

#echo "Creating files"
taskset -c 1-1 ./create /mnt/pmem0/share.txt $SIZE 

start_time=$(date +%s%N)

#echo "Writing to files: P1"
taskset -c 1-1 ./write-file /mnt/pmem0/share.txt "b" 5000000 $SIZE 0 &

#echo "Writing to files: P2"
taskset -c 2-2 ./write-file /mnt/pmem0/share.txt "c" 5000000 $SIZE 4096 &

wait

end_time=$(date +%s%N)  # 끝 시간 (ns)

elapsed_ns=$((end_time - start_time))
elapsed_s=$(echo "scale=6; $elapsed_ns/1000000000" | bc)

# 총 쓰기 크기 (bytes)
total_bytes=$(( (5000000 + 5000000) * 4096 ))

# GiB/s 계산
gib_per_s=$(echo "scale=3; ($total_bytes / $elapsed_s) / 1073741824" | bc -l)

echo "Elapsed time: $elapsed_s seconds"
echo "Throughput: $gib_per_s GiB/s"


echo 