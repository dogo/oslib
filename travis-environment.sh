#!/bin/bash 
set -ex

# Add PSP environment variable
echo $TRAVIS_BUILD_DIR
export PSPDEV=`pwd`/pspsdk
export PSPSDK=$PSPDEV/bin
export PATH=$PSPSDK:$PATH

# let's dump some info to debug a bit
echo PSPDEV = $PSPDEV
echo ls = `ls`
echo pwd = `pwd`
