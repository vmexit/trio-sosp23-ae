#!/bin/bash

source common.sh
source fs.sh
source test.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^splitfs$' \
    --workload='^fio_global_seq-read-4K$|^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="splitfs-read.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^splitfs$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="splitfs-write.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^pmem-local' \
    --fs='^splitfs$' \
    --workload='^MRPL$|^MWCL$|^MWUL$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_META_LOG_DIR" --log_name="splitfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^splitfs$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket='0' \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="splitfs-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^splitfs$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="splitfs-meta.log" --duration=10



echo "Parsing single-threaded fio results"
for i in `ls $FXMARK_LOG_PATH/$SG_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_LOG_DIR/$i" \
    --type='fio' --out="$DATA_PATH/$SG_DATA_DIR"
done
echo ""

echo "Parsing filebench results"
for i in `ls $FXMARK_LOG_PATH/$FB_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_DATA_DIR"
done
echo ""

./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"