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
import re
import io
from os.path import join
import cpupol
import strata
import splitfs

CUR_DIR = os.path.abspath(os.path.dirname(__file__))

# To specify a job, please use the format <configuration>-<task>
# The configuration is the global part of the fio config. You can specify the
# numa policy, cpu affinity, etc. there
# The task specifies the benchmark. You can specify the read/write size, etc.
# There is no ultimate rule which parameter should be inside global or task --
# use whichever is the most.


# The script will search for the <configuration>.fio and the <task>.fio inside the
# bin/fio-workloads directory. The script then appends the root directory of the
# device, and merges the two .fio files together, and run the benchmark.
class Fio(object):
    PERF_STR = "fio-"
    WORKLOAD_DIR = os.path.normpath(os.path.join(CUR_DIR, "fio-workloads"))
    CREATE_FILE = os.path.normpath(os.path.join(CUR_DIR, "create-file"))
    CREATE_FILE_SUFS = os.path.normpath(os.path.join(CUR_DIR, "create-file-sufs"))

    LS_DIR_SUFS = os.path.join("ls-sufs")
#    CREATE_DIR_SUFS = os.path.join("mkdir-sufs")
    PERF_PATTERN = re.compile("")
    CMD_OPTIONS = ' '.join(["--output-format", "terse"])
    DEBUG_SCRIPT = False
    TERSE_FIELDS = [
        "terse_version_3", "fio_version", "jobname", "groupid", "error",
        "read_kb", "read_bandwidth", "read_iops", "read_runtime_ms",
        "read_slat_min", "read_slat_max", "read_slat_mean", "read_slat_dev",
        "read_clat_min", "read_clat_max", "read_clat_mean", "read_clat_dev",
        #XXX: I am not sure how does FIO catagorize latency percentile output
        #The below percentile seems correct with the benchmarks we use
        #1%                5%                  10%
        "read_clat_pct01", "read_clat_pct02", "read_clat_pct03",
        #20%               30%                 40%
        "read_clat_pct04", "read_clat_pct05", "read_clat_pct06",
        #50%               60%                 70%
        "read_clat_pct07", "read_clat_pct08", "read_clat_pct09",
        #80%               90%                 95%
        "read_clat_pct10", "read_clat_pct11", "read_clat_pct12",
        #99%               99.5%              99.9%
        "read_clat_pct13", "read_clat_pct14", "read_clat_pct15",
        #99.95%            99.99%              Not used
        "read_clat_pct16", "read_clat_pct17", "read_clat_pct18",
        #Not used           Not used
        "read_clat_pct19", "read_clat_pct20",
        "read_tlat_min", "read_lat_max",
        "read_lat_mean", "read_lat_dev", "read_bw_min", "read_bw_max",
        "read_bw_agg_pct", "read_bw_mean", "read_bw_dev", "write_kb",
        "write_bandwidth", "write_iops", "write_runtime_ms", "write_slat_min",
        "write_slat_max", "write_slat_mean", "write_slat_dev",
        "write_clat_min", "write_clat_max", "write_clat_mean",
        "write_clat_dev",
         #1%                5%                  10%
        "write_clat_pct01", "write_clat_pct02", "write_clat_pct03",
        #20%               30%                 40%
        "write_clat_pct04", "write_clat_pct05", "write_clat_pct06",
        #50%               60%                 70%
        "write_clat_pct07", "write_clat_pct08", "write_clat_pct09",
        #80%               90%                 95%
        "write_clat_pct10", "write_clat_pct11", "write_clat_pct12",
        #99%               99.5%              99.9%
        "write_clat_pct13", "write_clat_pct14", "write_clat_pct15",
        #99.95%            99.99%              Not used
        "write_clat_pct16", "write_clat_pct17", "write_clat_pct18",
        #Not used           Not used
        "write_clat_pct19", "write_clat_pct20",
        "write_tlat_min", "write_lat_max", "write_lat_mean", "write_lat_dev",
        "write_bw_min", "write_bw_max", "write_bw_agg_pct", "write_bw_mean",
        "write_bw_dev", "cpu_user", "cpu_sys", "cpu_csw", "cpu_mjf",
        "cpu_minf", "iodepth_1", "iodepth_2", "iodepth_4", "iodepth_8",
        "iodepth_16", "iodepth_32", "iodepth_64", "lat_2us", "lat_4us",
        "lat_10us", "lat_20us", "lat_50us", "lat_100us", "lat_250us",
        "lat_500us", "lat_750us", "lat_1000us", "lat_2ms", "lat_4ms",
        "lat_10ms", "lat_20ms", "lat_50ms", "lat_100ms", "lat_250ms",
        "lat_500ms", "lat_750ms", "lat_1000ms", "lat_2000ms",
        "lat_over_2000ms", "disk_name", "disk_read_iops", "disk_write_iops",
        "disk_read_merges", "disk_write_merges", "disk_read_ticks",
        "write_ticks", "disk_queue_time", "disk_util"
    ]

    def calc_num_jobs(self, ncore, rcore, delegation_threads, delegation_sockets):
        if not self.rcore:
            return ncore
        else:
            socket = (ncore - 1) // cpupol.CORE_PER_CHIP + 1

            if (socket <= delegation_sockets):
                max_jobs = socket * (cpupol.CORE_PER_CHIP - delegation_threads)
            else:
                max_jobs = socket * cpupol.CORE_PER_CHIP - delegation_sockets * delegation_threads

            if ncore < max_jobs:
                return ncore
            else:
                return max_jobs



    def __init__(self, type_, ncore_, duration_, root_, profbegin_, profend_,
                 proflog_, media_, fs_, bench_, nfg_, dio_, numa_cpu_node_,
                 delegation_threads_, delegation_sockets_, delegate_, rcore_):
        # take configuration parameters
        self.workload = type_
        self.ncore = int(ncore_)
        self.duration = int(duration_)
        self.root = root_
        self.profbegin = profbegin_
        self.profend = profend_
        self.proflog = proflog_
        self.media = media_
        self.fs = fs_
        self.bench = bench_
        self.nfg = nfg_
        self.dio = dio_
        self.numa_cpu_node = numa_cpu_node_
        self.profenv = ' '.join([
            "PERFMON_LEVEL=%s" % os.environ.get('PERFMON_LEVEL', "x"),
            "PERFMON_LDIR=%s" % os.environ.get('PERFMON_LDIR', "x"),
            "PERFMON_LFILE=%s" % os.environ.get('PERFMON_LFILE', "x")
        ])
        self.global_config = self.workload.split('_')[0]
        self.benchmark_config = self.workload.split('_')[1:]
        self.run_file = "%s-%s.fio" % (self.global_config,
                                       self.benchmark_config)
        self.perf_msgs = list()
        self.seq_cores = cpupol.seq_cores
        self.delegation_threads = int(delegation_threads_)
        self.delegation_sockets = int(delegation_sockets_)
        self.delegate = bool(int(delegate_))
        self.rcore = bool(int(rcore_))
        self.file_size = 1073741824 # FIXME

        self.numjobs = self.calc_num_jobs(self.ncore, self.rcore,
                                          self.delegation_threads,
                                          self.delegation_sockets)

        if self.DEBUG_SCRIPT:
            print("delegation_threads: %d, delegation_sockets: %d, rcore: %s, ncore: %d, jobs %d" %
                  (self.delegation_threads, self.delegation_sockets, self.rcore, self.ncore, self.numjobs))

        if self.delegate:
            # disable the cpus on the delegation threads for each delegation socket
            for socket in range(self.delegation_sockets):
                for delegation_thread in range(self.delegation_threads):
                    self.seq_cores.remove(socket * cpupol.CORE_PER_CHIP + delegation_thread)

        self.env=""
        if (self.fs == "strata"):
            self.env = ("LD_PRELOAD=%s" % 
               (os.path.normpath(os.path.join(CUR_DIR, strata.lib))))
        
        if (self.fs == "splitfs"):
            self.env = (splitfs.ledge_str + " " +  
                        ("NVP_TREE_FILE=%s " % (os.path.normpath(os.path.join(CUR_DIR, splitfs.tree))) 
                         + 
                        ("LD_LIBRARY_PATH=%s " % os.path.normpath(CUR_DIR))
                        + ("LD_PRELOAD=%s "  %  os.path.normpath(os.path.join(CUR_DIR, splitfs.lib))))) 

        if (self.fs == "sufs"):
            cmd = "sudo %s %s/%s %s %s %s %s" % (Fio.CREATE_FILE_SUFS, self.root,
                    self.benchmark_config[0], self.file_size, self.numjobs,
                    self.delegation_threads, self.delegation_sockets)

            self.exec_cmd(cmd)
            self.fio = "fio-sufs"
        else:
            # create files for fio
            cmd = "sudo %s %s %s/%s %s %s %s %s" % (self.env, Fio.CREATE_FILE, 
                    self.root, self.benchmark_config[0], self.file_size, 
                    self.numjobs, self.delegation_threads, self.delegation_sockets)
            
            if self.fs == "strata" or self.fs == "splitfs":
                self.exec_cmd(cmd, subprocess.DEVNULL)
            else:
                self.exec_cmd(cmd)

            # self.exec_cmd(cmd)

            self.fio = "fio"

    def __del__(self):
        # clean up
        try:
            if self.jobfile:
                os.unlink(self.jobfile.name)
            if self.bench_out:
                os.unlink(self.bench_out.name)
        except:
            pass

    def run(self):
        # set up benchmark configuration
        if not self.generate_config():
            return -1
        # start performance profiling
        self.exec_cmd("%s %s" % (self.profenv, self.profbegin))
        # run fio
        self._run_fio()
        # stop performance profiling
        self.exec_cmd("%s %s" % (self.profenv, self.profend))
        return

    def generate_env_var(self):
        if (self.fs == 'sufs'):
            cmd = "%s -t %s" % (self.LS_DIR_SUFS, self.root)
            p = self.exec_cmd(cmd, subprocess.PIPE)
            out, err = p.communicate()
            return ("sufs_preload_file=%s" % (out.decode("utf-8").rstrip()))
        elif self.fs == 'strata' or self.fs == 'splitfs':
            return self.env
        else:
            return ""

    def warmup(self, cmd, times):
        for i in range(times):
            self.exec_cmd(cmd, subprocess.PIPE)

    def _run_fio(self):
        with tempfile.NamedTemporaryFile(delete=False) as self.bench_out:
            fio_env = self.generate_env_var()
            cmd = "sudo %s %s %s %s" % (fio_env, self.fio, self.config.name, self.CMD_OPTIONS)

            if self.fs == "sufs":
                if ( ("read" in self.bench) and ("4K" in self.bench) ):
                    self.warmup(cmd, 10)
                else:
                    self.warmup(cmd, 1)
            elif ("read" in self.bench):
                self.warmup(cmd, 10)
            else:
                self.warmup(cmd, 1)

            p = subprocess.Popen(cmd, shell= True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            p.wait()

            for line in p.stdout.readlines():
                str_line = str(line)
                if str_line.find(fio.PERF_STR) != -1:
                    self.perf_msgs.append(str_line.rstrip())
                    break

                # if line is b'':
                #     break
                # self.perf_msgs.append(line.rstrip().decode("utf-8"))

        return

    def report(self):
        # TODO: add multi-tests support
        # 3;fio-3.27-84-gf7942;pmem-write-4K;0;0;0;0;0;0;0;0;0.000000;0.000000;0;0;0.000000;0.000000;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0;0;0.000000;0.000000;0;0;0.000000%;0.000000;0.000000;316663548;5277549;1319387;60002;0;0;0.000000;0.000000;0;0;0.000000;0.000000;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0%=0;0;0;0.000000;0.000000;4467223;5840349;100.000000%;5285203.613445;58970.989578;6.784153%;93.208764%;514;0;78;100.0%;0.0%;0.0%;0.0%;0.0%;0.0%;0.0%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;0.00%;pmem7;0;0;0;0;0;0;0;0.00%
        for perf_msg in self.perf_msgs:
            output = perf_msg.split(';')

            profile_name = ' '.join(self.TERSE_FIELDS)
            profile_data = ' '.join(output)
            fio_dict = dict(zip(self.TERSE_FIELDS, output))

        (read_iops, write_iops) = int(fio_dict["read_iops"]), int(
            fio_dict["write_iops"])
        (read_bandwidth, write_bandwidth) = int(fio_dict["read_bandwidth"]), int(
            fio_dict["write_bandwidth"])

        (read_50_latency, read_99_latency) = fio_dict["read_clat_pct07"], fio_dict["read_clat_pct13"]
        (write_50_latency, write_99_latency) = fio_dict["write_clat_pct07"], fio_dict["write_clat_pct13"]

        jobname = fio_dict["jobname"]

        print("## %s:%s:%s:%s:%s" %
              (self.media, self.fs, jobname, self.nfg, self.dio))
        print("# ncpu secs read_iops write_iops "
              "read_bandwidth write_bandwidth "
              "read_50_latency read_99_latency "
              "write_50_latency write_99_latency")

        print("%s %s %s %s %s %s %s %s %s %s" %
              (self.ncore, self.duration, read_iops, write_iops,
                  read_bandwidth, write_bandwidth,
                  read_50_latency, read_99_latency,
                  write_50_latency, write_99_latency))

    def generate_config(self):
        def chunks(lst, n):
            # https://stackoverflow.com/questions/312443/how-do-you-split-a-list-into-evenly-sized-chunks
            for i in range(0, len(lst), n):
                yield lst[i:i + n]

        global_config_template = os.path.normpath(
            os.path.join(Fio.WORKLOAD_DIR, self.global_config + ".fio"))
        benchmark_config_list = self.benchmark_config
        # create a configured workload file
        self.config = tempfile.NamedTemporaryFile(delete=False)
        self.config.write(b'# auto generated by fxmark\n')
        self.config.close()
        split_jobs = self.numjobs // len(benchmark_config_list)
        split_seq_cores = list(chunks(self.seq_cores, split_jobs))
        for idx, benchmark_config in enumerate(benchmark_config_list):
            benchmark_config_template = os.path.normpath(
                os.path.join(Fio.WORKLOAD_DIR, benchmark_config + ".fio"))
            if not os.path.isfile(global_config_template) or not os.path.isfile(
                    benchmark_config_template):
                return False
            self.exec_cmd("cat %s >> %s" %
                          (global_config_template, self.config.name))
            self.setup_workload(split_jobs, split_seq_cores[idx])
            self.exec_cmd("cat %s >> %s" %
                          (benchmark_config_template, self.config.name))
        return True

    def setup_workload(self, numjobs, seq_cores):
        self.append_to_config("directory=%s" % self.root)
        self.append_to_config("runtime=%ds" % self.duration)
        self.append_to_config("time_based")
        self.append_to_config("numjobs=%d" % numjobs)
        self.append_to_config("cpus_allowed=%s" %
                              ','.join(str(cpu) for cpu in seq_cores))
        # self.append_to_config("numa_cpu_nodes=%s" % self.numa_cpu_node)
        return

    def append_to_config(self, config_str):
        self.exec_cmd("echo \'%s\' >> %s" % (config_str, self.config.name))
        return

    def exec_cmd(self, cmd, out=None):
        if self.DEBUG_SCRIPT:
            print(cmd)
        p = subprocess.Popen(cmd, shell=True, stdout=out, stderr=out)
        p.wait()
        return p


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--type", help="workload name")
    parser.add_option("--ncore", help="number of core")
    parser.add_option("--duration", help="benchmark time in seconds")
    parser.add_option("--root", help="benchmark root directory")
    parser.add_option("--profbegin", help="profile begin command")
    parser.add_option("--profend", help="profile end command")
    parser.add_option("--proflog", help="profile log path")
    parser.add_option("--media", help="media")
    parser.add_option("--fs", help="fs")
    parser.add_option("--bench", help="bench")
    parser.add_option("--nfg", help="nfg")
    parser.add_option("--dio", help="dio")
    parser.add_option("--numa_cpu_node", help="numa cpu node")
    parser.add_option("--delegation_threads", help="delegation per socket")
    parser.add_option("--delegation_sockets", help="sockets on which delegation threads will run")
    parser.add_option("--delegate", help="reserve delegation threads or not")
    parser.add_option("--rcore", help="reduce #of app threads or not")
    (opts, args) = parser.parse_args()

    # check options
    for opt in vars(opts):
        val = getattr(opts, opt)
        if val == None:
            print("Missing options: %s" % opt)
            parser.print_help()
            exit(1)

    # run benchmark
    fio = Fio(opts.type, opts.ncore, opts.duration, opts.root, opts.profbegin,
              opts.profend, opts.proflog, opts.media, opts.fs, opts.bench,
              opts.nfg, opts.dio, opts.numa_cpu_node, opts.delegation_threads,
              opts.delegation_sockets, opts.delegate, opts.rcore)
    rc = fio.run()
    fio.report()
    exit(rc)
