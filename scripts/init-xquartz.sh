#!/bin/zsh

# Call this script on Mac (not in the Docker container)
#  - Usage: source init-xquartz.sh

export DISPLAY=:0.0
xhost +
