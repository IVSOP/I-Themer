#!/bin/bash

if [ $# -eq 1 ]
then
	make debug -j$(nproc)
	gdb --args themer-debug data/table.tb
	
else
	make -j$(nproc)
	./themer data/table.tb
fi
