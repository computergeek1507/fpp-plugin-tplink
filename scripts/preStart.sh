#!/bin/sh

echo "Running fpp-plugin-tplink PreStart Script"

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..
make
