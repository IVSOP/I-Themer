#!/bin/bash

make -j$(nproc) debug 1>/dev/null && \
valgrind --tool=memcheck --leak-check=yes --num-callers=20 --track-fds=yes --show-leak-kinds=all\
		--track-origins=yes  --suppressions=/usr/share/glib-2.0/valgrind/glib.supp --suppressions=Scripts/custom.supp -s --fair-sched=try\
		./bin/ithemer-debug-$@
# --gen-suppressions=yes
