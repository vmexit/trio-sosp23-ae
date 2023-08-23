```
/usr/local/bin/fio
fxmark/bin/create-file
fxmark/bin/fxmark
/usr/local/bin/filebench
/usr/local/bin/filebench-fd
/usr/local/bin/filebench-kvs-sufs
================================
Cannot open /dev/supremefs
./check.sh: line 20: 17823 Aborted                 (core dumped) $i
fio-sufs segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17825 Aborted                 (core dumped) $i
fxmark/bin/create-file-sufs segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17827 Aborted                 (core dumped) $i
fxmark/bin/fxmark-sufs segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17829 Aborted                 (core dumped) $i
filebench-sufs segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17831 Aborted                 (core dumped) $i
filebench-fd-sufs segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17833 Aborted                 (core dumped) $i
filebench-fd-sufs-fd segfaults as expected
================================


================================
Cannot open /dev/supremefs
./check.sh: line 20: 17835 Aborted                 (core dumped) $i
filebench-kvs-sufs segfaults as expected
================================

All passed!
```