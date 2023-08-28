#!/bin/bash

# fpp-plugin-tplink install script

BASEDIR=$(dirname $0)
cd $BASEDIR
cd ..

#sudo apt-get -y update
#sudo apt-get -y install libasio-dev --no-install-recommends

sudo apt-get -y install pip
sudo pip install python-kasa

make
