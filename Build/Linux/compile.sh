#!/bin/sh

#VAR=${1:-DEFAULTVALUE}

g++ -c -fPIC -I/opt/pylon5/include ../../src/BaslerCpp.cpp -lpthread

g++ -c -fPIC -I/opt/pylon5/include ../../src/BaslerCWrapper.cc -o BaslerCWrapper.o

g++ BaslerCpp.o BaslerCWrapper.o -shared -o BaslerCpp.so -Wl,--enable-new-dtags -Wl,-rpath,/opt/pylon5/lib64 -L/opt/pylon5/lib64 -Wl,-E -lpylonbase -lpylonutility -lGenApi_gcc_v3_1_Basler_pylon_v5_1 -lGCBase_gcc_v3_1_Basler_pylon_v5_1
