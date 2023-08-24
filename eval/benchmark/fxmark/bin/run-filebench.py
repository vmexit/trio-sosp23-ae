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
import splitfs

from os.path import join

CUR_DIR = os.path.abspath(os.path.dirname(__file__))

class FileBench(object):
    WORKLOAD_DIR = os.path.normpath(os.path.join(CUR_DIR, "filebench-workloads"))
    PRE_SCRIPT = os.path.normpath(os.path.join(CUR_DIR, "turnoff-aslr"))

    FILEBENCH_BIN      = "filebench"
    FILEBENCH_SUFS_BIN = "filebench-sufs"

    FILEBENCH_FD_BIN = "filebench-fd"
    FILEBENCH_FD_SUFS_BIN = "filebench-fd-sufs"
    FILEBENCH_FD_SUFS_FD_BIN = "filebench-fd-sufs-fd"

    FILEBENCH_KV_SUFS_BIN = "filebench-kvs-sufs"

    PERF_STR = "IO Summary: "
    VIDEO_READ_STR = "vidreader"
    VIDEO_WRITE_STR = "newvid"

    VARMAIL_FILE_SIZE = 10000
    VARMAIL_FILESET_TEMP = "define fileset name=bigfileset%d,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=80"

    #4 arguments
    VARMAIL_THREAD_ARG_CNT = 4
    VARMAIL_THREAD_TEMP = """    flowop deletefile name=deletefile1,filesetname=bigfileset%d
    flowop createfile name=createfile2,filesetname=bigfileset%d,fd=1
    flowop appendfilerand name=appendfilerand2,iosize=$meanappendsize,fd=1
    flowop fsync name=fsyncfile2,fd=1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=$iosize
    flowop appendfilerand name=appendfilerand3,iosize=$meanappendsize,fd=1
    flowop fsync name=fsyncfile3,fd=1
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=$iosize
    flowop closefile name=closefile4,fd=1"""


    #4 arguments
    VARMAIL_KVS_THREAD_ARG_CNT = 7
    VARMAIL_KVS_THREAD_TEMP = """    flowop deletefile name=deletefile1,filesetname=bigfileset%d
    flowop createfile name=createfile2,filesetname=bigfileset%d,fd=1
    flowop appendfilerand name=appendfilerand2,iosize=$meanappendsize,fd=1
    flowop fsync name=fsyncfile2,fd=1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile3,filesetname=bigfileset%d,iosize=$iosize
    flowop writewholefile name=appendfilerand3,iosize=$meanappendsize,filesetname=bigfileset%d
    flowop fsync name=fsyncfile3,fd=1
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile4,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile4,fd=1"""

