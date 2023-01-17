#!/bin/bash

if [ $# -eq 1 ]
then
	make debug -j$(nproc)
	gdb --args themer-debug
	
else
	make -j$(nproc)
	./themer
fi
