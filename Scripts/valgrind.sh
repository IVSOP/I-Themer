#!/bin/bash

make -j$(nproc) debug 1>/dev/null && \
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes\
		--track-origins=yes --suppressions=/usr/share/glib-2.0/valgrind/glib.supp -s --fair-sched=try\
		./themer-debug $@
