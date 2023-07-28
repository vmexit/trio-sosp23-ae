#!/bin/bash

source common.sh


$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^DWTL$' \
    --ncore="^2$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs-meta.log" --duration=10

# $FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
#     --workload='filebench_*' \
#     --ncore='*' --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
#     --rcore='False' --delegate='True' --confirm='True' \
#     --directory_name="$FB_LOG_DIR" --log_name="sufs.log" --duration=30

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
