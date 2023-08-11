#!/bin/bash

# This script creates a VM with two socket VM, 56-core, 32GB DRAM, and 256GB NVM
# Please check the nvm_file_path before running the scripts
# Please don't change the NVM size, it needs to be that large to run our
# benchmarks

TCP_PORT=9887
QMP_PORT=9889


IMAGE=./trio.img

nvm_file_path="./vm_nvm_files/"
nvm_size=128G

mkdir $nvm_file_path

qemu-system-x86_64 \
  -enable-kvm \
  -cpu host \
  -m  32G,slots=2,maxmem=512G \
  -smp cores=28,threads=1,sockets=2 \
  -numa node,nodeid=0,mem=16G,cpus=0-27 \
  -numa node,nodeid=1,mem=16G,cpus=28-55 \
  -netdev user,id=hostnet0,hostfwd=tcp::$TCP_PORT-:22 \
  -drive if=virtio,file=$IMAGE,format=qcow2 \
  -device virtio-serial-pci,id=virtio-serial0,bus=pci.0,addr=0x6 \
  -device virtio-net-pci,netdev=hostnet0,id=net0,bus=pci.0,addr=0x3 \
  -machine pc,accel=kvm,nvdimm=on \
  -qmp tcp:127.0.0.1:$QMP_PORT,server,nowait \
  -device nvdimm,memdev=mem1,id=nv1 \
  -device nvdimm,memdev=mem2,id=nv2 \
  -object memory-backend-file,id=mem1,share,mem-path=$nvm_file_path/vpm1,size=$nvm_size,align=2M \
  -object memory-backend-file,id=mem2,share,mem-path=$nvm_file_path/vpm2,size=$nvm_size,align=2M \
  -nographic


