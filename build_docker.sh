#!/bin/bash

# Build the docker image
docker build -t es-build .

# Run the container and compile ES using cmake
docker run --rm -v "$(pwd):/src" es-build bash -c "mkdir -p /src/build && cd /src/build && cmake .. && make -j$(nproc)"
