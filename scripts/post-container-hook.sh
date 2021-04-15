#!/bin/bash

export WORK_DIR=/workspaces/mikanos
export OS_DIR=$WORK_DIR/mikanos
if [ ! -d $OS_DIR ]
then
    echo "Directory $OS_DIR DOES NOT exists. Checking out..."
    git clone https://github.com/uchan-nos/mikanos.git $OS_DIR
fi

chmod 755 $OS_DIR
chmod 666 /dev/null

sudo -u vscode ln -s $WORK_DIR /home/vscode/mikanos
sudo -u vscode bash $WORK_DIR/scripts/build-loader.sh

su - vscode
