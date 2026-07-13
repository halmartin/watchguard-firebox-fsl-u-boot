current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

.PHONY: all

target: all

docker-image:
	docker build -t ubuntu1804-layerscapearm64 -f Dockerfile .

docker-build:
	docker run -it --rm -v $(current_dir)/u-boot-2018.09:/src -v $(current_dir)/build-uboot.sh:/tmp/build-uboot.sh ubuntu1804-layerscapearm64 /tmp/build-uboot.sh

docker-build-dbg:
	docker run -it --rm -v $(current_dir)/u-boot-2018.09:/src -v $(current_dir)/build-uboot.sh:/tmp/build-uboot.sh --entrypoint=/bin/bash ubuntu1804-layerscapearm64

all: docker-image docker-build

clean:
	docker run -it --rm -v $(current_dir)/u-boot-2018.09:/src ubuntu1804-layerscapearm64 /bin/sh -c 'cd /src && CROSS_COMPILE=aarch64-linux-gnueabi- make clean && rm *.itb *.bin.gz'
