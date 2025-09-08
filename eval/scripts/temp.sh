#!/bin/bash

source common.sh
source test.sh
source fs.sh

rm -r ../data/fxmark
rm -r ../benchmark/fxmark/logs/fxmark

(cd ../../arckfs && ./compile.sh)

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^MRDL$' \
    --ncore="^48$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs-meta.log" --duration=10

echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/fxmark/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR"
done
echo ""


(cd ../../arckfsplus && ./compile.sh)

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^MRDL$' \
    --ncore="^48$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="fxmark" --log_name="sufs-meta.log" --duration=10


echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/fxmark/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR/plus"
done
echo ""

