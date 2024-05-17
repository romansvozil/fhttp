#!/bin/bash

# Build the project
echo "Building the project..."
mkdir build
cd build
cmake ..
make clean
make