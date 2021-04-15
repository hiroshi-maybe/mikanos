#!/bin/bash

# Build the bootloader
# https://github.com/uchan-nos/mikanos-build#%E3%83%96%E3%83%BC%E3%83%88%E3%83%AD%E3%83%BC%E3%83%80%E3%83%BC%E3%81%AE%E3%83%93%E3%83%AB%E3%83%89

export OS_DIR=/workspaces/mikanos/mikanos

cd /home/vscode/edk2
ln -s $OS_DIR/MikanLoaderPkg ./
source edksetup.sh
# cp $OS_DIR/target.txt Conf/target.txt
