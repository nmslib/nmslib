#!/bin/bash
set -e -x

# Auditwheel requirements
yum install -y atlas-devel
# nmslib requirements
#yum install -y gsl-devel
#yum install -y boost-devel

OUT_DIR=/io/python_bindings/dist/
mkdir -p "${OUT_DIR}"
for PYBIN in /opt/python/*/bin; do
    # Select python version corresponding to this test
    if [ $("${PYBIN}/python" --version 2>&1 | grep -c "Python ${PYTHON}") -eq 0 ]; then
        continue
    fi

    # Setup
    TMP_DIR="wheelhouse_tmp/${PLAT}/${PYBIN}"
    REPAIR_DIR="wheelhouse_repair/${PLAT}/${PYBIN}"
    mkdir -p $TMP_DIR
    mkdir -p $REPAIR_DIR

    # Compile wheels
    cd /io/python_bindings
    "${PYBIN}/pip" install -r dev-requirements.txt
    "${PYBIN}/python" setup.py build_ext
    "${PYBIN}/pip" wheel . -w "${TMP_DIR}"

    # Bundle external shared libraries into the wheels
    ls -lrt $TMP_DIR
    for whl in $(ls -1 ${TMP_DIR} | grep nmslib); do
      auditwheel repair --plat "$PLAT" -w "${REPAIR_DIR}" ${TMP_DIR}/$whl 
    done

    # Install and test
    "${PYBIN}/pip" install nmslib --no-index -f "${REPAIR_DIR}"
    cd /io/python_bindings/tests/
    "${PYBIN}/python" -m pytest bindings_test.py
    cd ..

    # Clean up
    "${PYBIN}/pip" uninstall -y nmslib
    rm -rf build
    
    # Move wheel to output directory
    for whl in $(ls -1 ${REPAIR_DIR} | grep nmslib); do
      mv ${REPAIR_DIR}/$whl "${OUT_DIR}"
    done
done

