#!/bin/sh
cd GETDecoder
mkdir build
cd build
cmake ../
make -j2
cd ../..
make
