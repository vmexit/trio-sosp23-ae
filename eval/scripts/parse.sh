#!/bin/bash

source common.sh
source test.sh

echo "Parsing fio results"
for i in `ls $FXMARK_LOG_PATH/fio/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/fio/$i" \
    --type='fio' --out="$DATA_PATH/fio"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done
echo ""

echo "Parsing fxmark results"
for i in `ls $FXMARK_LOG_PATH/fxmark/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FM_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$FM_DATA_DIR"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done
echo ""

echo "Parsing filebench results"
for i in `ls $FXMARK_LOG_PATH/filebench/`
do
     echo "On $i"
     $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/filebench/$i" \
     --type='filebench' --out="$DATA_PATH/filebench"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done
echo ""

echo "Parsing filebench-sp results"
for i in `ls $FXMARK_LOG_PATH/$FB_SP_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$FB_SP_LOG_DIR/$i" \
    --type='filebench' --out="$DATA_PATH/$FB_SP_DATA_DIR"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi

done
echo ""

echo "Parsing single-threaded fio results"
for i in `ls $FXMARK_LOG_PATH/$SG_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_LOG_DIR/$i" \
    --type='fio' --out="$DATA_PATH/$SG_DATA_DIR"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done

echo "Parsing single-threaded metadata results"
for i in `ls $FXMARK_LOG_PATH/$SG_META_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$SG_META_LOG_DIR/$i" \
    --type='fxmark' --out="$DATA_PATH/$SG_META_DATA_DIR"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done

echo "Parsing dbench results"
for i in `ls $FXMARK_LOG_PATH/$DB_LOG_DIR/`
do
    echo "On $i"
    $FXMARK_PARSER_PATH/pdata.py --log="$FXMARK_LOG_PATH/$DB_LOG_DIR/$i" \
    --type='dbench' --out="$DATA_PATH/$DB_DATA_DIR"

    ret=$?
    if [ "$ret" -ne 0 ] && [ "$test" -eq 1 ]  
    then
        echo "Errors"
        exit 1
    fi
done

./pextra.py --fio_log="$DATA_PATH/$SG_DATA_DIR" \
    --fxmark_log="$DATA_PATH/$SG_META_DATA_DIR" \
    --filebench_log="$DATA_PATH/$FB_SP_DATA_DIR" \
    --dbench_log="$DATA_PATH/$DB_DATA_DIR" \
    --out="$DATA_PATH/$EXTRA_DATA_DIR" \
    --out_table="$FIG_PATH"


if [ $test -eq 1 ] 
then
    echo "----------------------------------------------------------"
    echo "Looks good! "
    echo "----------------------------------------------------------"
    exit 0
fi