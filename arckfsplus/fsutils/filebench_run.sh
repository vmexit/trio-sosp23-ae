#!/bin/bash

RESULT_FILE="throughput_results.txt"
LOG_FILE="benchmark.log"
RUNS=10  # 전체 반복 횟수

# 시작할 때 한 번만 초기화 (같은 스크립트 실행 내에서 누적)
: > "$RESULT_FILE"
: > "$LOG_FILE"

run_workload() {
    local workload=$1
    local threads=$2
    local retries=10

    for ((i=1; i<=retries; i++)); do
        {
            echo "=== Running $workload with $threads threads (attempt $i) ==="
            cd ..
            sudo ./install.sh
            cd fsutils
            sudo taskset -a -c 0-23 ./$workload "$threads"
        } >> "$LOG_FILE" 2>&1

        last_line=$(grep -v '^[[:space:]]*$' "$LOG_FILE" | tail -n 1)
        result=$(awk '{print $NF}' <<< "$last_line" | tr -d '\r\n')

        if [[ $result =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
            echo "$workload,$threads,$result" >> "$RESULT_FILE"
            echo "[OK] $workload $threads threads -> $result"
            return 0
        else
            echo "[WARN] Parsing failed for $workload $threads threads (attempt $i)" | tee -a "$LOG_FILE"
        fi
    done

    echo "[FAIL] $workload $threads threads after $retries attempts" | tee -a "$LOG_FILE"
    return 1
}

for ((run=1; run<=RUNS; run++)); do
    echo "===== BEGIN RUN $run/$RUNS $(date -Is) =====" | tee -a "$LOG_FILE"

    for workload in webproxy varmail; do
        for threads in 1 16; do
            run_workload "$workload" "$threads"
        done
    done

    echo "===== END RUN $run/$RUNS $(date -Is) =====" | tee -a "$LOG_FILE"
done

echo "Results are saved in $RESULT_FILE"
