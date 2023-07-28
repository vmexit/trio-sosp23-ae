call "common.gnuplot" "3.3in, 1.5in"

set terminal postscript color
set output "fig10.eps"

mp_startx=0.05
mp_starty=0.10
mp_height=0.70
mp_rowgap=0.10
mp_colgap=0.10
mp_width=1.0

eval mpSetup(2, 1)

eval mpNext
unset ylabel
set key at 0.22,2000
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set ylabel 'Kops/sec'
set yrange [0:]
set ytics  500

plot \
"<(sed -n '1,1p' ../data/extra/customization.dat)" \
using ($2/1000):xtic(1) title '\ext' lc rgb C3 fillstyle pattern 1, \
'' using ($3/1000):xtic(1) title '\pmfs' lc rgb C7 fillstyle pattern 2, \
'' using ($4/1000):xtic(1) title '\nova' lc rgb C5 fillstyle pattern 4, \
'' using ($5/1000):xtic(1) title '' lc rgb C2 fillstyle pattern 6, \
'' using ($6/1000):xtic(1) title '\extr' lc rgb C2 fillstyle pattern 5, \
'' using ($7/1000):xtic(1) title '' lc rgb C1 fillstyle pattern 7, \
'' using ($8/1000):xtic(1) title '' lc rgb C6 fillstyle pattern 3, \
'' using ($9/1000):xtic(1) title 'FPFS' lc rgb C5 fillstyle pattern 9, \


eval mpNext
unset ylabel
set style data histogram
set style fill pattern
set boxwidth 0.8 absolute
set key at 0.2, 1600
set ytics  400

plot \
"<(sed -n '2,2p' ../data/extra/customization.dat)" \
using ($2/1000):xtic(1) title '' lc rgb C3 fillstyle pattern 1, \
'' using ($3/1000):xtic(1) title '' lc rgb C7 fillstyle pattern 2, \
'' using ($4/1000):xtic(1) title '' lc rgb C5 fillstyle pattern 4, \
'' using ($5/1000):xtic(1) title '\winefs' lc rgb C2 fillstyle pattern 6, \
'' using ($6/1000):xtic(1) title '' lc rgb C2 fillstyle pattern 5, \
'' using ($7/1000):xtic(1) title '\odinfs' lc rgb C1 fillstyle pattern 7, \
'' using ($8/1000):xtic(1) title '\sys' lc rgb C6 fillstyle pattern 3, \
'' using ($9/1000):xtic(1) title 'KVFS' lc rgb C6 fillstyle pattern 10, \