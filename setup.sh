#!/usr/bin/env bash

# Setup script for building VVVVVV for PSP.

export CC=psp-gcc
export CXX=psp-g++

if [ -e build ]; then
   echo "! removing old build directory"
   rm -rf build
fi

mkdir build

pspsdk_path=$(psp-config --pspsdk-path)
psp_prefix=$(psp-config --psp-prefix)

libc="-lc"
psp_libraries="\
   -lc \
   -lpspaudio -lpspvram -lGL -lpsprtc -lpspvfpu \
   -lpspdebug -lpspgu -lpspdisplay -lpspge -lpsphprm -lpspctrl -lpspsdk \
   -lpspnet -lpspnet_inet -lpsputility -lpspuser -lpspkernel"

lib_dirs="-L$pspsdk_path/lib -L$psp_prefix/lib"
include_dir=$psp_prefix/include
lib_link="-lSDL2_mixer -lSDL2 -lvorbisfile -lvorbis -logg $psp_libraries"

lib_flags="$lib_dirs $lib_link"

echo "include directory: $include_dir"
echo "library flags: $lib_flags"

echo "* running cmake"
cd build
psp-cmake .. \
   -D SDL2_INCLUDE_DIRS="$include_dir" \
   -D SDL2_LIBRARIES="$lib_flags"
cd ..

echo "* setup complete"
