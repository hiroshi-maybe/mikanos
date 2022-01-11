#!/bin/bash

# Build the bootloader
# https://github.com/uchan-nos/mikanos-build#%E3%83%96%E3%83%BC%E3%83%88%E3%83%AD%E3%83%BC%E3%83%80%E3%83%BC%E3%81%AE%E3%83%93%E3%83%AB%E3%83%89

export OS_DIR=/workspaces/mikanos

cd /home/vscode/edk2

# https://twitter.com/ww40336161/status/1380312835968835584
git checkout edk2-stable202102

if [ ! -d MikanLoaderPkg ]; then
    ln -s $OS_DIR/MikanLoaderPkg ./
fi

source ./edksetup.sh

sed -i '/ACTIVE_PLATFORM/ s:= .*$:= MikanLoaderPkg/MikanLoaderPkg.dsc:' Conf/target.txt
sed -i '/TARGET_ARCH/ s:= .*$:= X64:' Conf/target.txt
sed -i '/TOOL_CHAIN_TAG/ s:= .*$:= CLANG38:' Conf/target.txt

sed -i '/CLANG38/ s/-flto//' Conf/tools_def.txt

build

export DISPLAY=host.docker.internal:0
$HOME/osbook/devenv/run_qemu.sh Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi
