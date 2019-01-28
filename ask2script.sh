#!/bin/bash

if [ $# -ne 1 ]
then
	echo "Give path"
	exit
fi

echo -n "Total number of keywords searched: "
grep "search" $1/Worker_* | cut -d ":" -f5 | grep -c search
