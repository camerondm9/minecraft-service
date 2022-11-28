#!/bin/bash

#Resolve all process IDs
PIDS=()
for var in "$@"; do
	if [[ "$var" =~ ^[0-9]+$ ]]; then
		PIDS+=("$var")
	else
		readarray -t -O ${#PIDS[@]} PIDS < <(pgrep -f "${var%/}")
	fi
done
if [ ${#PIDS[@]} -eq 0 ]; then
	echo 'Specify a process ID or a command-line fragment!'
	exit 1
fi

#Include all child processes
for var in "${PIDS[@]}"; do
	readarray -t -O ${#PIDS[@]} PIDS < <(pstree -pT "$var" | grep -Po '(?<=\()[0-9]+(?=\))')
done

#Remove duplicates
PIDS=($(printf "%s\n" "${PIDS[@]}" | sort -u));

#Print list of process IDs
#echo "${PIDS[@]}"

#Determine total PSS memory usage
TOTAL=0
for var in "${PIDS[@]}"; do
	while read -r pss; do
		TOTAL=$(($TOTAL + $pss))
	done < <(grep -Po '(?<=^Pss:)\s+[0-9]+(?= kB$)' 2>/dev/null </proc/$var/smaps)
done

#Print total memory usage
if [ "${#TOTAL}" -ge 8 ]; then
	echo "$(($TOTAL / 1000000)) GB"
elif [ "${#TOTAL}" -ge 5 ]; then
	echo "$(($TOTAL / 1000)) MB"
else
	echo "$TOTAL kB"
fi
