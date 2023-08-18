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
    --ncore="$core" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FIO_LOG_DIR" --log_name="sufs-read-2m.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="$core" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FIO_LOG_DIR" --log_name="sufs-write.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="$core" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_fileserver$|^filebench_webserver$' \
    --ncore="$core" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='True' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="sufs-data.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^filebench_varmail$|^filebench_webproxy$' \
    --ncore="$score" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FB_LOG_DIR" --log_name="sufs-meta.log" --duration=10

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

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-4K$' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-read-4k.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-read-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-read-2m.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^fio_global_seq-write-4K$|^fio_global_seq-write-2M$' \
    --ncore="^1$" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$SG_LOG_DIR" --log_name="sufs-write.log" --duration=30

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='dbench_*' \
    --ncore="^1$" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$DB_LOG_DIR" --log_name="sufs.log" --duration=10

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

echo "Parsing filebench-sp results"
for i in `ls $FXMARK_LOG_PATH/$FB_SP_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_SP_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_SP_DATA_DIR"
done
echo ""

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

echo "Parsing dbench results"
for i in `ls $FXMARK_LOG_PATH/$DB_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$DB_LOG_DIR/$i" \
    --type='dbench' --out="$DATA_PATH/$DB_DATA_DIR"
done

./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"
