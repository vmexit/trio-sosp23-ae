#!/bin/bash

source common.sh
source fs.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^ext4$|^pmfs$|^nova$|^winefs$' \
    --workload='^filebench_varmail-fd$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="fs-varmail-fd.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^dm-stripe$' --fs='^ext4$' \
    --workload='^filebench_varmail-fd$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="ext4-raid0-meta-fd.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='^odinfs$' \
    --workload='^filebench_varmail-fd$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="odinfs-meta-fd.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_varmail-fd$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="sufs-meta-fd.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs-fd$' \
    --workload='^filebench_varmail-fd$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="sufs-fd-meta-fd.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' --fs='^ext4$|^pmfs$|^nova$|^winefs$' \
    --workload='^filebench_webproxy$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="fs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^dm-stripe$' --fs='^ext4$' \
    --workload='^filebench_webproxy$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="ext4-raid0-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='^odinfs$' \
    --workload='^filebench_webproxy$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='12' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="odinfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_webproxy$' \
    --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_SP_LOG_DIR" --log_name="sufs-meta.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs-kv$' \
   --workload='^filebench_webproxy$' \
   --ncore="^8$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
   --rcore='False' --delegate='True' --confirm='True' \
   --directory_name="$FB_SP_LOG_DIR" --log_name="sufs-kv-meta.log" --duration=10

echo "Parsing filebench-sp results"
for i in `ls $FXMARK_LOG_PATH/$FB_SP_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_SP_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_SP_DATA_DIR"
done
echo ""

./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"