#!/usr/bin/env python3
import os
import sys
import signal
import subprocess
import datetime
import tempfile
import pdb
from os.path import join
from perfmon import PerfMon
import time
import random
import re
import optparse
import psutil
import strata

CUR_DIR = os.path.abspath(os.path.dirname(__file__))
FXMARK_DIR = os.path.dirname(CUR_DIR)
BENCHMARK_DIR = os.path.dirname(FXMARK_DIR)
SRC_DIR = os.path.dirname(BENCHMARK_DIR)
ROOT_DIR = os.path.dirname(SRC_DIR)

try:
    import cpupol
except ImportError:
    print("No cpupolicy for this machine.")
    print("Do \'make\' at %s\n" %
          os.path.normpath(os.path.join(CUR_DIR, "..")))
    raise


def catch_ctrl_C(sig, frame):
    print("Umount a testing file system. Please wait.")


class Runner(object):
    # media path
    LOOPDEV = "/dev/loopX"
    NVMEDEV = "/dev/nvme0n1pX"
    HDDDEV = "/dev/sdX"
    SSDDEV = "/dev/sdY"
    PMEMDEV = "/dev/pmem0"
    PMEMCHAR = "/dev/dax0.0"
    DMSTRIPEDEV = "/dev/mapper/dm-stripe"
    PMARRAYDEV = "/dev/pmem_ar0"
    PMARRAYCHARDEV = "/dev/supremefs"
    SUFS_MODULE_PATH = "/lib/modules/5.13.13/sufs.ko"
    # test core granularity
    CORE_FINE_GRAIN = 0
    CORE_COARSE_GRAIN = 1

    def __init__(self,
                 core_grain=CORE_COARSE_GRAIN,
                 pfm_lvl=PerfMon.LEVEL_LOW,
                 duration=30,
                 directory_name='',
                 log_name='',
                 run_filter=("*", "*", "*", "*", "*", 0, 0, False, False)):
        # run config
        self.CORE_GRAIN = core_grain
        self.PERFMON_LEVEL = pfm_lvl
        self.directory_name = directory_name
        self.log_name = log_name
        self.FILTER = run_filter  # media, fs, bench, ncore, directio
        self.DRYRUN = False
        self.DEBUG_OUT = False

        # bench config
        self.DISK_SIZE = "32G"

        # enable directio except tmpfs -> nodirectio
        self.DIRECTIOS = ["bufferedio", "directio"]
        self.MEDIA_TYPES = [
            "ssd",
            "hdd",
            "nvme",
            "mem",
            "pmem-local",
            "pm-char",
            "pmem-remote-1-hop",
            "pmem-remote-2-hop",
            "dm-stripe",
            "dm-stripe-1-local",
            "dm-stripe-1-remote",
            "pm-array",
            "pm-char-array",
        ]
        self.FS_TYPES = [
            "tmpfs",
            "ext4",
            "ext4-no-jnl",
            "xfs",
            "btrfs",
            "f2fs",
            "nova",
            "splitfs",
            "strata",
            "pmfs",
            "ext2",
            "ext3",
            "odinfs",
            "winefs",
            "sufs",
            "sufs-kv",
            "sufs-fd"
        ]
        self.MKFS_TYPES = {
            "ext2-no-jnl": "ext2",
            "ext3-no-jnl": "ext3",
            "ext4-no-jnl": "ext4",
        }
        self.pmem_dm_stripe_num = self.FILTER[6]
        self.PMEM_DM_STRIPE_CHUNK_SIZE = 4096
        self.PMEM_DEVS = ['/dev/pmem0', '/dev/pmem1', '/dev/pmem2', '/dev/pmem3',
                          '/dev/pmem4', '/dev/pmem5', '/dev/pmem6', '/dev/pmem7']
        self.PARRADM_BIN = os.path.join("parradm")
        self.SUFS_INIT_BIN = os.path.join("init-sufs")

        self.STRATA_INIT_BIN = os.path.join("mkfs.mlfs")
        self.STRATA_LD_LIB_PATH = "%s:%s" % (
            os.path.normpath(os.path.join(CUR_DIR, "strata_build")), 
            os.path.normpath(os.path.join(CUR_DIR, "nvml_lib")))
        self.STRATA_KFS_PRELOAD = (
            os.path.normpath(os.path.join(CUR_DIR, "libjemalloc.so.2")))
        self.STRATA_KFS_BIN = (
            os.path.normpath(os.path.join(CUR_DIR, "strata_kfs")))

        # make sure these pmem are one hop from each other
        self.PMEM_STRIPE_CONFIGS = {
                1: ["/dev/pmem0"],
                2: ["/dev/pmem0", "/dev/pmem1"],
                4: ["/dev/pmem0", "/dev/pmem1", "/dev/pmem2", "/dev/pmem3"],
                8: ["/dev/pmem0", "/dev/pmem1", "/dev/pmem2", "/dev/pmem3",
                    "/dev/pmem4", "/dev/pmem5", "/dev/pmem6", "/dev/pmem7"],
        }
        self.NUMA_ONE_HOP_CONFIGS = {
            0: [1, 2, 4],
            1: [0, 3, 5],
            2: [0, 3, 6],
            3: [1, 2, 7],
            4: [0, 5, 7],
            5: [1, 4, 6],
            6: [2, 5, 7],
            7: [3, 4, 6],
        }
        self.NUMA_TWO_HOP_CONFIGS = {
            0: [3, 5, 6, 7],
            1: [2, 4, 6, 7],
            2: [1, 4, 5, 7],
            3: [0, 4, 5, 6],
            4: [1, 2, 3, 6],
            5: [0, 2, 3, 7],
            6: [0, 1, 3, 4],
            7: [0, 1, 2, 5]
        }
        self.BENCH_TYPES = [
            # write/write
            "DWAL",
            "DWOL",
            "DWOM",
            "DWSL",
            "MWRL",
            "MWRM",
            "MWCL",
            "MWCM",
            "MWUM",
            "MWUL",
            "DWTL",

            # filebench
            "filebench_varmail",
            "filebench_fileserver",
            "filebench_videoserver",
            "filebench_webserver",
            "filebench_webproxy",
            "filebench_webproxy-fd",
            "filebench_varmail-fd",

            # dbench
            "dbench_fill100K",
            "dbench_fillseq", 
            "dbench_fillsync",
            "dbench_fillrandom",
            "dbench_readrandom",
            "dbench_deleterandom",

            # fio: fio_[config]_[bench]
            "fio_global_seq-read-4K",
            "fio_global_seq-read-8K",
            "fio_global_seq-read-16K",
            "fio_global_seq-read-32K",
            "fio_global_seq-read-2M",
            "fio_global_seq-read-1G",
            # "fio_global_seq-read-4K-2M",
            "fio_global_seq-write-4K",
            "fio_global_seq-write-8K",
            "fio_global_seq-write-16K",
            "fio_global_seq-write-32K",
            "fio_global_seq-write-2M",
            "fio_global_seq-write-1G",
            # "fio_global_seq-write-4K-2M",
            "fio_global_rand-read-4K",
            "fio_global_rand-read-8K",
            "fio_global_rand-read-16K",
            "fio_global_rand-read-32K",
            "fio_global_rand-read-2M",
            "fio_global_rand-read-1G",
            # "fio_global_rand-read-4K-2M",
            "fio_global_rand-write-4K",
            "fio_global_rand-write-8K",
            "fio_global_rand-write-16K",
            "fio_global_rand-write-32K",
            "fio_global_rand-write-2M",
            "fio_global_rand-write-1G",
            # mixed workload
            "fio_global_seq-read-2M_seq-write-4K",
            "fio_global_seq-read-4K_seq-write-2M",

            # "fio_global_rand-write-4K-2M",
            # "fio_global_seq-read-write-50-50-4K",
            # "fio_global_seq-read-write-50-50-2M",
            # "fio_global_seq-read-write-50-50-4K-2M",
            # "fio_global_seq-read-write-90-10-4K",
            # "fio_global_seq-read-write-90-10-2M",
            # "fio_global_seq-read-write-90-10-4K-2M",
            # "fio_global_rand-read-write-50-50-4K",
            # "fio_global_rand-read-write-50-50-2M",
            # "fio_global_rand-read-write-50-50-4K-2M",
            # "fio_global_rand-read-write-90-10-4K",
            # "fio_global_rand-read-write-90-10-2M",
            # "fio_global_rand-read-write-90-10-4K-2M",

            # read/read
            "MRPL",
            "MRPM",
            "MRPH",
            "MRDM",
            "MRDL",
            "DRBH",
            "DRBM",
            "DRBL",

            # read/write
            # "MRPM_bg",
            # "DRBM_bg",
            # "MRDM_bg",
            # "DRBH_bg",
            # "DRBL_bg",
            # "MRDL_bg",
        ]
        self.BENCH_BG_SFX = "_bg"

        # path config
        self.ROOT_NAME        = "root"
        self.LOGD_NAME        = "../logs"
        self.FXMARK_NAME      = "fxmark"
        self.FXMARK_NAME_SUFS = "fxmark-sufs"
        self.FILEBENCH_NAME   = "run-filebench.py"
        self.FIO_NAME         = "run-fio.py"
        self.DBENCH_NAME      = "run-dbench.py"
        self.PERFMN_NAME      = "perfmon.py"
        self.PID_NAME         = "pid.txt"
        self.LD_LIBRARY_PATH  = "../splitfs/splitfs/"
        self.NVP_TREE_FILE    = "../splitfs/splitfs/bin/nvp_nvp.tree"
        self.LD_PRELOAD       = "../splitfs/splitfs/libnvp.so"
        self.EXT_BLOCK_SIZE = 4096
        self.EXT_STRIDE_SIZE = 512
        self.LOCAL_CPU_NODE = int(self.PMEMDEV[-1])
        self.REMOTE_1_HOP_CPU_NODE = random.choice(self.NUMA_ONE_HOP_CONFIGS[self.LOCAL_CPU_NODE])
        self.REMOTE_2_HOP_CPU_NODE = random.choice(self.NUMA_TWO_HOP_CONFIGS[self.LOCAL_CPU_NODE])
        self.DELEGATION_THREADS = self.FILTER[5]
        self.DELEGATION_SOCKETS = self.FILTER[6]

        # fs config
        self.HOWTO_MOUNT = {
            "tmpfs": self.mount_tmpfs,
            "ext2": self.mount_anyfs,
            "ext3": self.mount_anyfs,
            "ext4": self.mount_anyfs,
            "ext4-no-jnl": self.mount_anyfs,
            "xfs": self.mount_anyfs,
            "btrfs": self.mount_anyfs,
            "f2fs": self.mount_anyfs,
            "jfs": self.mount_anyfs,
            "reiserfs": self.mount_anyfs,
            "nova": self.mount_nova,
            "splitfs": self.mount_splitfs,
            "strata": self.mount_strata,
            "pmfs": self.mount_pmfs,
            "winefs" : self.mount_winefs,
            "odinfs": self.mount_odinfs,
            "sufs": self.mount_sufs,
            "sufs-kv": self.mount_sufs,
            "sufs-fd": self.mount_sufs,
        }
        self.HOWTO_MKFS = {
            "ext2":
            f"-b {self.EXT_BLOCK_SIZE} -E stride={self.EXT_STRIDE_SIZE} -F",
            "ext3":
            f"-b {self.EXT_BLOCK_SIZE} -E stride={self.EXT_STRIDE_SIZE} -F",
            "ext4":
            f"-b {self.EXT_BLOCK_SIZE} -E stride={self.EXT_STRIDE_SIZE} -F",
            "ext4-no-jnl": "-F",
            "xfs": "-f",
            "btrfs": "-f",
            "jfs": "-q",
            "reiserfs": "-q",
        }

        # media config
        self.HOWTO_INIT_MEDIA = {
            "mem": self.init_mem_disk,
            "nvme": self.init_nvme_disk,
            "ssd": self.init_ssd_disk,
            "hdd": self.init_hdd_disk,
            "pmem-local": self.init_pmem_disk,
            "pm-char" : self.init_pmem_char,
            "dm-stripe": self.init_dm_stripe_disk,
            "pm-array": self.init_pm_array,
            "pm-char-array": self.init_pm_devdax_array,
        }

        # Need to further investigate
        self.BLACKLIST = [

        ]

        # misc. setup
        self.redirect = subprocess.PIPE if not self.DEBUG_OUT else None
        self.dev_null = open("/dev/null", "a") if not self.DEBUG_OUT else None
        self.npcpu = cpupol.PHYSICAL_CHIPS * cpupol.CORE_PER_CHIP
        self.nhwthr = self.npcpu * cpupol.SMT_LEVEL
        self.rcore = self.FILTER[7]
        self.delegate = self.FILTER[8]
        self.ncores = self.get_ncores(self.FILTER[3])
        self.duration = duration

        self.test_root = os.path.normpath(
            os.path.join(CUR_DIR, self.ROOT_NAME))

        self.filebench_path = os.path.normpath(
            os.path.join(CUR_DIR, self.FILEBENCH_NAME))
        self.dbench_path = os.path.normpath(
            os.path.join(CUR_DIR, self.DBENCH_NAME))
        self.fio_path = os.path.normpath(
            os.path.join(CUR_DIR, self.FIO_NAME))
        self.tmp_path = os.path.normpath(
            os.path.join(CUR_DIR, ".tmp"))
        self.disk_path = os.path.normpath(
            os.path.join(self.tmp_path, "disk.img"))
        self.perfmon_start = "%s start" % os.path.normpath(
            os.path.join(CUR_DIR, self.PERFMN_NAME))
        self.perfmon_stop = "%s stop" % os.path.normpath(
            os.path.join(CUR_DIR, self.PERFMN_NAME))
        self.perfmon_log = ""
        self.log_dir     = ""
        self.log_path    = ""
        self.profile_pid_file = "%s" % os.path.normpath(
            os.path.join(CUR_DIR, self.PID_NAME))
        self.umount_hook = []
        self.umount_dm_stripe = 0
        self.active_ncore = -1
        signal.signal(signal.SIGUSR1, self.start_profile_handler)
        signal.signal(signal.SIGUSR2, self.end_profile_handler)

        if os.path.exists(self.profile_pid_file):
            os.remove(self.profile_pid_file)

    def pmem_1_hop(self, rand = False):
        conf = self.NUMA_ONE_HOP_CONFIGS[pmem_node()]
        if rand:
            return random.choice(conf)
        return conf[0]

    def pmem_2_hop(self, rand = False):
        conf = self.NUMA_TWO_HOP_CONFIGS[pmem_node()]
        if rand:
            return random.choice(conf)
        return conf[0]

    def log_start(self):

        directory_name = self.directory_name
        if (directory_name == ''):
            directory_name = str(datetime.datetime.now()).replace(' ', '-').replace(':', '-');

        log_name = self.log_name
        if (log_name == ''):
            log_name = 'fxmark.log'

        self.log_dir = os.path.normpath(
            os.path.join(CUR_DIR, self.LOGD_NAME, directory_name))
        self.log_path = os.path.normpath(
            os.path.join(self.log_dir, log_name))
        self.exec_cmd("mkdir -p " + self.log_dir, self.dev_null)

        self.log_fd = open(self.log_path, "bw")
        p = self.exec_cmd(
            "echo -n \"### SYSTEM         = \"; uname -a", self.redirect)
        if self.redirect:
            for l in p.stdout.readlines():
                self.log(l.decode("utf-8").strip())
        self.log("### DISK_SIZE      = %s" % self.DISK_SIZE)
        self.log("### DURATION       = %ss" % self.duration)
        self.log("### DIRECTIO       = %s" % ','.join(self.DIRECTIOS))
        self.log("### MEDIA_TYPES    = %s" % ','.join(self.MEDIA_TYPES))
        self.log("### FS_TYPES       = %s" % ','.join(self.FS_TYPES))
        self.log("### BENCH_TYPES    = %s" % ','.join(self.BENCH_TYPES))
        self.log("### NCORES         = %s" %
                 ','.join(map(lambda c: str(c), self.ncores)))
        self.log("### CORE_SEQ       = %s" %
                 ','.join(map(lambda c: str(c), cpupol.seq_cores)))
        self.log("\n")
        self.log("### MODEL_NAME     = %s" % cpupol.MODEL_NAME)
        self.log("### PHYSICAL_CHIPS = %s" % cpupol.PHYSICAL_CHIPS)
        self.log("### CORE_PER_CHIP  = %s" % cpupol.CORE_PER_CHIP)
        self.log("### SMT_LEVEL      = %s" % cpupol.SMT_LEVEL)
        self.log("\n")

    def log_end(self):
        self.log_fd.close()

    def log(self, log):
        self.log_fd.write((log+'\n').encode('utf-8'))
        print(log)

    def get_ncores(self, hint):
        # if user specifies the specific number of cores he/she wants to run,
        # stop auto generate the core numbers
        if hint.isdigit():
            hint_core = int(hint)
            if hint_core <= 0 or hint_core > self.npcpu:
                print("Invalid ncore hint", file=sys.stderr)
            return [hint_core]

        ncores = []
        for socket in range(1, cpupol.PHYSICAL_CHIPS + 1):
            if socket == 1:
                n = 0
                while (2 ** n) < cpupol.CORE_PER_CHIP:
                    ncores.append((2 ** n, socket))
                    n += 1
                ncores.append((cpupol.CORE_PER_CHIP, socket))
            else:
                ncores.append((socket * (cpupol.CORE_PER_CHIP), socket))

        ncores.sort()
        return ncores

    def exec_cmd(self, cmd, out=None, wait=True, timeout=0, fn=None):
        p = subprocess.Popen(cmd, shell=True, stdout=out, stderr=out, 
                             preexec_fn=fn)
        if wait:
            p.wait()
        else:
            time.sleep(timeout)
            p.poll()

        return p

    def keep_sudo(self):
        self.exec_cmd("sudo -v", self.dev_null)

    def drop_caches(self):
        cmd = ' '.join(["sudo",
                        os.path.normpath(
                            os.path.join(CUR_DIR, "drop-caches"))])
        self.exec_cmd(cmd, self.dev_null)

    def set_cpus(self, ncore):
        if self.active_ncore == ncore:
            return
        self.active_ncore = ncore
        if ncore == 0:
            ncores = "all"
        else:
            ncores = ','.join(map(lambda c: str(c), cpupol.seq_cores[0:ncore]))
        cmd = ' '.join(["sudo",
                        os.path.normpath(
                            os.path.join(CUR_DIR, "set-cpus")),
                        ncores])
        self.exec_cmd(cmd, self.dev_null)

    def add_bg_worker_if_needed(self, bench, ncore):
        if bench.endswith(self.BENCH_BG_SFX):
            ncore = min(ncore + 1, self.nhwthr)
            return (ncore, 1)
        return (ncore, 0)

    def prepre_work(self, ncore):
        self.keep_sudo()
        self.exec_cmd("sudo sh -c \"echo 0 >/proc/sys/kernel/lock_stat\"",
                      self.dev_null)

        self.exec_cmd("sudo sh -c \"echo 262144 >/proc/sys/vm/max_map_count\"",
                      self.dev_null)

        self.drop_caches()
        self.exec_cmd("sync", self.dev_null)
        # self.set_cpus(ncore)

    def pre_work(self):
        self.keep_sudo()
        self.drop_caches()

    def post_work(self, fs):
        self.keep_sudo()

        if (fs == "strata"):
            self.umount_strata()

    def unset_loopdev(self):
        self.exec_cmd(' '.join(["sudo", "losetup", "-d", Runner.LOOPDEV]),
                      self.dev_null)

    def init_pm(self, num, mode):
        if mode == "fsdax":
            ptemp = "/dev/pmem%s"
        elif mode == "devdax":
            ptemp = "/dev/dax%s.0"
        else:
            sys.exit("Unknown mode in init_pm")

        for i in range(num):
            path = ptemp % (i)

            if (os.path.exists(path) == False):
                cmd = "sudo ndctl create-namespace -f -e namespace%s.0 --mode=%s" % (i, mode)
                self.exec_cmd(cmd)


    def umount(self, where):
        while True:
            time.sleep(1)
            p = self.exec_cmd("sudo umount " + where, self.dev_null)
            if p.returncode != 0:
                break
        (umount_hook, self.umount_hook) = (self.umount_hook, [])
        map(lambda hook: hook(), umount_hook)

        if self.umount_dm_stripe:
            self.deinit_dm_stripe_disk()

    def init_mem_disk(self):
        self.unset_loopdev()
        self.umount(self.tmp_path)
        self.unset_loopdev()
        self.exec_cmd("mkdir -p " + self.tmp_path, self.dev_null)
        if not self.mount_tmpfs("mem", "tmpfs", self.tmp_path):
            return False
        self.exec_cmd("dd if=/dev/zero of="
                      + self.disk_path + " bs=1G count=1024000",
                      self.dev_null)
        p = self.exec_cmd(' '.join(["sudo", "losetup",
                                    Runner.LOOPDEV, self.disk_path]),
                          self.dev_null)
        if p.returncode == 0:
            self.umount_hook.append(self.deinit_mem_disk)
        return (p.returncode == 0, Runner.LOOPDEV)

    def deinit_mem_disk(self):
        self.unset_loopdev()
        self.umount(self.tmp_path)

    def init_nvme_disk(self):
        return (os.path.exists(Runner.NVMEDEV), Runner.NVMEDEV)

    def init_ssd_disk(self):
        return (os.path.exists(Runner.SSDDEV), Runner.SSDDEV)

    def init_pmem_disk(self):
        self.init_pm(1, "fsdax")
        return (os.path.exists(Runner.PMEMDEV), Runner.PMEMDEV)
    
    def init_pmem_char(self):
        self.init_pm(1, "devdax")
        return (os.path.exists(Runner.PMEMCHAR), Runner.PMEMCHAR)

    def init_dm_stripe_disk(self):
        self.init_pm(self.pmem_dm_stripe_num, "fsdax")
        # create the pm stripe
        if self.pmem_dm_stripe_num not in self.PMEM_STRIPE_CONFIGS:
            return False
        stripe_length = 0
        stripe_config = self.PMEM_STRIPE_CONFIGS[self.pmem_dm_stripe_num]
        devices = ""

        for device in stripe_config:
            # TODO: make exec_cmd better
            stripe_size = subprocess.run(["sudo", "blockdev", "--getsz", device],
                                         check=True,
                                         stdout=subprocess.PIPE,
                                         universal_newlines=True)
            stripe_length += int(stripe_size.stdout)
            devices += ' '.join([device, '0']) + ' '

        # config format: start length striped #stripes chunk_size
        #                device1 offset1 ... deviceN offsetN
        stripe_config = f'0 {stripe_length} striped {self.pmem_dm_stripe_num} {self.PMEM_DM_STRIPE_CHUNK_SIZE} {devices}'

        p = self.exec_cmd("echo {} | sudo dmsetup create {}".format(
            stripe_config, os.path.basename(self.DMSTRIPEDEV)))

        # TODO: double check if the unmount hook is invoked
        # It seems that umount hook is not working
        # self.umount_hook.append(self.deinit_dm_stripe_disk)

        self.umount_dm_stripe = True

        return (p.returncode == 0, Runner.DMSTRIPEDEV)

    def init_pm_array(self):

        self.init_pm(self.DELEGATION_SOCKETS, "fsdax")

        pmem_devs = self.PMEM_DEVS[:self.DELEGATION_SOCKETS]

        cmd = 'sudo {} create {}'.format(self.PARRADM_BIN, ' '.join(pmem_devs))
        p = self.exec_cmd(cmd)
        return (p.returncode == 0, Runner.PMARRAYDEV)

    def deinit_dm_stripe_disk(self):
        p = self.exec_cmd("sudo dmsetup remove {}".format(
            os.path.basename(self.DMSTRIPEDEV)))
        return p.returncode == 0

    def init_hdd_disk(self):
        return (os.path.exists(Runner.HDDDEV), Runner.HDDDEV)


    def init_pm_devdax_array(self):
        self.init_pm(self.DELEGATION_SOCKETS, "devdax")
        return (os.path.exists(Runner.PMARRAYCHARDEV), Runner.PMARRAYCHARDEV)


    def init_media(self, media):
        _init_media = self.HOWTO_INIT_MEDIA.get(media, None)
        if not _init_media:
            return (False, None)

        (rc, dev_path) = _init_media()
        return (rc, dev_path)

    def mount_tmpfs(self, media, fs, mnt_path):
        p = self.exec_cmd("sudo mount -t tmpfs -o mode=0777,size="
                          + self.DISK_SIZE + " none " + mnt_path,
                          self.dev_null)
        return p.returncode == 0

    def mount_nova(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        if not rc:
            return False
        p = self.exec_cmd(
            ' '.join(["sudo mount -t NOVA -o init", dev_path, mnt_path]), self.dev_null)
        if p.returncode != 0:
            return False
        p = self.exec_cmd("sudo chmod 777 " + mnt_path,
                          self.dev_null)
        if p.returncode != 0:
            return False
        return True

    def mount_strata(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        if not rc:
            return False

        p = self.exec_cmd("sudo numactl -N0 -m0 " + self.STRATA_INIT_BIN + " 1")
        if p.returncode != 0:
            return False
        
        p = self.exec_cmd(("sudo LD_LIBRARY_PATH=%s LD_PRELOAD=%s MLFS_PROFILE=1 "
                           "numactl -N0 -m0 %s") 
                          % (self.STRATA_LD_LIB_PATH, self.STRATA_KFS_PRELOAD, 
                            self.STRATA_KFS_BIN), wait=False, timeout=5)
        
        self.strata_kfs = p
        if p.returncode != None and p.returncode != 0:
            return False
        
        return True

    def mount_splitfs(self, media, fs, mnt_path):
        # For splitfs, it only works when it is mounted at /mnt/pmem_emul
        if mnt_path != "/mnt/pmem_emul":
            return False
        if not self.mount_anyfs(media, "ext4", mnt_path):
            return False
        return True

    def get_fs(self, fs):
        return self.MKFS_TYPES.get(fs, fs)

    def mount_anyfs(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        if not rc:
            return False

        p = self.exec_cmd(
            "sudo mkfs." + self.get_fs(fs) + " " +
            self.HOWTO_MKFS.get(fs, "") + " " + dev_path, self.dev_null)
        if p.returncode != 0:
            return False

        if fs.endswith("-no-jnl"):
            p = self.exec_cmd("sudo tune2fs -O ^has_journal %s" % dev_path,
                              self.dev_null)
            if p.returncode != 0:
                return False

        if fs.startswith("ext"):
            cmd = ' '.join(["sudo mount -o dax -t", self.get_fs(fs), dev_path, mnt_path])
        else:
            cmd = ' '.join(["sudo mount -t", self.get_fs(fs), dev_path, mnt_path])
        p = self.exec_cmd(cmd, self.dev_null)
        if p.returncode != 0:
            return False
        p = self.exec_cmd("sudo chmod 777 " + mnt_path)
        if p.returncode != 0:
            return False
        return True

    def mount_odinfs(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        cmd = "sudo mount -o init,dele_thrds={} -t {} {} {}".format(self.DELEGATION_THREADS, fs, dev_path, mnt_path)
        p = self.exec_cmd(cmd, self.dev_null)
        if p.returncode != 0:
            return False
        p = self.exec_cmd("sudo chmod 777 " + mnt_path)
        if p.returncode != 0:
            return False
        return True

    def mount_pmfs(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        cmd = ' '.join(["sudo mount -o init -t", fs, dev_path, mnt_path])
        p = self.exec_cmd(cmd, self.dev_null)
        if p.returncode != 0:
            return False
        p = self.exec_cmd("sudo chmod 777 " + mnt_path)
        if p.returncode != 0:
            return False
        return True

    def mount_winefs(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)
        cmd = ' '.join(["sudo mount -o init -t", fs, dev_path, mnt_path])
        p = self.exec_cmd(cmd, self.dev_null)
        if p.returncode != 0:
            return False
        p = self.exec_cmd("sudo chmod 777 " + mnt_path)
        if p.returncode != 0:
            return False
        return True

    def mount_sufs(self, media, fs, mnt_path):
        (rc, dev_path) = self.init_media(media)

        #It is fine if this fails
        self.exec_cmd("sudo rmmod " + Runner.SUFS_MODULE_PATH)

        cmd = ' '.join(["sudo insmod", Runner.SUFS_MODULE_PATH,
                        "sufs_kfs_dele_thrds=" + str(self.DELEGATION_THREADS),
                        "pm_nr=" + str(self.DELEGATION_SOCKETS)])

        p = self.exec_cmd(cmd)
        if p.returncode != 0:
            return False

        p = self.exec_cmd("sudo chmod 777 " + dev_path)
        if p.returncode != 0:
            return False

        p = self.exec_cmd("sudo " + self.SUFS_INIT_BIN)
        if p.returncode != 0:
            return False

        return True

    def umount_sufs(self):
        p = self.exec_cmd("sudo rmmod " + Runner.SUFS_MODULE_PATH)
        if p.returncode != 0:
            return False

        return True

    def umount_strata(self):
        process = psutil.Process(self.strata_kfs.pid)
        for proc in process.children(recursive=True):
            try:
                self.exec_cmd("sudo kill -9 %s" % (str(proc.pid)))
            except:
                pass

        try:
            self.exec_cmd("sudo kill -9 %s" % (str(process.pid)))
        except:
            pass
        
        return True

    def mount(self, media, fs, mnt_path):
        mount_fn = self.HOWTO_MOUNT.get(fs, None)
        if not mount_fn:
            return False

        if fs != "sufs" and fs != "sufs-kv" and fs != "sufs-fd" and fs != "strata":
            self.umount(mnt_path)
            self.exec_cmd("mkdir -p " + mnt_path, self.dev_null)

        return mount_fn(media, fs, mnt_path)

    def _match_config(self, key1, key2):
        for (k1, k2) in zip(key1, key2):
            if k1 == "*" or k2 == "*":
                continue
            if re.match(k1, k2):
                continue
            if k1 != k2:
                return False
        return True

    def in_blacklist(self, bench, media, fs):
        return (bench, media, fs) in self.BLACKLIST

    def gen_config(self):
        for ncore in sorted(self.ncores, key=lambda x: x[0], reverse=True):
            for bench in self.BENCH_TYPES:
                for media in self.MEDIA_TYPES:
                    for dio in self.DIRECTIOS:
                        for fs in self.FS_TYPES:
                            if fs == "tmpfs" and media != "mem":
                                continue
                            if self.in_blacklist(bench, media, fs):
                                continue
                            mount_fn = self.HOWTO_MOUNT.get(fs, None)
                            if not mount_fn:
                                continue
                            if self._match_config(self.FILTER,
                                                  (media, fs, bench, str(ncore[0]), dio)):
                                yield(media, fs, bench, ncore[0], ncore[1], dio)

    def fxmark_clean_env(self):
        env = ' '.join(["PERFMON_LEVEL=%s" % self.PERFMON_LEVEL,
                        "PERFMON_LDIR=%s" % self.log_dir,
                        "PERFMON_LFILE=%s" % self.perfmon_log])

        return env

    def check_path(self, path):
        if not os.path.exists(path):
            print("Please verify if {} exists".format(path))
            exit(1)

    def fxmark_run_env(self, fs, bench, type, ncore, nsocket, node):
        env = ' '.join(["PERFMON_LEVEL=%s" % self.PERFMON_LEVEL,
                        "PERFMON_LDIR=%s" % self.log_dir,
                        "PERFMON_LFILE=%s" % self.perfmon_log])
        
        fs_env = ' '
        
        if fs == "sufs":
            if bench == "fxmark":
                if type == "MWRL" or type == "MWRM" or type == "MWCL" or type == "MWCM":
                    fs_env = ' '.join(["sufs_alloc_cpu=%s" % ncore,
                                "sufs_alloc_numa=%s" % "-1",
                                "sufs_init_alloc_size=%s" % "65536",
                                "sufs_alloc_pin_cpu=%s" % 1])
                elif type == "DWAL":
                    fs_env = ' '.join(["sufs_alloc_cpu=%s" % ncore,
                                "sufs_alloc_numa=%s" % node,
                                "sufs_init_alloc_size=%s" % "65536",
                                "sufs_alloc_pin_cpu=%s" % 0])
        elif fs == "strata":
            if bench == "fxmark":
                    fs_env = ("LD_PRELOAD=%s" % 
                              (os.path.normpath(os.path.join(CUR_DIR, strata.lib))))


        env = env + ' ' + fs_env
        return env

    def get_bin_type(self, bench):
        if bench.startswith("filebench_"):
            return (self.filebench_path, bench[len("filebench_"):])
        if bench.startswith("dbench_"):
            return (self.dbench_path, bench[len("dbench_"):])
        if bench.startswith("fio_"):
            return (self.fio_path, bench[len("fio_"):])
        return (self.fxmark_path, bench)

    def start_profile_handler(self, sig, frame):
        pid = os.fork()
        if pid == 0:
            cmds = self.perfmon_start.split()
            os.execve(
                cmds[0], cmds, {
                    "PERFMON_LEVEL": str(self.PERFMON_LEVEL),
                    "PERFMON_LDIR": str(self.log_dir),
                    "PERFMON_LFILE": str(self.perfmon_log)
                })
        elif pid > 0:
            os.waitpid(pid, 0)


    def end_profile_handler(self, sig, frame):
        pid = os.fork()
        if pid == 0:
            cmds = self.perfmon_stop.split()
            os.execve(
                cmds[0], cmds, {
                    "PERFMON_LEVEL": str(self.PERFMON_LEVEL),
                    "PERFMON_LDIR": str(self.log_dir),
                    "PERFMON_LFILE": str(self.perfmon_log)
                })
        elif pid > 0:
            pid, status = os.waitpid(pid, 0)
            # send SIGUSR2 to signal the profiling is stopped
            with open(self.profile_pid_file, 'r') as f:
                # read pid from file
                profile_pid = int(f.read())
                os.kill(profile_pid, signal.SIGUSR2)

    def fxmark(self, media, fs, bench, ncore, nsocket, nfg, nbg, dio):
        self.perfmon_log = os.path.normpath(
            os.path.join(self.log_dir,
                         '.'.join([media, fs, bench, str(nfg), "pm"])))
        (bin, type) = self.get_bin_type(bench)

        directio = '1' if dio == "directio" else '0'

        if media == "pmem-local":
            numa_cpu_node = self.LOCAL_CPU_NODE
        elif media == "pmem-remote-1-hop" or media == "dm-stripe-1-remote":
            numa_cpu_node = self.REMOTE_1_HOP_CPU_NODE
        elif media == "pmem-remote-2-hop":
            numa_cpu_node = self.REMOTE_2_HOP_CPU_NODE
        else:
            numa_cpu_node = -1

        if directio == '1':
            if fs == "tmpfs":
                print("# INFO: DirectIO under tmpfs disabled by default")
                directio = '0'
            else:
                print("# INFO: DirectIO Enabled")
        if bin is self.fxmark_path:
            cmd = ' '.join([self.fxmark_run_env(fs, "fxmark", type, ncore, nsocket,
                                                self.DELEGATION_THREADS),
                            "sudo" if fs == "strata" else "",
                            bin,
                            "--type", type,
                            "--ncore", str(ncore),
                            "--nbg",  str(nbg),
                            "--duration", str(self.duration),
                            "--directio", directio,
                            "--root", self.test_root,
                            "--profbegin", "\"%s\"" % self.perfmon_start,
                            "--profend",   "\"%s\"" % self.perfmon_stop,
                            "--proflog", self.perfmon_log,
                            "--filesys", fs,
                            "--pid", str(os.getpid()),
                            "--pidfile", self.profile_pid_file,
                            "--delegation_threads", str(self.DELEGATION_THREADS),
                            "--delegation_sockets", str(self.DELEGATION_SOCKETS),
                            "--delegate", str(int(self.delegate == True)),
                            "--rcore", str(int(self.rcore == True))])

        elif bin is self.filebench_path:
            cmd = ' '.join([self.fxmark_run_env(fs, "filebench", type, ncore, nsocket,
                                                self.DELEGATION_THREADS),
                            bin,
                            "--type", type,
                            "--ncore", str(ncore),
                            "--nsocket", str(nsocket),
                            "--duration", str(self.duration),
                            "--root", self.test_root,
                            "--profbegin", "\"%s\"" % self.perfmon_start,
                            "--profend",   "\"%s\"" % self.perfmon_stop,
                            "--proflog", self.perfmon_log,
                            "--fs", fs,
                            "--delegation_threads", str(self.DELEGATION_THREADS),
                            "--delegation_sockets", str(self.DELEGATION_SOCKETS),
                            "--delegate", str(int(self.delegate == True)),
                            "--rcore", str(int(self.rcore == True))])

        elif bin is self.fio_path:
            cmd = ' '.join([self.fxmark_run_env(fs, "fio", type, ncore,
                                                nsocket, self.DELEGATION_THREADS),
                            bin,
                            "--type", type,
                            "--ncore", str(ncore),
                            "--duration", str(self.duration),
                            "--root", self.test_root,
                            "--profbegin", "\"%s\"" % self.perfmon_start,
                            "--profend",   "\"%s\"" % self.perfmon_stop,
                            "--proflog", self.perfmon_log,
                            "--media", media,
                            "--fs", fs,
                            "--bench", bench,
                            "--nfg", str(nfg),
                            "--dio", dio,
                            "--numa_cpu_node", str(numa_cpu_node),
                            "--delegation_threads", str(self.DELEGATION_THREADS),
                            "--delegation_sockets", str(self.DELEGATION_SOCKETS),
                            "--delegate", str(int(self.delegate == True)),
                            "--rcore", str(int(self.rcore == True))])
            
        elif bin is self.dbench_path:
            cmd = ' '.join([self.fxmark_run_env(fs, "dbench", type, ncore,
                                    nsocket, self.DELEGATION_THREADS),
                bin,
                "--type", type,
                "--root", self.test_root,
                "--fs", fs,
                "--delegation_threads", str(self.DELEGATION_THREADS),
                "--delegation_sockets", str(self.DELEGATION_SOCKETS),
                "--delegate", str(int(self.delegate == True))])
            
        start = time.time()
        p = self.exec_cmd(cmd, self.redirect)
        end = time.time()
        print("Execution Time={}".format(end - start))
        if self.redirect:
            for l in p.stdout.readlines():
                self.log(l.decode("utf-8").strip())

    def fxmark_cleanup(self):
        cmd = ' '.join([self.fxmark_clean_env(),
                        "%s; rm -f %s/*.pm" % (self.perfmon_stop, self.log_dir)])
        self.exec_cmd(cmd)
        self.exec_cmd("sudo sh -c \"echo 0 >/proc/sys/kernel/lock_stat\"",
                      self.dev_null)
        if os.path.exists(self.profile_pid_file):
            os.remove(self.profile_pid_file)

    def setup_root_and_bin(self, fs):
        if fs == "splitfs":
            self.test_root = "/mnt/pmem_emul"
        elif fs == "strata":
            self.test_root = "/mlfs/"
        elif fs == "sufs" or fs == "sufs-kv" or fs == "sufs-fd":
            self.test_root = "/sufs/"

        if fs == "sufs":
            self.fxmark_path = os.path.normpath(
                os.path.join(CUR_DIR, self.FXMARK_NAME_SUFS))
        else:
            self.fxmark_path = os.path.normpath(
                os.path.join(CUR_DIR, self.FXMARK_NAME))


    def run(self):
        try:
            cnt = -1
            self.log_start()
            self.fs = ''
            for (cnt, (media, fs, bench, ncore, nsocket, dio)) in enumerate(self.gen_config()):
                print(f"Running Configuration: {(media, fs, bench, ncore, nsocket, dio)}")
                (ncore, nbg) = self.add_bg_worker_if_needed(bench, ncore)
                nfg = ncore - nbg

                self.fs = fs
                self.setup_root_and_bin(fs)

                if self.DRYRUN:
                    self.log("## %s:%s:%s:%s:%s" %
                             (media, fs, bench, nfg, dio))
                    continue

                self.prepre_work(ncore)
                if not self.mount(media, fs, self.test_root):
                    self.log("# Fail to mount %s on %s." % (fs, media))
                    continue

                (bin, type) = self.get_bin_type(bench)

                if bin is not self.fio_path:
                    # Let fio log this part on its own, as one .fio might
                    # contain several test cases
                    self.log("## %s:%s:%s:%s:%s" % (media, fs, bench, nfg, dio))
                self.pre_work()
                self.fxmark(media, fs, bench, ncore, nsocket, nfg, nbg, dio)
                self.post_work(fs)
            self.log("### NUM_TEST_CONF  = %d" % (cnt + 1))

        finally:
            signal.signal(signal.SIGINT, catch_ctrl_C)
            self.log_end()
            self.fxmark_cleanup()

            if self.fs == "sufs" or self.fs == "sufs-kv" or self.fs == "sufs-fd":
                self.umount_sufs()
            elif self.fs == "odinfs":
                self.umount(self.test_root)
                cmd = "sudo %s delete" % self.PARRADM_BIN
                self.exec_cmd(cmd, self.dev_null)
            elif self.fs == "strata":
                self.umount_strata()
            else:
                self.umount(self.test_root)

            self.set_cpus(0)


def confirm_media_path():
    print("%" * 80)
    print("%% WARNING! WARNING! WARNING! WARNING! WARNING!")
    print("%" * 80)
    yn = input("All data in %s %s, %s, %s, %s, %s and %s will be deleted. Is it ok? [Y,N]: "
               % (Runner.PMARRAYDEV, Runner.DMSTRIPEDEV, Runner.PMEMDEV, Runner.HDDDEV, Runner.SSDDEV, Runner.NVMEDEV, Runner.LOOPDEV))
    if yn != "Y":
        print("Please, check Runner.LOOPDEV and Runner.NVMEDEV")
        exit(1)
    yn = input("Are you sure? [Y,N]: ")
    if yn != "Y":
        print("Please, check Runner.LOOPDEV and Runner.NVMEDEV")
        exit(1)
    print("%" * 80)
    print("\n\n")


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--media", help="Underlying storage media")
    parser.add_option("--fs", help="file system")
    parser.add_option("--workload", help="workload")
    parser.add_option("--ncore", help="number of cores")
    parser.add_option("--iotype", help="IO type")
    parser.add_option("--dthread", help="Number of delegation threads")
    parser.add_option("--dsocket", help="Number of delegation sockets")
    parser.add_option("--rcore", help="reduce #of app threads or not")
    parser.add_option("--delegate", help="reserve CPUs for delegation threads "
                                         "or not")
    parser.add_option("--confirm", help="If true, say yes to everything")
    #TODO: Make them optional
    parser.add_option("--duration", help="Execution time of the benchmark")
    parser.add_option("--directory_name", help="Name of the log directory")
    parser.add_option("--log_name", help="name of the log")
    (opts, args) = parser.parse_args()

    for opt in vars(opts):
        val = getattr(opts, opt)
        if val == None:
            print("Missing options: %s" % opt)
            parser.print_help()
            exit(1)

    if cpupol.SMT_LEVEL != 1:
        print("Please disable hyperthreading in the BIOS")
        exit(1)


    # config parameters
    # -----------------
    #
    # o testing core granularity
    # - Runner.CORE_FINE_GRAIN
    # - Runner.CORE_COARSE_GRAIN
    #
    # o profiling level
    # - PerfMon.LEVEL_LOW
    # - PerfMon.LEVEL_PERF_RECORD
    # - PerfMon.LEVEL_PERF_PROBE_SLEEP_LOCK
    # - PerfMon.LEVEL_PERF_PROBE_SLEEP_LOCK_D  # do NOT use if you don't understand what it is
    # - PerfMon.LEVEL_PERF_LOCK                # do NOT use if you don't understand what it is
    # - PerfMon.LEVEL_PERF_STAT                # for cycles and instructions
    #
    # o testcase filter
    # - (storage device, filesystem, test case, # core, directio | bufferedio)

    run_config = [
        (Runner.CORE_FINE_GRAIN,
         PerfMon.LEVEL_LOW,
         int(opts.duration),
         opts.directory_name,
         opts.log_name,
         (opts.media, opts.fs, opts.workload, opts.ncore, opts.iotype,
          int(opts.dthread), int(opts.dsocket), opts.rcore == "True",
          opts.delegate == "True")),
    ]

    #run_config = [
        # Example 1: ext4, nova, pmfs etc.
        # (Runner.CORE_FINE_GRAIN,
        #  PerfMon.LEVEL_LOW,
        #  ("pmem-local", "^ext4$", "DRBL", "*", "bufferedio", 0, 0, False)),
        # Example 2: odinfs, 8 delegation threads with 4 delegaiton sockets
        # (Runner.CORE_FINE_GRAIN,
        #  PerfMon.LEVEL_LOW,
        # 1. Media, 2. File System, 3. Benchmark, 4. Ncore, 5. IO-type,
        # 6. Delegation threads, 7. Delegation sockets,
        # 8. If true, delegation threads do not collocate with application threads
        # ("pm-array", "odinfs", "DRBL", "*", "bufferedio", 8, 8, False)),
        # (Runner.CORE_FINE_GRAIN,
        # PerfMon.LEVEL_LOW,
        # ("pm-array", "odinfs", "fio_global_seq-read-4K", "*", "bufferedio", 8, 8, True)),
        # (Runner.CORE_FINE_GRAIN,
        # PerfMon.LEVEL_LOW,
        # ("pm-array", "odinfs", "fio_global_seq-write-2M", "*", "bufferedio", 8, 8, True)),
        # (Runner.CORE_FINE_GRAIN,
        # PerfMon.LEVEL_LOW,
        # ("pm-array", "odinfs", "fio_global_seq-read-2M", "*", "bufferedio", 8, 8, True)),
        # Example 3: odinfs, 1 delegation threads with 8 delegaiton sockets
        # (Runner.CORE_FINE_GRAIN,
        #  PerfMon.LEVEL_LOW,
        #  ("pm-array", "odinfs", "DRBL", "*", "bufferedio", 1, 8, True)),
        # ("pmem", "ext4", "*", "*", "bufferedio")),
    #]
    if (opts.confirm != "True"):
        confirm_media_path()

    for c in run_config:
        runner = Runner(c[0], c[1], c[2], c[3], c[4], c[5])
        runner.run()
