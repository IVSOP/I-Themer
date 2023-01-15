#!/bin/bash

for srcFile in $(ls $1) 
do
	for testFile in $(ls $2)
	do
		if [ "$srcFile" = "$testFile" ]
		then
			echo -n "$srcFile "
		fi
	done
done
