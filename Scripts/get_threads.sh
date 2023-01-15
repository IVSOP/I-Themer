#!/bin/bash

if ! command -v nproc &> /dev/null
then
	sysctl -n hw.ncpu
else
    nproc 
fi
