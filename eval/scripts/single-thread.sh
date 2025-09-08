#!/bin/bash

source common.sh
source fs.sh

(cd ../../arckfs && ./compile.sh)

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

echo "Parsing single-threaded metadata results"
for i in `ls $FXMARK_LOG_PATH/$SG_META_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_META_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$SG_META_DATA_DIR"
done

(cd ../../arckfsplus && ./compile.sh)

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^MRPL$|^MWCL$|^MWUL$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_META_LOG_DIR" --log_name="sufs-meta.log" --duration=10


echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/$SG_META_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_META_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$SG_META_LOG_DIR/plus"
done
echo ""


./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"

echo ""