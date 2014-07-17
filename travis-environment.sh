#!/bin/sh -ex

# Add PSP environment variable
export PSPDEV=`pwd`/pspsdk
export PSPSDK=$PSPDEV/bin
export PATH=$PSPSDK:$PATH

# let's dump some info to debug a bit
echo PSPDEV = $PSPDEV
echo psp-config = `psp-config --psp-prefix`
echo ls = `ls`
echo pwd = `pwd`
