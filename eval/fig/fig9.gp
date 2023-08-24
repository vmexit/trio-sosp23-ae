call "common.gnuplot" "3.4in, 3.9in"

set terminal postscript color
set output "fig9.eps"

mp_startx=0.1
mp_starty=0.05
mp_height=0.9
mp_rowgap=0.1
mp_colgap=0.1
mp_width=0.80

eval mpSetup(2,3)

set ylabel 'KOps/sec'
eval mpNext

set title '(a) Fileserver'
plot \
"../data/filebench/pmem-local:ext4:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\ext' with lp ls ext, \
"../data/filebench/pmem-local:pmfs:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\pmfs' with lp ls pmfs, \
"../data/filebench/pmem-local:nova:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\nova' with lp ls nova, \
"../data/filebench/pmem-local:winefs:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\winefs' with lp ls winefs, \
 "../data/filebench/pmem-local:splitfs:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\splitfs' with lp ls splitfs, \
"../data/filebench/dm-stripe:ext4:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\extr' with lp ls extr, \
"../data/filebench/pm-array:odinfs:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\odinfs' with lp ls odinfs, \
"../data/filebench/pm-char-array:sufs:filebench_fileserver:bufferedio.dat" \
 using 1:($2/1000) title '\sufs' with lp ls sufs, \

eval mpNext
unset key
unse ylabel
set title '(b) Webserver'
plot \
"../data/filebench/pmem-local:ext4:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\ext' with lp ls ext, \
"../data/filebench/pmem-local:pmfs:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\pmfs' with lp ls pmfs, \
"../data/filebench/pmem-local:nova:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\nova' with lp ls nova, \
"../data/filebench/pmem-local:winefs:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\winefs' with lp ls winefs, \
 "../data/filebench/pmem-local:splitfs:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\splitfs' with lp ls splitfs, \
"../data/filebench/dm-stripe:ext4:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\extr' with lp ls extr, \
"../data/filebench/pm-array:odinfs:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\sys' with lp ls odinfs, \
"../data/filebench/pm-char-array:sufs:filebench_webserver:bufferedio.dat" \
 using 1:($2/1000) title '\sufs' with lp ls sufs, \

#set ylabel 'Throughput GiB/s'

eval mpNext
set ylabel 'KOps/sec'
set xlabel '\# threads'
set title '(c) Webproxy'
plot \
"../data/filebench/pmem-local:ext4:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\ext' with lp ls ext, \
"../data/filebench/pmem-local:pmfs:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\pmfs' with lp ls pmfs, \
"../data/filebench/pmem-local:nova:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\nova' with lp ls nova, \
"../data/filebench/pmem-local:winefs:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\winefs' with lp ls winefs, \
  "../data/filebench/pmem-local:splitfs:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\splitfs' with lp ls splitfs, \
"../data/filebench/dm-stripe:ext4:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\extr' with lp ls extr, \
"../data/filebench/pm-array:odinfs:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\sys' with lp ls odinfs, \
"../data/filebench/pm-char-array:sufs:filebench_webproxy:bufferedio.dat" \
 using 1:($2/1000) title '\sufs' with lp ls sufs, \

eval mpNext
set ylabel 'KOps/sec'
set xlabel '\# threads'
set title '(d) Varmail' offset 0,-1
plot \
"../data/filebench/pmem-local:ext4:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\ext' with lp ls ext, \
"../data/filebench/pmem-local:pmfs:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\pmfs' with lp ls pmfs, \
"../data/filebench/pmem-local:nova:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\nova' with lp ls nova, \
"../data/filebench/pmem-local:winefs:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\winefs' with lp ls winefs, \
   "../data/filebench/pmem-local:splitfs:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\splitfs' with lp ls splitfs, \
"../data/filebench/dm-stripe:ext4:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\extr' with lp ls extr, \
"../data/filebench/pm-array:odinfs:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\odinfs' with lp ls odinfs, \
"../data/filebench/pm-char-array:sufs:filebench_varmail:bufferedio.dat" \
 using 1:($2/1000) title '\sufs' with lp ls sufs, \

