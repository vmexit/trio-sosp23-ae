DPDK_VER := 16.11

.PHONY: all  clean
all:	
	make -C nvml -j8
	(cd rdma-core && ./build.sh)
	(cd rdma && make)
	(cd syscall_intercept; \
	 mkdir build; \
	 mkdir install; \
	 cd build; \
	 cmake -DCMAKE_INSTALL_PREFIX=$(shell pwd)/syscall_intercept/install -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release ..; \
	 make; \
	 make install)
	(cd jemalloc-4.5.0 && \
	 ./autogen.sh && \
	 ./configure && \
	 make && \
	 sudo make install)


clean:
	make clean -C nvml
	@rm -rf rdma-core/build
