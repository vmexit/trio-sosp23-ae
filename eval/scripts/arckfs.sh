#!/bin/bash

source common.sh
source test.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-4K$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FIO_LOG_DIR" --log_name="sufs-read-4k.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-2M$' \
    --ncore="$core" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FIO_LOG_DIR" --log_name="sufs-read-2m.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="$core" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FIO_LOG_DIR" --log_name="sufs-write.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs-meta.log" --duration=10

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

echo "Parsing fio results"
for i in `ls $FXMARK_LOG_PATH/fio/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FIO_LOG_DIR/$i" \
    --type='fio' --out="$DATA_PATH/$FIO_DATA_DIR"
done
echo ""

echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/fxmark/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR"
done
echo ""

echo "Parsing filebench results"
for i in `ls $FXMARK_LOG_PATH/filebench/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_DATA_DIR"
done
echo ""
