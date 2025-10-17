#!/bin/bash

set -e

python3 /mnt/c/users/ethan/onedrive/documents/github/LGVL-BareMetal-Demo/generate-template.cpp

make clean
make -j$(nproc)