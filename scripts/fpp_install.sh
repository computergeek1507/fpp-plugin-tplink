#!/bin/bash
set -e

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..

# Build the native plugin first so a transient PyPI/network failure below
# never leaves a stale libfpp-plugin-tplink.so behind on upgrade.
make

apt-get update
apt-get install -y python3-dev python3-venv

python3 -m venv --system-site-packages env

source env/bin/activate

env/bin/pip install python-kasa
