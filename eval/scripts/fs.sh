fs=(pmfs nova winefs odinfs)

for i in ${fs[@]}
do
    lsmod | grep $i
    ret=$?

    if [ $ret -ne 0 ]
    then 
        echo "$i *not* installed"
        echo "Please run fs/compile.sh"
        echo "----------------------------------------------------------------"
        exit 1
    fi
done 