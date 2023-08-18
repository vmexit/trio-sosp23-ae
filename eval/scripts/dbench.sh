#!/bin/bash

source common.sh
source fs.sh

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^ext4$|^nova$|^winefs$' \
    --workload='dbench_*' \
    --ncore="^1$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$DB_LOG_DIR" --log_name="dbench.log" --duration=10


$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='dbench_*' \
    --ncore="^1$" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$DB_LOG_DIR" --log_name="sufs.log" --duration=10


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

