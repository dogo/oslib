#!/bin/bash

if [[ $TRAVIS_OS_NAME == "osx" ]] ; then
  brew update
	brew install mpfr
	brew install gmp
	brew install libelf
	brew install libmpc
	brew install libusb
fi