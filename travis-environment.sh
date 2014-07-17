#!/bin/bash 
set -ex

# Add PSP environment variable
export "PSPDEV=$TRAVIS_BUILD_DIR/pspdev"
export "PSPSDK=$PSPDEV/psp"
export "PATH=$PATH:$PSPDEV/bin:$PSPSDK/bin"

# let's dump some info to debug a bit
echo PSPDEV = $PSPDEV
echo ls = `ls`
echo pwd = `pwd`
