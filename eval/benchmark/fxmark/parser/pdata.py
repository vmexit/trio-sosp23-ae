#!/usr/bin/env python3
import os
import stat
import sys
import subprocess
import optparse
import math
import pdb
from parser import Parser

CUR_DIR     = os.path.abspath(os.path.dirname(__file__))

"""
# GNUPLOT HOWTO
- http://www.gnuplotting.org/multiplot-placing-graphs-next-to-each-other/
- http://stackoverflow.com/questions/10397750/embedding-multiple-datasets-in-a-gnuplot-command-script
- http://ask.xmodulo.com/draw-stacked-histogram-gnuplot.html
"""

class Plotter(object):
    def __init__(self, log_file):
        # config
        self.EXCLUDED_FS = ()  # ("tmpfs")

        # init.
        self.log_file = log_file
        self.parser = Parser()
        self.parser.parse(self.log_file)
        self.config = self._get_config()
        self.ncore = int(self.parser.get_config("PHYSICAL_CHIPS")) * \
                     int(self.parser.get_config("CORE_PER_CHIP"))
        self.out_dir  = ""
 
    def _get_config(self):
        all_config = []
        config_dic = {}
        for kd in self.parser.search_data():
            key = kd[0]
            for (i, k) in enumerate(key):
                try:
                    all_config[i]
                except IndexError:
                    all_config.append(set())
                all_config[i].add(k)
        for (i, key) in enumerate(["media", "fs", "bench", "ncore", "iomode"]):
            config_dic[key] = sorted(list(all_config[i]))
        return config_dic



    def _get_fs_list(self, media, bench, iomode):
        data = self.parser.search_data([media, "*", bench, "*", iomode])
        fs_set = set()
        for kd in data:
            fs = kd[0][1]
            if fs not in self.EXCLUDED_FS:
                fs_set.add(fs)
        #remove tmpfs - to see more acurate comparision between storage fses
#        fs_set.remove("tmpfs");
        return sorted(list(fs_set))

    def _parse_latency(self, str, expect_name):
        name = str.split('=')[0]
        value = str.split('=')[1]

        assert(name == expect_name)

        return float(value)

    def _print_data(self, d_kv, out_file, bench, type):
        if (type == "filebench"):
            if bench == "filebench_videoserver":
                print("%s %s %s" %
                      (d_kv["ncpu"], float(d_kv["read_bw"]),
                       float(d_kv["write_bw"])),
                      file=out_file)
            else:
                print("%s %s" %
                      (d_kv["ncpu"], float(d_kv["works/sec"])),
                      file=out_file)

        elif (type == "fio"):

            if "read" in bench:
                read_50_lat = self._parse_latency(d_kv["read_50_latency"],
                                                        "50.000000%")

                read_99_lat = self._parse_latency(d_kv["read_99_latency"],
                                                        "99.000000%")
                print("%s %s %s %s" %
                      (d_kv["ncpu"], float(d_kv["read_bandwidth"]),
                       read_50_lat,  read_99_lat), file=out_file)

            else:
                write_50_lat = self._parse_latency(d_kv["write_50_latency"],
                                                        '50.000000%')

                write_99_lat = self._parse_latency(d_kv["write_99_latency"],
                                                        '99.000000%')
                print("%s %s %s %s" %
                      (d_kv["ncpu"], float(d_kv["write_bandwidth"]),
                       write_50_lat,  write_99_lat), file=out_file)
        elif (type == "fxmark"):
                print("%s %s" %
                       (d_kv["ncpu"], float(d_kv["works/sec"])),
                      file=out_file)
        else:
                print("%s %s" %
                       (d_kv["ncpu"], float(d_kv["works/msec"])),
                      file=out_file)  




    def _get_data(self, media, bench, iomode, type):

        def _get_data_file(fs):
            return "%s:%s:%s:%s.dat" % (media, fs, bench, iomode)

        # check if there are data
        fs_list = self._get_fs_list(media, bench, iomode)
        if fs_list == []:
            return

        # gen sc data files
        for fs in fs_list:
            data = self.parser.search_data([media, fs, bench, "*", iomode])
            if data == []:
                continue
            data_file = os.path.join(self.out_dir, _get_data_file(fs))
            with open(data_file, "w") as out:
                print("# %s:%s:%s:%s:*" % (media, fs, bench, iomode), file=out)
                for d_kv in data:
                    d_kv = d_kv[1]
                    if int(d_kv["ncpu"]) > self.ncore:
                        break
                    self._print_data(d_kv, out, bench, type)

        

    def get_data(self, out_dir, type):
        self.out_dir  = out_dir
        subprocess.call("mkdir -p %s" % self.out_dir, shell=True)
        
        for media in self.config["media"]:
            for bench in self.config["bench"]:
                for iomode in self.config["iomode"]:
                    self._get_data(media, bench, iomode, type)
                    


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("--log",   help="Log file")
    parser.add_option("--out",   help="output directory")
    parser.add_option("--type",  help="figure number")
    (opts, args) = parser.parse_args()

    # check arg
    for opt in vars(opts):
        val = getattr(opts, opt)
        if val == None:
            print("Missing options: %s" % opt)
            parser.print_help()
            exit(1)
    # run
    plotter = Plotter(opts.log)
    plotter.get_data(opts.out, opts.type)

