#!/bin/sh

export CROSS_COMPILE=aarch64-linux-gnu-
cd /src && make clean && make ls1043aqds_wg_t40_qspi_config && make && gzip u-boot.bin
