#!/bin/bash

set -x
git clean -fxd
git clean -fXd   
if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    arch=`uname -m`
    # manylinux build
    echo "Building manylinux wheels with auditwheel and docker"
    echo "Building $arch wheels"
    if [ "$arch" == "aarch64" -o "$arch" == "arm64" ]; then
        CFLAGS=""
        PRE_CMD=""
        PLAT=manylinux2014_aarch64
        DOCKER_IMAGE=quay.io/pypa/$PLAT
        docker pull $DOCKER_IMAGE
        docker run --rm -v `pwd`:/io -e PLAT=$PLAT -e PYTHON=$PYTHON -e CFLAGS=$CFLAGS $DOCKER_IMAGE $PRE_CMD /io/travis/build-wheels.sh
    else
        CFLAGS=-msse2
        echo "Ignoring manylinux1_i686"
        for PLAT in manylinux1_x86_64 manylinux2010_x86_64; do
            DOCKER_IMAGE=quay.io/pypa/$PLAT
            if [ "$PLAT" == "manylinux1_i686" ]; then
                PRE_CMD=linux32
            else
                PRE_CMD=""
            fi
            docker pull $DOCKER_IMAGE
            docker run --rm -v `pwd`:/io -e PLAT=$PLAT -e PYTHON=$PYTHON -e CFLAGS=$CFLAGS $DOCKER_IMAGE $PRE_CMD /io/travis/build-wheels.sh
        done
    fi
    WHEEL_DIR="wheelhouse"
else
    # os x build
    cd python_bindings
    $PIP install --user -r dev-requirements.txt
    $PY setup.py build_ext
    $PY setup.py sdist bdist_wheel
    WHEEL_DIR="dist"
    cd ..
fi

