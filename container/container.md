Please perform the below steps **after** you have installed the 5.13.13 kernel. 

### 1. Install Docker

On Ubuntu

```
$ sudo apt install docker.io

# verify that Docker has been successfully installed  
$ sudo docker run hello-world
```

On Fedora
```
$ sudo dnf install dnf-plugins-core

# add the docker repository  
$ sudo dnf config-manager --add-repo https://download.docker.com/linux/fedora/docker-ce.repo

$ sudo dnf install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# start docker service
$ sudo systemctl start docker

# verify that docker has been successfully installed  
sudo docker run hello-world
```

### 2. Run the container

```
# obtain the Ubuntu 20.04 docker image
$ sudo docker pull ubuntu:20.04


# run the Ubuntu container with full privilege and host-dependent 
# directories mapped 

$ sudo docker run --privileged --cap-add=ALL \
    -v /dev/:/dev/ -v /usr/src/:/usr/src/ -v /lib/modules:/lib/modules \
    -it ubuntu:20.04 bin/bash
```

Obtain the repository within the container:

```
$ apt update
$ apt install git sudo
$ git clone https://github.com/vmexit/trio-sosp23-ae.git
```

Now you can follow the steps in [README](../README.md) to run our artifact 
(except that you do not need to install the kernel). 


### For Docker beginners 

After a container exits, Docker by default does not persist changes to the its 
image (but does store the changes temporarily). 

To persist your changes to an image: 

```
# Find the name of the exited container
$ sudo docker ps -a 


$ sudo docker commit <container_name>  <container_image>
```

You can run with the new image:

```
$ $ sudo docker run --privileged --cap-add=ALL \
    -v /dev/:/dev/ -v /usr/src/:/usr/src/ -v /lib/modules:/lib/modules \
    -it <container_image> bin/bash
```
