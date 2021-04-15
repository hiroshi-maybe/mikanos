#!/bin/bash

# Run with vscode user!

export DISPLAY=host.docker.internal:0

cd $HOME/mikanos/helloworld/asm
$HOME/osbook/devenv/run_qemu.sh BOOTX64.EFI
