#!/bin/bash

# fpp-plugin-tplink install script

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..

sudo apt install python3.11-venv

python3 -m venv --system-site-packages env
# sudo apt-get -y install pip
source env/bin/activate
sudo env/bin/pip install python-kasa
make
