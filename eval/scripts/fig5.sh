#!/bin/bash

source common.sh
source fs.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^nova$' \
    --workload='^fio_global_seq-read-4K$|^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="fs-read.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^nova$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="fs-write.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^pm-char$' --fs='^strata$' \
    --workload='^fio_global_seq-read-4K$|^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="strata-read.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^pm-char$' --fs='^strata$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="strata-write.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='odinfs' \
    --workload='^fio_global_seq-read-4K$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="odinfs-read-4k.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='odinfs' \
    --workload='^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="odinfs-read-2m.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='odinfs' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="odinfs-write.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-4K$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-read-4k.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-read-2m.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-write.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^nova$' \
    --workload='^MRPL$|^MWCL$|^MWUL$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_META_LOG_DIR" --log_name="nova-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^pm-char$' \
    --fs='^strata$' \
    --workload='^MRPL$|^MWCL$|^MWUL$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_META_LOG_DIR" --log_name="strata-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^MRPL$|^MWCL$|^MWUL$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_META_LOG_DIR" --log_name="sufs-meta.log" --duration=10

echo "Parsing single-threaded fio results"
for i in `ls $FXMARK_LOG_PATH/$SG_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_LOG_DIR/$i" \
    --type='fio' --out="$DATA_PATH/$SG_DATA_DIR"
done

echo "Parsing single-threaded metadata results"
for i in `ls $FXMARK_LOG_PATH/$SG_META_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_META_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$SG_META_DATA_DIR"
done

./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"

echo ""