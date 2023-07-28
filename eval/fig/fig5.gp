call "common.gnuplot" "3.3in, 1.3in"

set terminal postscript color
set output "fig5.eps"

# set multiplot layout 1,2

mp_startx=0.05
mp_starty=0.23
mp_height=0.70
mp_rowgap=0.10
mp_colgap=0.08
mp_width=0.9

eval mpSetup(4, 1)

eval mpNext
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set ylabel 'Throughput (GiB/s)'
set title '(a) 4K data'
#set ytics rotate by 60
set xtics rotate by 20
set ytics 1
#unset xtics
set xtics offset -3,-1.5
set key at 5,4.0
set key top
set key maxrows 1 width 4
plot \
"<(sed -n '1,2p' ../data/extra/single-data.dat)" using ($5/1024/1024):xtic(1) title '\splitfs' lc rgb C7 fillstyle pattern 2, \
'' using ($2/1024/1024):xtic(1) title '\nova' lc rgb C5 fillstyle pattern 4, \
'' using ($3/1024/1024):xtic(1) title '' lc rgb C3 fillstyle pattern 7, \
'' using ($4/1024/1024):xtic(1) title '' lc rgb C6 fillstyle pattern 3, \
# splitfs nova odinfs, sys

eval mpNext
unset key
unset ylabel
set title '(b) 2M data'
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set ytics 10
set key at 4.3,53
plot \
"<(sed -n '3,4p' ../data/extra/single-data.dat)" using ($5/1024/1024):xtic(1) title '' lc rgb C7 fillstyle pattern 2, \
'' using ($3/1024/1024):xtic(1) title '\odinfs' lc rgb C3 fillstyle pattern 7, \
'' using ($4/1024/1024):xtic(1) title '' lc rgb C6 fillstyle pattern 3, \
#splitfs odinfs sys

eval mpNext
unset key
set ylabel 'Throughput (ops/$\mu$s)'
set title '(c) Read metadata'
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set ytics 1
#set xtics offset -2.5, -1
#unset xtics
set key at 2,5.3
plot \
"<(sed -n '1,1p' ../data/extra/single-meta.dat)" using ($2/1000/1000):xtic(1) title '' lc rgb C5 fillstyle pattern 4, \
'' using ($6/1000/1000):xtic(1) title '\strata' lc rgb C2 fillstyle pattern 6, \
'' using ($4/1000/1000):xtic(1) title '' lc rgb C6 fillstyle pattern 3,\
#nova strata sys

eval mpNext
unset key
unset ylabel
set title '(d) Write metadata'
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set ytics 0.3
set key at 3,1.6
plot \
"<(sed -n '2,3p' ../data/extra/single-meta.dat)" using ($2/1000/1000):xtic(1) title '' lc rgb C5 fillstyle pattern 4, \
'' using ($6/1000/1000):xtic(1) title '' lc rgb C2 fillstyle pattern 6, \
'' using ($4/1000/1000):xtic(1) title '\sys' lc rgb C6 fillstyle pattern 3, \
#nova strata sys