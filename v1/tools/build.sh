#!/usr/bin/bash

if [ ! -d build ]; then
    mkdir build
fi

if [ ! -d ~/.orta ]; then
    mkdir ~/.orta
fi

if [ "$1" == "install_libs" ]; then
  cp std/* ~/.orta/
  exit 0
fi

: ${CC:="/usr/bin/cc"}

FLAGS="-O3 -static"

$CC $FLAGS -o build/orta src/orta.c
$CC $FLAGS -o build/deovm src/deovm.c
$CC $FLAGS -o build/fcfo src/fcfo.c

cp std/* ~/.orta/
