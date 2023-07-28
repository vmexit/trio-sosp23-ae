#!/bin/bash

source common.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^ext4$|^nova$|^winefs$' \
    --workload='db_bench_*' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="dbench.log" --duration=10


$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='db_bench_*' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs.log" --duration=10




# echo "Parsing dbench results"
# for i in `ls $FXMARK_LOG_PATH/dbench/`
# do
#     echo "On $i"
#     $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
#     --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR"
# done
# echo ""

