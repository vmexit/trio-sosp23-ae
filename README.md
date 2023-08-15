# Artifact Evaluation Submission for Trio [SOSP '23] 

This repository contains the artifact for reproducing our SOSP '23 paper "Enabling High-Performance and Secure Userspace NVM File Systems with the Trio Architecture". 

# Table of Contents
* [Overview](#overview)
* [Setup](#setup)
* [Running experiments](#running-experiments)
* [Validation of the main claims](#validation-of-the-main-claims)
* [Known Issues](#known-issues)
* [Authors](#authors)

# Overview 

### Structure:

```
root
|---- arckfs             (source code of arckfs)
    |---- fsutils        (simple file system commands)         
    |---- include        (header files shared between libfs and kernel module)
    |---- lib            (libraries shared between libfs and kernel module) 
    |---- kfs            (kernel part of arckfs)
    |---- libfs          (arckfs library file system source code)
    |---- libfsfd        (fdfs source code)
    |-----libfskv        (kvfs source code)
    |-----test           (simple testing code for arckfs)
|---- fs                 (source code of other evaluated file systems)
    |---- odinfs         
    |---- parradm       
    |---- nova         
    |---- pmfs 
    |---- winefs
    |---- strata
|---- linux              (5.13.13 Linux kernel)
|---- eval               (evaluation)
    |---- benchmark      (source code of workload applications) 
    |---- scripts        (main evaluation scripts) 
    |---- fig            (figures) 
    |---- data           (raw data)
|---- dep.sh             (scripts to install dependency)    
```

### Environment: 

Our artifact should run on any Linux distribution. The current scripts are developed for **Ubuntu 20.04.4 LTS**. Porting to other Linux distributions would require some scripts modifications, especially ```dep.sh```, which installs dependencies with package management tools. 

To run our artifact within a VM to test functionality, see [here](vm/vm.md)

# Setup 

**Note**: For the below steps, our scripts will complain if it fails to compile or install the target. Check the end part of the scripts' output to ensure that the install is successful. Also, some scripts would prompt to ask the sudo permission at the beginning. 

### 1. Install the dependencies:
```
$ ./dep.sh 
```

### 2. Install the 5.13.13 Linux kernel (50GB space and 20 minutes)
```
$ cd linux
$ cp config .config
$ make oldconfig             (update the config with the provided .config file)
```

Say N to KASAN if the config program asks about it. 

```
KASAN: runtime memory debugger (KASAN) [N/y/?] (NEW) N
```


Next, please use your favorite way to compile and install the kernel. The below step is just for reference. The installation requires 50GB space and takes around 20 minutes on our machines. 

For Ubuntu:
```
$ make -j8 deb-pkg           (generate the kernel installment package)
$ cd ..
$ sudo dpkg -i *.deb         (install the package) 
```

Otherwise, the classical ways will work as well:

```
$ make -j8              
$ make -j8 modules 
$ sudo make install
$ sudo make modules_install
```
Reboot the machine to the installed 5.13.13 kernel. 

### 3. Install and insmod file systems 

```
$ cd fs
$ ./compile.sh
```
The script will compile, install, and insert the following kernel modules:

* Odinfs 
* PMFS 
* NOVA 
* Winefs

Please note that these kernel modules do not persist across machine reboots. Please rerun the above steps every time you boot the kernel. Our experimental scripts will detect this before execution. 

### 4. Install ArckFS

```
$ cd arckfs
$ ./compile.sh
```

### 5. Compile and install benchmarks 

**5.1 Various versions of Filebench**

```
# Normal filebench
$ cd eval/benchmark/filebench
$ ./compile.sh

# Filebench adapted for FDFS
$ cd eval/benchmark/filebench-fd 
$ ./compile.sh

# Filebench adapted for KVFS
$ cd eval/benchmark/filebench-fd 
$ ./compile.sh
```

If the installation fails, see [Known Issues](#known-issues)

**5.2 Fxmark and fio**

```
$ cd eval/benchmark
$ ./install.sh
```

**5.3 Verify the installation**

```
$ cd eval/benchmark
$ ./check.sh
```

If ```check.sh``` fails, follow the instruction (explained in [Known Issues](#known-issues)) and rerun the ```check.sh```. 

# Running Experiments:

Main scripts are under ```eval/scripts/```

```
eval/scripts
|---- fio.sh                    (FIO-related experiments; fig6)
|---- fxmark.sh                 (Fxmark-related experiments; fig7)
|---- filebench.sh              (Filebench-related experiments; fig9)
|-----filebench-sp.sh           (running customized filebench for customized file system; fig10)
|-----fig5.sh                   (running single-threaded results; fig5)
|-----dbench.sh                 (db_bench results for leveldb; table 5)
|---- run-all.sh                (running all the above scripts)
|---- run-test.sh               (quick run of fio, fxmark, and filbench with the evaluated file systems)
|---- arckfs.sh                 (rerun all the experiments related to arckfs)
|---- parse.sh                  (parse and output the results to directory: eval/data)
```

**Exeuction time**

The table below shows the execution time of each script on a two-socket, 56 core machine. Machines with more sockets take longer. 

    
|      Scripts     | Execution Time in Minutes |
|:----------------:|:-------------------------:|
|      fio.sh      |                       270 |
|     fxmark.sh    |                       177 |
|   filebench.sh   |                        88 | 
|   fileben-sp.sh  |                         9 |
|      fig5.sh     |                        21 |
|     debnch.sh    |                         7 |
|    run-all.sh    |                       572 |
|    run-test.sh   |                       158 |
|     arckfs.sh    |                        82 |
|     parse.sh     |                        <1 |
                                               
**Note**: 
* We recommend running ```run-test.sh``` first to ensure that everything seems correct 
* And then perform a full run with ```run-all.sh``` to get all the data. 
* Feel free to customize ```run-all.sh``` to skip some experiments. 


**1. Hardware setup**: 
* Please disable hyperthreading in the BIOS to avoid issues due to CPU pinning before running experiments. 

**2. Testing runs**

```
$ cd eval/scripts
$ ./run-test.sh
```

**3. Full runs**

```
$ cd eval/scripts
$ ./run-all.sh
$ ./parse.sh
```

**4. Generate figures**

```
$ cd eval/fig
$ ./fig.sh
```

Please check all the ```*.eps``` files numbered according to the figures in the paper.  We do not draw legends in these figures (since we cannot predict where to put them  beforehand.) Please refer to the paper for the legends. 

```Table5.md``` shows Table 5 of the paper (in Markdown format). 

# Validation of the main claims:

Please refer to [here](main-claim.md)

# Known issues 

1. The kernel might complain about CPU soft lockup during some experiments. This can be safely ignored. 

2. Filebench installation fails upon the first invocation of ./compile.sh. Rerun ./compile.sh can successfully install filebench

3. ArckFS works by intercepting file system-related system calls. However, some library invokes these system calls during their initialization, before ArckFS can initialize. Fixing this requires enforcing the initialization order of libraries, which, to our understanding, depends on the implementation of the loader and is not always possible. 

The above problem mostly affects fio. We have attached a pre-built version of all our workloads, including fio, that has the right library initialization order (at least on Ubuntu 20.04). If you encounter the above problem, please use the pre-built version instead. 

4. ArckFS numbers its threads (i.e., the first thread within the process is 0 and the next thread is 1 etc.) based on difference of gettid() and the thread id of the first thread. (With the current Linux, there seems to be no good way to number threads.). If the system is constantly creating a large number of proceses/threads in the background (e.g., running k8s services), ArckFS may panic. A small number of background processes are fine. 

# Authors

Diyu Zhou (EPFL)

Vojtech Aschenbrenner (EPFL) 

Tao Lyu (EPFL) 

Jian Zhang (Rutgers University) 

Sudarsun Kannan (Rutgers University) 

Sanidhya Kashyap (EPFL)

# License

Trio is licensed under Apache License 2.0.
