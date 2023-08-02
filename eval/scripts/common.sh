FXMARK_PATH="../benchmark/fxmark"
FXMARK_PARSER_PATH="../benchmark/fxmark/parser"

FXMARK_LOG_PATH="../benchmark/fxmark/logs"
FXMARK_BIN_PATH="../benchmark/fxmark/bin/"

MAX_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
MAX_SOCKETS=`cat /proc/cpuinfo  | grep "physical id" | sort -u | wc -l`

DATA_PATH=../data/
FIG_PATH=../fig/

FB_LOG_DIR=filebench
FB_DATA_DIR=filebench

FIO_LOG_DIR=fio
FIO_DATA_DIR=fio

FM_LOG_DIR=fxmark
FM_DATA_DIR=fxmark

DB_LOG_DIR=dbench
DB_DATA_DIR=dbench

SG_LOG_DIR=sg
SG_DATA_DIR=sg

SG_META_LOG_DIR=sg_meta
SG_META_DATA_DIR=sg_meta

FB_SP_LOG_DIR=filebench_sp
FB_SP_DATA_DIR=filebench_sp

EXTRA_DATA_DIR=extra

sudo -v 
