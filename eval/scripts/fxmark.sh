#!/bin/bash

source common.sh
source test.sh
source fs.sh

rm -r ../data/fxmark
rm -r ../benchmark/fxmark/logs/fxmark
(cd ../../arckfs && ./compile.sh)

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^ext4$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="ext4-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^nova$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="nova-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^winefs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="winefs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pmem-local' \
    --fs='^pmfs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="pmfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-array' --fs='^odinfs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread="$DTHREADS" --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='True' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="odinfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='^pmem-local' \
    --fs='^splitfs$' \
    --workload='^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="0" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="splitfs-meta.log" --duration=10

$FXMARK_BIN_PATH/run-fxmark.py --media='pm-char-array' --fs='^sufs$' \
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
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
    --workload='^DWTL$|^MRPL$|^MRPM$|^MRPH$|^MRDL$|^MRDM$|^MWCL$|^MWCM$|^MWUL$|^MWUM$|^MWRL$|^MWRM$' \
    --ncore="^1$|^2$|^4$|^6$|^8$|^10$|^12$|^16$|^20$|^24$|^28$|^32$|^36$|^40$|^44$|^48$" --iotype='bufferedio' --dthread='0' --dsocket="$MAX_SOCKETS" \
    --rcore='False' --delegate='False' --confirm='True' \
    --directory_name="$FM_LOG_DIR" --log_name="sufs-meta.log" --duration=10


echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/fxmark/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR/plus"
done
echo ""
