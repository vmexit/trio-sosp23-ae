#!/bin/bash

source common.sh
source test.sh
source fs.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^ext4$|^pmfs$|^nova$|^winefs$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket='0' \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="fs-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^ext4$|^pmfs$|^nova$|^winefs$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="fs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^dm-stripe$' --fs='^ext4$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="ext4-raid0-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='^dm-stripe$' --fs='^ext4$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="ext4-raid0-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='^odinfs$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="odinfs-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='^odinfs$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="odinfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="sufs-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="sufs-meta.log" --duration=10


echo "Parsing filebench results"
for i in `ls $FXMARK_LOG_PATH/$FB_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_DATA_DIR"
done
echo ""
