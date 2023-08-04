#!/usr/bin/env python3
import optparse
import os
import subprocess

fig5_data_fs = ["nova", "odinfs", "sufs", "splitfs", "strata"]
fig5_meta_fs = ["nova", "strata", "sufs"]

fig10_fs = ["ext4", "pmfs", "nova", "winefs", "ext4-raid", "odinfs", "sufs", "sufs-fd", "sufs-kv"]

table5_fs = ["ext4", "nova", "winefs", "sufs"]

media  = {"ext4"      : "pmem-local",
          "pmfs"      : "pmem-local", 
          "nova"      : "pmem-local",
          "winefs"    : "pmem-local",
          "splitfs"   : "pmem-local",
          "strata"    : "pm-char",
          "ext4-raid" : "dm-stripe",
          "odinfs"    : "pm-array", 
          "sufs"      : "pm-char-array",
          "sufs-fd"   : "pm-char-array",
          "sufs-kv"   : "pm-char-array"
          }


fio_bm = ["seq-read-4K", "seq-write-4K", "seq-read-2M", "seq-write-2M"]
fxmark_bm = ["MRPL", "MWCL", "MWUL"]
filebench_bm = ["filebench_webproxy", "filebench_varmail-fd"]
dbench_bm = ["dbench_fill100K", "dbench_fillseq", "dbench_fillsync", 
             "dbench_fillrandom", "dbench_readrandom", "dbench_deleterandom"]

def gen_data(fsl, log_dir, bm, out):

    per_str="1 "
    if (bm in filebench_bm):
        per_str="8 "

    end_str=" "
    if (bm in dbench_bm):
        end_str="|"

    for f in fsl:
        name=f
        if name == "ext4-raid":
            name = "ext4"

        fname = "%s:%s:%s:bufferedio.dat" % (media[f], name, bm)
        f = os.path.join(log_dir, fname)
        
        try:
            with open(f, "r") as input:
                for line in input:
                    if line.startswith(per_str):
                        num=line.split(" ")[1].strip()
                        print(num, end=end_str, file=out)                
                        break
        except:
            print("0", end=end_str, file=out)


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--fio_log", help = "log directory of fio")
    parser.add_option("--fxmark_log", help = "log directory of fxmark")
    parser.add_option("--filebench_log", help = "log directory of filebench")
    parser.add_option("--dbench_log", help = "log directory of dbench")
    parser.add_option("--out",   help="output directory")
    parser.add_option("--out_table",   help="Table output directory")
    (opts, args) = parser.parse_args()

    # check arg
    for opt in vars(opts):
        val = getattr(opts, opt)
        if val == None:
            print("Missing options: %s" % opt)
            parser.print_help()
            exit(1)

    subprocess.call("mkdir -p %s" % opts.out, shell=True)

    data_file = os.path.join(opts.out, "single-data.dat")
    with open(data_file, "w") as out:
        for bm in fio_bm:
            if "read" in bm:
                print("read", end=" ", file=out)
            else:
                print("write", end=" ", file=out)

            gen_data(fig5_data_fs, opts.fio_log, bm, out)
            print("", file=out)
        
    meta_file = os.path.join(opts.out, "single-meta.dat")
    with open(meta_file, "w") as out:
        for bm in fxmark_bm:
            if bm == "MRPL":
                print("open", end=" ", file=out)
            elif bm == "MWCL":
                print("create", end=" ", file=out)
            else:
                print("delete", end=" ", file=out)

            gen_data(fig5_meta_fs, opts.fxmark_log, bm, out)
            print("", file=out)

    cus_file = os.path.join(opts.out, "customization.dat")
    with open(cus_file, "w") as out:
        for bm in filebench_bm:
            if bm == "filebench_webproxy":
                print("Webproxy", end=" ", file=out)
            elif bm == "filebench_varmail-fd":
                print("Varmail", end=" ", file=out)

            gen_data(fig10_fs, opts.filebench_log, bm, out)
            print("", file=out)

    table_file = os.path.join(opts.out_table, "table5.md")
    with open(table_file, "w") as out:

        #Table header
        print("Throughput(ops/ms)", end='', file=out)
        for f in table5_fs:
            print("|%s" % (f), end='', file=out)
        print("|", file=out)

        for bm in dbench_bm:
            print("|%s|" % (bm), end='', file=out)
            gen_data(table5_fs, opts.dbench_log, bm, out)
            print("", file=out)