#!/usr/bin/env bash

# Rebuilds the project fully. This includes regenerating the cmake build directory.

./setup.sh
cd build
make $@
cd ..
