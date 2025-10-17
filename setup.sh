#!/bin/bash

sudo apt update
sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86 git gcc-multilib g++-multilib

git clone --depth 1 --branch release/v9.2 https://github.com/lvgl/lvgl.git