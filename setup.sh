#!/usr/bin/env bash

# Setup script for building VVVVVV for PSP.

export CC=psp-gcc
export CXX=psp-g++

if ! [ -e build ]; then
   echo "! creating build directory"
   mkdir build
fi

echo "* running cmake"
cd build
psp-cmake ..
cd ..

echo "* setup complete"
