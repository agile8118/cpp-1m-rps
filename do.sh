#!/bin/bash


BINARY_NAME="cpp-server"
CMD=$1

# If no command is given, show usage
if [ -z "$CMD" ]; then
  echo "Usage: $0 [-cmake | -build | -run]"
  exit 1
fi

if [ "$CMD" == "cmake" ]; then
  rm -rf build
  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release ..
  # cmake ..
  make
fi


if [ "$CMD" == "build" ]; then
  cd build
  make
  cd ..
fi

if [ "$CMD" == "run" ]; then
  cd build
  make
  ./$BINARY_NAME
  cd ..
fi