#    17 arguments
#    VARMAIL_THREAD_ARG_CNT = 17
#    VARMAIL_THREAD_TEMP = """    flowop deletefile name=deletefile%d1,filesetname=bigfileset%d
#    flowop createfile name=createfile%d2,filesetname=bigfileset%d,fd=1
#    flowop appendfilerand name=appendfilerand%d2,iosize=$meanappendsize,fd=1
#    flowop fsync name=fsyncfile%d2,fd=1
#    flowop closefile name=closefile%d2,fd=1
#    flowop openfile name=openfile%d3,filesetname=bigfileset%d,fd=1
#    flowop readwholefile name=readfile%d3,fd=1,iosize=$iosize
#    flowop appendfilerand name=appendfilerand%d3,iosize=$meanappendsize,fd=1
#    flowop fsync name=fsyncfile%d3,fd=1
#    flowop closefile name=closefile%d3,fd=1
#    flowop openfile name=openfile%d4,filesetname=bigfileset%d,fd=1
#    flowop readwholefile name=readfile%d4,fd=1,iosize=$iosize
#    flowop closefile name=closefile%d4,fd=1"""

    WEBPROXY_FILE_SIZE = 10000
    WEBPROXY_FD_FILE_SIZE = 100000
    WEBPROXY_FILESET_TEMP = "define fileset name=bigfileset%d,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=80"

    WEBPROXY_THREAD_ARG_CNT = 7
    WEBPROXY_THREAD_TEMP = """    flowop deletefile name=deletefile1,filesetname=bigfileset%d
    flowop createfile name=createfile1,filesetname=bigfileset%d,fd=1
    flowop appendfilerand name=appendfilerand1,iosize=$meaniosize,fd=1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile2,fd=1,iosize=$iosize
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=$iosize
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=$iosize
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile5,fd=1,iosize=$iosize
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile6,fd=1,iosize=$iosize
    flowop closefile name=closefile6,fd=1"""

    WEBPROXY_KVS_THREAD_ARG_CNT = 12
    WEBPROXY_KVS_THREAD_TEMP = """    flowop deletefile name=deletefile1,filesetname=bigfileset%d
    flowop createfile name=createfile1,filesetname=bigfileset%d,fd=1
    flowop appendfilerand name=appendfilerand1,iosize=$meaniosize,fd=1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile2,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile3,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile4,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile5,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset%d,fd=1
    flowop readwholefile name=readfile6,filesetname=bigfileset%d,iosize=$iosize
    flowop closefile name=closefile6,fd=1"""


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


    def __init__(self, type_, ncore_, nsocket_, duration_, root_,
                 profbegin_, profend_, proflog_, fs_,
                 delegation_threads_, delegation_sockets_,
                 delegate_, rcore_):
        self.config = None
        self.bench_out = None
        # take configuration parameters
        self.workload = type_
        self.ncore = int(ncore_)
        self.nsocket = int(nsocket_)
        self.duration = int(duration_)
        self.root = root_
        self.profbegin = profbegin_
        self.profend = profend_
        self.proflog = proflog_
        self.fs = fs_
        self.profenv = ' '.join(["PERFMON_LEVEL=%s" %
                                 os.environ.get('PERFMON_LEVEL', "x"),
                                 "PERFMON_LDIR=%s"  %
                                 os.environ.get('PERFMON_LDIR',  "x"),
                                 "PERFMON_LFILE=%s" %
                                 os.environ.get('PERFMON_LFILE', "x")])
        self.perf_msg = None
        self.video_read_msg = None
        self.video_write_msg = None
        self.delegation_sockets = int(delegation_sockets_)
        self.delegation_threads = int(delegation_threads_)
        self.delegate = bool(int(delegate_));
        self.rcore = bool(int(rcore_))

        self.numjobs = self.calc_num_jobs(self.ncore, self.rcore,
                                          self.delegation_threads,
                                          self.delegation_sockets)


        if (fs_ == "sufs"):
            self.bin = FileBench.FILEBENCH_SUFS_BIN
        elif (fs_ == "sufs-kv"):
            self.bin = FileBench.FILEBENCH_KV_SUFS_BIN
        elif self.workload == "varmail-fd" or self.workload == "webproxy-fd":
            if (fs_ == "sufs"):
                self.bin = FileBench.FILEBENCH_FD_SUFS_BIN
            elif (fs_ == "sufs-fd"):
                self.bin = FileBench.FILEBENCH_FD_SUFS_FD_BIN
            else:
                self.bin = FileBench.FILEBENCH_FD_BIN
        else:
            self.bin = FileBench.FILEBENCH_BIN

    def __del__(self):
        # clean up
        # try:
        #     if self.config:
        #         os.unlink(self.config.name)
        #     if self.bench_out:
        #         os.unlink(self.bench_out.name)
        # except:
        #     pass
        return

    def run(self):
        # set up benchmark configuration
        if not self.generate_config():
            return -1
        # run pre-script then sync
        self._exec_cmd("sudo %s; sync" % FileBench.PRE_SCRIPT).wait()
        # start performance profiling
        self._exec_cmd("%s %s" % (self.profenv, self.profbegin)).wait()
        # run filebench
        self._run_filebench()
        # stop performance profiling
        self._exec_cmd("%s %s" % (self.profenv, self.profend)).wait()
        return 0

    def _get_cpu_ranges(self):
        ret = ' '
        core_per_socket = cpupol.CORE_PER_CHIP;

        for i in range(1, int(self.nsocket) + 1):
            begin = core_per_socket * (i - 1) + int(self.delegation_threads);
            end = core_per_socket * i - 1;
            if (i > 1):
                ret = ret + ',';

            ret = ret + str(begin) + '-' + str(end)

        return ret;

    def filebench_run_env(self):
        env_cmd = ''

        if self.fs == "sufs" or self.fs == "sufs-kv" or self.fs == "sufs-fd":
            env_cmd = ' '.join(["sufs_alloc_cpu=%s" % (self.nsocket * cpupol.CORE_PER_CHIP),
            "sufs_alloc_numa=%s" % self.delegation_sockets,
            "sufs_init_alloc_size=%s" % "8192",
            "sufs_alloc_pin_cpu=%s" % 0])
        elif self.fs == "splitfs":
            env_cmd = (splitfs.ledge_str + " " +  
            ("NVP_TREE_FILE=%s " % (os.path.normpath(os.path.join(CUR_DIR, splitfs.tree))) 
                + 
            ("LD_LIBRARY_PATH=%s " % os.path.normpath(CUR_DIR))
            + ("LD_PRELOAD=%s "  %  os.path.normpath(os.path.join(CUR_DIR, splitfs.lib))))) 

        return env_cmd

    def _run_filebench(self):

        env_cmd = self.filebench_run_env()

        taskset_cmd= ' '
        if (self.delegate):
            taskset_cmd = 'taskset -a -c ' + self._get_cpu_ranges()

        with tempfile.NamedTemporaryFile(delete=False) as self.bench_out:
            cmd = "sudo %s %s %s -f %s" % (env_cmd, taskset_cmd, self.bin,
                                        self.config.name)
            
            p = self._exec_cmd(cmd, subprocess.PIPE)   

            status = p.wait(timeout=600)

            if (status !=0):
                return 

                
            for l in p.stdout.readlines():
                self.bench_out.write("#@ ".encode("utf-8"))
                self.bench_out.write(l)
                l_str = str(l)
                if self.workload == "videoserver":
                    find_str = FileBench.VIDEO_WRITE_STR if self.video_read_msg else FileBench.VIDEO_READ_STR
                    idx = l_str.find(find_str)
                    if idx != -1:
                        if self.video_read_msg:
                            self.video_write_msg = l_str[idx+len(find_str):]
                        else:
                            self.video_read_msg = l_str[idx+len(find_str):]
                else:
                    idx = l_str.find(FileBench.PERF_STR)
                    if idx != -1:
                        self.perf_msg = l_str[idx+len(FileBench.PERF_STR):]

            self.bench_out.flush()

    def report(self):
        if self.workload == "videoserver":
            self.report_videoserver()
        else:
            self.report_filebench()

    def report_videoserver(self):
        read_bw, write_bw = 0, 0
        unit = 'mb/s'
        for item in self.video_read_msg.split(','):
            vk = item.strip().split()
            if vk[2].endswith(unit):
                read_bw = vk[2][:-len(unit)]
        for item in self.video_write_msg.split(','):
            vk = item.strip().split()
            if vk[2].endswith(unit):
                write_bw = vk[2][:-len(unit)]
        profile_name = ""
        profile_data = ""
        try:
            with open(self.proflog, "r") as fpl:
                l = fpl.readlines()
                if len(l) >= 2:
                    profile_name = l[0]
                    profile_data = l[1]
        except:
            pass
        print("# ncpu secs read_bw write_bw %s" % profile_name)
        print("%s %s %s %s %s" %
              (self.ncore, self.duration, read_bw, write_bw, profile_data))

    def report_filebench(self):
        # 32.027: IO Summary: 9524462 ops 317409.718 ops/s 28855/57715 rd/wr 7608.5mb/s 0.470ms/op
        work = 0
        work_sec = 0
        for item in self.perf_msg.split(','):
            # hard-code for now...
            vk = item.strip().split()
            if vk[1] == "ops":
                work = vk[0]
            if vk[3] == "ops/s":
                work_sec = vk[2]
        profile_name = ""
        profile_data = ""
        try:
            with open(self.proflog, "r") as fpl:
                l = fpl.readlines()
                if len(l) >= 2:
                    profile_name = l[0]
                    profile_data = l[1]
        except:
            pass
        
        print("# ncpu secs works works/sec %s" % profile_name)
        print("%s %s %s %s %s" %
              (self.ncore, self.duration, work, work_sec, profile_data))

    def generate_config(self):
        # check config template
        if (self.workload != "varmail"  and self.workload != "varmail-fd" and
            self.workload != "webproxy" and self.workload != "webproxy-fd"):
                config_template = os.path.normpath(os.path.join(FileBench.WORKLOAD_DIR,
                                                                self.workload + ".f"))
                if not os.path.isfile(config_template):
                    return False

        # create a configured workload file
        self.config = tempfile.NamedTemporaryFile(delete=False)
        self.config.write(b'# auto generated by fxmark\n')
        self.config.close()

        self.setup_workload_start()

        if self.workload == "varmail" or self.workload == "varmail-fd":
            self.setup_workload_varmail()
        elif self.workload == "webproxy" or self.workload == "webproxy-fd":
            self.setup_workload_webproxy()
        else:
            self._exec_cmd("cat %s >> %s" % (config_template, self.config.name)).wait()

        self.setup_workload_end()
        return True

    def setup_workload_varmail_fileset(self):
        for i in range(self.numjobs):
            n = i + 1
            self._append_to_config(self.VARMAIL_FILESET_TEMP % n)

    def setup_workload_webproxy_fileset(self):
        for i in range(self.numjobs):
            n = i + 1
            self._append_to_config(self.WEBPROXY_FILESET_TEMP % n)

    def setup_workload_varmail_threads(self):

        if self.fs == "sufs-kv":
            iter = self.VARMAIL_KVS_THREAD_ARG_CNT
            templ = self.VARMAIL_KVS_THREAD_TEMP
        else:
            iter = self.VARMAIL_THREAD_ARG_CNT
            templ = self.VARMAIL_THREAD_TEMP

        for i in range(self.numjobs):
            n = i + 1
            tn = (n,) * iter
            self._append_to_config("  thread name=filereaderthread,memsize=10m,instances=1")
            self._append_to_config("  {")
            self._append_to_config(templ % tn)
            self._append_to_config("  }\n")

    def setup_workload_webproxy_threads(self):

        if self.fs == "sufs-kv":
            iter = self.WEBPROXY_KVS_THREAD_ARG_CNT
            templ = self.WEBPROXY_KVS_THREAD_TEMP
        else:
            iter = self.WEBPROXY_THREAD_ARG_CNT
            templ = self.WEBPROXY_THREAD_TEMP

        for i in range(self.numjobs):
            n = i + 1
            tn = (n,) * iter
            self._append_to_config("  thread name=proxycache,memsize=10m,instances=1")
            self._append_to_config("  {")
            self._append_to_config(templ % tn)
            self._append_to_config("  }\n")

    def setup_workload_varmail(self):
        self._append_to_config("set $nfiles = %d" %(self.VARMAIL_FILE_SIZE / self.numjobs))

        if self.workload == "varmail-fd":
            self._append_to_config("set $meandirwidth=2")
        else:
            self._append_to_config("set $meandirwidth=1000000")

        self._append_to_config("set $filesize=cvar(type=cvar-gamma,parameters=mean:16384;gamma:1.5)")
        self._append_to_config("set $iosize=1m")
        self._append_to_config("set $meanappendsize=16k\n")

        self.setup_workload_varmail_fileset()
        self._append_to_config("\ndefine process name=filereader,instances=1")
        self._append_to_config("{")
        self.setup_workload_varmail_threads()
        self._append_to_config("}")
        self._append_to_config('echo  "Varmail Version 3.0 personality successfully loaded"')

    def setup_workload_webproxy(self):

        if self.workload == "webproxy-fd":
            self._append_to_config("set $nfiles=%d" %(self.WEBPROXY_FD_FILE_SIZE / self.numjobs))
        else:
            self._append_to_config("set $nfiles=%d" %(self.WEBPROXY_FILE_SIZE / self.numjobs))

        if self.workload == "webproxy-fd":
            self._append_to_config("set $meandirwidth=2")
        else:
            self._append_to_config("set $meandirwidth=1000000")

        self._append_to_config("set $meanfilesize=16k")
        self._append_to_config("set $meaniosize=16k")
        self._append_to_config("set $iosize=1m\n")

        self.setup_workload_webproxy_fileset()
        self._append_to_config("\ndefine process name=proxycache,instances=1")
        self._append_to_config("{")
        self.setup_workload_webproxy_threads()
        self._append_to_config("}")
        self._append_to_config('echo  "Web proxy-server Version 3.0 personality successfully loaded"')


    def setup_workload_start(self):
        # config number of workers
        if self.workload == "fileserver":
            self._append_to_config("set $nthreads=%d"   % (self.numjobs))
        elif self.workload == "varmail" or self.workload == "varmail-fd":
            self._append_to_config("set $nthreads=%d"   % (self.numjobs))
        elif self.workload == "videoserver":
            # make the number of write threads the same as the number of sockets
            # we use. So with less or equal than 28 cores, I have 1 writer
            # thread. And then, the number of writer threads increase as we
            # increase the number of sockets
            wthreads = self.nsocket
            rthreads = self.numjobs - wthreads
            self._append_to_config("set $wthreads=%d"   % (wthreads))
            self._append_to_config("set $rthreads=%d"   % (rthreads))
        elif self.workload == "webserver":
            self._append_to_config("set $nthreads=%d"   % (self.numjobs))
        elif self.workload == "webproxy" or self.workload == "webproxy-fd":
            self._append_to_config("set $nthreads=%d"   % (self.numjobs))
        else:
            return False
        # config target dir and benchmark time
        self._append_to_config("set $dir=%s"            % self.root)
        return True

    def setup_workload_end(self):
        self._append_to_config("run %d"                 % self.duration)
        return True

    def _append_to_config(self, config_str):
        self._exec_cmd("echo \'%s\' >> %s" % (config_str, self.config.name)).wait()

    def _exec_cmd(self, cmd, out=None):
        p = subprocess.Popen(cmd, shell=True, stdout=out, stderr=out)
        return p

if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--type", help="workload name")
    parser.add_option("--ncore", help="number of core")
    parser.add_option("--nsocket", help="number of socket")
    parser.add_option("--duration", help="benchmark time in seconds")
    parser.add_option("--root", help="benchmark root directory")
    parser.add_option("--profbegin", help="profile begin command")
    parser.add_option("--profend", help="profile end command")
    parser.add_option("--proflog", help="profile log path")
    parser.add_option("--fs", help="file system to run")
    parser.add_option("--delegation_threads", help="delegation per socket")
    parser.add_option("--delegation_sockets", help="sockets on which delegation threads will run")
    parser.add_option("--delegate", help="reserve CPUs for delegation threads"
                                            " or not")
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
    filebench = FileBench(opts.type, opts.ncore, opts.nsocket, opts.duration,
                          opts.root, opts.profbegin, opts.profend, opts.proflog,
                          opts.fs, opts.delegation_threads,
                          opts.delegation_sockets, opts.delegate, opts.rcore)
    rc = filebench.run()
    filebench.report()
    exit(rc)

