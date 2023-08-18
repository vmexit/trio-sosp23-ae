if [ "$#" -ne 1 ] && [ "$#" -ne 0 ] 
then
    echo "Usage: $0 [test_or_not]"
    exit 1
fi

# score: number of threads for varmail and webproxy. 
# core: number of threads for multithreaded other benchmarks
# Each thread operates on a dedicated 

if [ "$#" -eq 1 ] && [ "$1" -eq 1 ]
then
    core="^$MAX_CPUS$"
    score="^16$"
    test=1
else
    core="*"
    score="^1$|^2$|^4$|^8$|^16$"
    test=0
fi 
