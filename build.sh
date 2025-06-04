#!/bin/bash
echo "Building project..."
mkdir -p build && cd build && cmake .. && make clean && make
if [ $? -ne 0 ]; then
    echo "Build failed. Please check the output for errors."
    exit 1
fi
echo "Build completed successfully."