#!/usr/bin/env python3
import os
import sys
import signal
import subprocess
import datetime
import tempfile
import optparse
import time
import pdb
import cpupol
from os.path import join

CUR_DIR = os.path.abspath(os.path.dirname(__file__))

class DBench(object):
    DBBENCH_BIN      = "db_bench"
    DBBENCH_SUFS_BIN = "db_bench_sufs"
    DBBENCH_OPS      = "1000000"

    def __init__(self, type_, root_, fs_, dthreads_, dsockets_, delegate_):
        self.workload = type_
        self.root = root_
        self.fs = fs_
        self.dthreads = int(dthreads_)
        self.dsockets = int(dsockets_)
        self.delegate = bool(int(delegate_))

        if (fs_ == "sufs"):
            self.bin = DBench.DBBENCH_SUFS_BIN
        else:
            self.bin = DBench.DBBENCH_BIN

    def __del__(self):
        # clean up
        try:
            if self.bench_out:
                os.unlink(self.bench_out.name)
        except:
            pass

    def _get_cpu_ranges(self):
        ret = ' '
        core_per_socket = cpupol.CORE_PER_CHIP;

        for i in range(1, int(self.dsockets) + 1):
            begin = core_per_socket * (i - 1) + int(self.dthreads);
            end = core_per_socket * i - 1;
            if (i > 1):
                ret = ret + ',';

            ret = ret + str(begin) + '-' + str(end)

        return ret;

    def _exec_cmd(self, cmd, out=None):
        p = subprocess.Popen(cmd, shell=True, stdout=out, stderr=out)
        return p

    def run(self):
        taskset_cmd= ' '
        if (self.delegate):
            taskset_cmd = 'taskset -a -c ' + self._get_cpu_ranges()

        with tempfile.NamedTemporaryFile(delete=False) as self.bench_out:
            cmd = "sudo %s %s --benchmarks=%s --compression=0 --num=%s --db=%s" % (taskset_cmd, 
                        self.bin, self.workload, DBench.DBBENCH_OPS, self.root)
                        
            p = self._exec_cmd(cmd, subprocess.PIPE)

            for l in p.stdout.readlines():
                self.bench_out.write("#@ ".encode("utf-8"))
                self.bench_out.write(l)
                l_str = str(l)
                idx = l_str.find(self.workload)
                if idx is not -1:
                    self.perf_msg = l_str[idx+len(self.workload):]

            self.bench_out.flush()
        return 0

    def report(self):

        item = self.perf_msg.split(';')[0]
        tp = float(item.split(':')[1].strip().split(' ')[0]) / 1000
       
        profile_name = ""
        profile_data = ""
        # we don't have works for dbench..
        print("# ncpu works/msec %s" % profile_name)
        print("%s %s %s" % ("1", str(tp), profile_data))

if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--type", help="workload name")
    parser.add_option("--root", help="benchmark root directory")
    parser.add_option("--fs", help="fs")
    parser.add_option("--delegation_threads", help="delegation per socket")
    parser.add_option("--delegation_sockets", help="sockets on which delegation threads will run")
    parser.add_option("--delegate", help="reserve delegation threads or not")
    (opts, args) = parser.parse_args()

    # check options
    for opt in vars(opts):
        val = getattr(opts, opt)
        if val == None:
            print("Missing options: %s" % opt)
            parser.print_help()
            exit(1)

    # run benchmark
    dbench = DBench(opts.type, opts.root, opts.fs,
                          opts.delegation_threads, opts.delegation_sockets, 
                          opts.delegate)
    
    rc = dbench.run()
    dbench.report()
    exit(rc)

