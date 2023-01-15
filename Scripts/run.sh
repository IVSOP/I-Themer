#!/bin/bash
STR="1\ndata/table.tb\n"
if [ $# -eq 1 ]
then
	make debug -j$(nproc)
	gdb --args themer-debug data/main_info.db 0
	
else
	make -j$(nproc)
	echo -e $STR | ./themer data/main_info.db 0
fi
