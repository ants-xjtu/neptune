#!/bin/bash

set -e

make -C build clean
make -C Vendor/libnids clean
make -C Vendor/prads clean
