set $filesize=512m
set $numactivevids=32
set $numpassivevids=194
set $reuseit=false
set $readiosize=256k
set $writeiosize=1m

set $passvidsname=passivevids
set $actvidsname=activevids

define fileset name=$actvidsname,path=$dir,size=$filesize,entries=$numactivevids,dirwidth=4,prealloc,paralloc,reuse=$reuseit
define fileset name=$passvidsname,path=$dir,size=$filesize,entries=$numpassivevids,dirwidth=20,prealloc=50,paralloc,reuse=$reuseit

define process name=vidwriter,instances=1
{
  thread name=vidwriter,memsize=10m,instances=$wthreads
  {
    flowop deletefile name=vidremover,filesetname=$passvidsname
    flowop createfile name=wrtopen,filesetname=$passvidsname,fd=1
    flowop writewholefile name=newvid,iosize=$writeiosize,fd=1,srcfd=1
    flowop closefile name=wrtclose, fd=1
  }
  thread name=vidreaders,memsize=10m,instances=$rthreads
  {
    flowop read name=vidreader,filesetname=$actvidsname,iosize=$readiosize
    flowop bwlimit name=serverlimit, target=vidreader
  }
}

echo  "Video Server Version 3.0 personality successfully loaded"

####################################
# re-configured values by fxmark
####################################
# set $dir=[test partition]
# set $wthreads=[$cpu]
# set $rthreads=[$cpu]
# run [benchmark time]
