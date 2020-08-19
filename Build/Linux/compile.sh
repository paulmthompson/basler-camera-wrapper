#!/bin/bash

PYLON_VERSION=${1:-5.2}

#For pylon 5.1 and 5.2
if [ "$PYLON_VERSION" == "5.1" ] || [ "$PYLON_VERSION" == "5.2" ]; then
  echo "$PYLON_VERSION"
  PYLON_LIB="/opt/pylon5/lib64"
  PYLON_INCLUDE="/opt/pylon5/include"
  GEN_API="-lGenApi_gcc_v3_1_Basler_pylon_v5_1"
  GEN_BASE="-lGCBase_gcc_v3_1_Basler_pylon_v5_1"
elif [ "$PYLON_VERSION" == "6.1" ]; then
  echo "$PYLON_VERSION"
  PYLON_LIB="/opt/pylon/lib"
  PYLON_INCLUDE="/opt/pylon/include"
  GEN_API="-lGenApi_gcc_v3_1_Basler_pylon"
  GEN_BASE="-lGCBase_gcc_v3_1_Basler_pylon"
else
  echo "incompatible Pylon version!"
fi

g++ -c -fPIC -I"$PYLON_INCLUDE" ../../src/BaslerCpp.cpp -lpthread
g++ -c -fPIC -I"$PYLON_INCLUDE" ../../src/BaslerCWrapper.cc -o BaslerCWrapper.o
g++ BaslerCpp.o BaslerCWrapper.o -shared -o BaslerCpp.so -Wl,--enable-new-dtags -Wl,-rpath,"$PYLON_LIB" -L"$PYLON_LIB" -Wl,-E -lpylonbase -lpylonutility "$GEN_API" "$GEN_BASE"
