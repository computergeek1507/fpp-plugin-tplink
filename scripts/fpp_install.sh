#!/bin/bash
set -e

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..

# Tapo devices are spoken to natively (KLAP over HTTP), which needs OpenSSL's
# libcrypto. FPP images ship its headers; install them if this one does not.
if [ ! -f /usr/include/openssl/evp.h ]; then
    apt-get update
    apt-get install -y libssl-dev
fi

make

# Earlier versions drove Tapo devices through python-kasa in a venv here.
rm -rf env
