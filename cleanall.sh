#!/bin/bash

set -e

bash -c 'cd build && make clean'
make -C Vendor/libnids clean
make -C Vendor/prads clean
