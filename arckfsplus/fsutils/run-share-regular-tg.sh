#!/bin/bash
# SIZE=8192
# SIZE=2097152
# SIZE=1073741824
SIZE=${1:-1073741824}

echo "Creating files"
taskset -c 1-1 ./create /sufs/share.txt $SIZE 

start_time=$(date +%s%N)

echo "Writing to files: P1 T1 T2"
taskset -c 1-23 ./write-file-tg /sufs/share.txt "a" 5000000 $SIZE 0 &

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

