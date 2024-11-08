#!/bin/bash

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..

sudo apt-get update
sudo apt install python3-dev python3-venv -y

python3 -m venv --system-site-packages env

source env/bin/activate

sudo env/bin/pip install python-kasa

make
