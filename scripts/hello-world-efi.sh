#!/bin/bash

# Run with vscode user!

export DISPLAY=host.docker.internal:0

# cd $HOME/mikanos/helloworld/asm
# $HOME/osbook/devenv/run_qemu.sh BOOTX64.EFI

cd $HOME/mikanos/helloworld/c
clang -target x86_64-pc-win32-coff -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -c hello.c
lld-link /subsystem:efi_application /entry:EfiMain /out:hello.efi hello.o
$HOME/osbook/devenv/run_qemu.sh hello.efi
