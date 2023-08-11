### 1. VM image

The VM image is on [Zenodo](https://zenodo.org/record/8239299)

The VM is created with a minimal installation of Ubuntu 20.04. 

Username: trio, passwd: sosp2023. It has the sudo permission.  

### 2. Running the VM

```
$ ./run.sh 
```

The script creates a VM with two socket VM, 56-core, 32GB DRAM, and 
256GB NVM (emulated with two 128GB files under ./vm_nvm_files/, see the comments of the scripts)

To access the VM run 

```
$ ssh -p 9887 trio@localhost 
```

### 3. Verifying that NVM has been succesfully emulated

Inside the VM, run

```
$ ndctl list
```

Output likes below shows that the NVM has been successfully emulated

```
[
  {
    "dev":"namespace1.0",
    "mode":"raw",
    "size":137438953472,
    "sector_size":512,
    "blockdev":"pmem1"
  },
  {
    "dev":"namespace0.0",
    "mode":"raw",
    "size":137438953472,
    "sector_size":512,
    "blockdev":"pmem0"
  }
]
```







