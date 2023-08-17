# Minimal working examples and sample outputs

### Switch the Optane PM on the first NUMA node to the devdax mode

```
$ sudo ndctl create-namespace -f -e namespace0.0 --mode=devdax

{
  "dev":"namespace0.0",
  "mode":"devdax",
  "map":"dev",
  "size":"126.00 GiB (135.29 GB)",
  "uuid":"57f7f7e5-6cf4-4507-b4be-23c62c1ad633",
  "daxregion":{
    "id":0,
    "size":"126.00 GiB (135.29 GB)",
    "align":2097152,
    "devices":[
      {
        "chardev":"dax0.0",
        "size":"126.00 GiB (135.29 GB)",
        "target_node":0,
        "mode":"devdax"
      }
    ]
  },
  "align":2097152
}
```

### Initialize ArckFS

```
$ cd arckfs

# mount ArckFS on the first NUMA node 
$ sudo insmod kfs/sufs.ko pm_nr=1

# hack to grant everyone access to ArckFS
$ sudo chmod 666 /dev/supremefs

$ cd ../fsutils/

# format ArckFS
$ sudo ./init 

ret is 0

$ ./ls /sufs/

d .                     2       0   1
d ..                    2       0   1
```

### Create directories 

All instructions below assume the working directory is "arckfs/fsutils/"

```
$ ./mkdir /sufs/dir1/ 
$ ./ls /sufs/

d .                     2       0   1
d ..                    2       0   1
d dir1            8089011 1073741824   1

$ ./mkdir /sufs/dir1/dir2/
$ ./ls /sufs/dir1/

d .               8089011       0   1
d ..                    2       0   1
d dir2            8089012 1073741824   1
```

### Create regular files 

```
# create a regular file "/sufs/dir1/dir2/hello.txt" with content "Hello, world!"

$ ./echof /sufs/dir1/dir2/hello.txt "Hello, world!"
$ ./cat /sufs/dir1/dir2/hello.txt
Hello, world!

$ ./cp /sufs/dir1/dir2/hello.txt /sufs/dir1/dir2/hello-cp.txt
$ ./cat /sufs/dir1/dir2/hello-cp.txt
Hello, world!
```

### Delete files

```
$ ./rm /sufs/dir1/dir2/hello.txt /sufs/dir1/dir2/hello-cp.txt
$ ./ls /sufs/dir1/dir2/

d .               8089012       0   1
d ..              8089011       0   1

$ ./rm /sufs/dir1/dir2/
$ ./ls /sufs/dir1

d .               8089011       0   1
d ..                    2       0   1

$ ./rm /sufs/dir1/
$ ./ls /sufs/

d .                     2       0   1
d ..                    2       0   1
```