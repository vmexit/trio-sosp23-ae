call "common.gnuplot" "3.3in, 2.8in"

set terminal postscript color
set output "fig6.eps"

mp_startx=0.08
mp_starty=0.10
mp_height=0.80
mp_rowgap=0.13
mp_colgap=0.1
mp_width=0.90

eval mpSetup(2,2)

#4k-read
eval mpNext
unset xlabel
unset key
set ylabel 'Throughput (GiB/s)'
set title '(a) 4K read'

plot \
"../data/fio/pmem-local:ext4:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024) title '\ext' with lp ls ext, \
"../data/fio/pmem-local:pmfs:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\pmfs' with lp ls pmfs, \
"../data/fio/pmem-local:nova:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\nova' with lp ls nova, \
"../data/fio/pmem-local:winefs:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\winefs' with lp ls winefs, \
"../data/fio/dm-stripe:ext4:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\extr' with lp ls extr, \
"../data/fio/pm-array:odinfs:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\odinfs' with lp ls odinfs, \
"../data/fio/pm-char-array:sufs:seq-read-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\sufs' with lp ls sufs 

# 4k-write
eval mpNext
unset xlabel
unset ylabel
unset key

set title '(b) 4K write'

plot \
"../data/fio/pmem-local:ext4:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024) title '\ext' with lp ls ext, \
"../data/fio/pmem-local:pmfs:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\pmfs' with lp ls pmfs, \
"../data/fio/pmem-local:nova:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\nova' with lp ls nova, \
"../data/fio/pmem-local:winefs:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\winefs' with lp ls winefs, \
"../data/fio/dm-stripe:ext4:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\extr' with lp ls extr, \
"../data/fio/pm-array:odinfs:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\sys' with lp ls odinfs, \
"../data/fio/pm-char-array:sufs:seq-write-4K:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\sufs' with lp ls sufs 


# 2m-read
eval mpNext
unset key
set ylabel 'Throughput (GiB/s)'
set xlabel '\# threads'
set title '(c) 2M read'

plot \
"../data/fio/pmem-local:ext4:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\ext' with lp ls ext, \
"../data/fio/pmem-local:pmfs:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\pmfs' with lp ls pmfs, \
"../data/fio/pmem-local:nova:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\nova' with lp ls nova, \
"../data/fio/pmem-local:winefs:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\winefs' with lp ls winefs, \
"../data/fio/dm-stripe:ext4:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\extr' with lp ls extr, \
"../data/fio/pm-array:odinfs:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\sys' with lp ls odinfs, \
"../data/fio/pm-char-array:sufs:seq-read-2M:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\sufs' with lp ls sufs 

# 2m-write
eval mpNext
unset ylabel
set xlabel '\# threads'
set title '(d) 2M write'


plot \
"../data/fio/pmem-local:ext4:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\ext' with lp ls ext, \
"../data/fio/pmem-local:pmfs:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\pmfs' with lp ls pmfs, \
"../data/fio/pmem-local:nova:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\nova' with lp ls nova, \
"../data/fio/pmem-local:winefs:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\winefs' with lp ls winefs, \
"../data/fio/dm-stripe:ext4:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\extr' with lp ls extr, \
"../data/fio/pm-array:odinfs:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024) title '\sys' with lp ls odinfs, \
"../data/fio/pm-char-array:sufs:seq-write-2M:bufferedio.dat" \
 using 1:($2/1024/1024)  title '\sufs' with lp ls sufs 


unset multiplot

