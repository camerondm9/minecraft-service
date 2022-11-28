#!/bin/bash

if [ $# -gt 0 ]; then
	echo -n "Counting to $1... "
	END="$1"
	for ((i=1; i<=END; i++)); do
		LAST=$(echo "$i")
	done
	echo "$LAST"
else
	echo "Specify a count"
	exit 1
fi
