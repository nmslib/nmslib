#!/bin/bash

set -x
git clean -fxd
git clean -fXd   

echo "Python version: $PYTHON"

arch=`uname -m`
# manylinux build
echo "Building manylinux wheels with auditwheel and docker"
echo "Building $arch wheels"
if [ "$arch" == "aarch64" -o "$arch" == "arm64" ]; then
    CFLAGS="-march=armv8-a"
    PRE_CMD=""
    for PLAT in manylinux2014_aarch64 ; do
        DOCKER_IMAGE=quay.io/pypa/$PLAT
        docker pull $DOCKER_IMAGE
        docker run --rm -v `pwd`:/io -e PLAT=$PLAT -e PYTHON=$PYTHON -e CFLAGS=$CFLAGS $DOCKER_IMAGE $PRE_CMD /io/build_linux_wheels/build-wheels.sh
    done
else
    CFLAGS=-msse2
    echo "Ignoring manylinux1_i686"
    PRE_CMD=""
    for PLAT in manylinux2014_x86_64; do
        DOCKER_IMAGE=quay.io/pypa/$PLAT
        docker pull $DOCKER_IMAGE
        docker run --rm -v `pwd`:/io -e PLAT=$PLAT -e PYTHON=$PYTHON -e CFLAGS=$CFLAGS $DOCKER_IMAGE $PRE_CMD /io/build_linux_wheels/build-wheels.sh
    done
fi
WHEEL_DIR="wheelhouse"

