#call "common.gnuplot" "3.4in, 3.9in"
#call "common.gnuplot" "3.4in, 7.8in"
#call "common.gnuplot" "3.4in, 7.8in"
call "common.gnuplot" "7.8in, 3.4in"

set terminal postscript color
set output "fig7.eps"


mp_startx=0.1
mp_starty=0.05
mp_height=0.9
mp_rowgap=0.08
mp_colgap=0.1
mp_width=0.85

eval mpSetup(2,6)

# read
eval mpNext
unset xlabel
unset key

set ylabel 'ops/$\mu$s'
set title  '\dwtl' offset 0,-1

plot \
'../data/fxmark/pmem-local:ext4:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\ext' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\pmfs' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\nova' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\winefs' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\extr' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\odinfs' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:DWTL:bufferedio.dat' using 1:($2/1000000) \
 title '\sufs' with lp ls sufs, \

eval mpNext
unset xlabel
unset ylabel
set title  '\mrpl' offset 0,-1

plot \
'../data/fxmark/pmem-local:ext4:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MRPL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset xlabel
unset ylabel
set title  '\mrpm' offset 0,-1

plot \
'../data/fxmark/pmem-local:ext4:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MRPM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset xlabel
unset key
set title  '\mrph' offset 0,-1
set ylabel 'ops/$\mu$s'

plot \
'../data/fxmark/pmem-local:ext4:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MRPH:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mrdl' offset 0,-1

plot \
'../data/fxmark/pmem-local:ext4:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MRDL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key #horizontal maxrows 1 maxcolumns 5 at 0.($2/1000000), 0.95
set title  '\mrdm' offset 0,-1
set ylabel 'ops/$\mu$s'

plot \
'../data/fxmark/pmem-local:ext4:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MRDM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mwcl' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWCL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mwcm' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWCM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mwul' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWUL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mwum' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWUM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \


eval mpNext
unset key
unset ylabel
set title  '\mwrl' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWRL:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

eval mpNext
unset key
unset ylabel
set title  '\mwrm' offset 0,-1
unset ylabel

plot \
'../data/fxmark/pmem-local:ext4:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls ext, \
'../data/fxmark/pmem-local:pmfs:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls pmfs, \
'../data/fxmark/pmem-local:nova:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls nova, \
'../data/fxmark/pmem-local:winefs:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls winefs, \
'../data/fxmark/dm-stripe:ext4:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls extr, \
'../data/fxmark/pm-array:odinfs:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls odinfs, \
'../data/fxmark/pm-char-array:sufs:MWRM:bufferedio.dat' using 1:($2/1000000) \
 title '' with lp ls sufs, \

