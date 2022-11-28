#!/bin/bash

#Tell the user what we're doing
echo "Building an instance of killwait.c"

#Determine list of signals (using bash kill built-in)
SIGNALS=$(kill -L | grep -Po '(?<=\s)SIG[A-Z]+[0-9]?(?<!SIGRTMIN|SIGRTMAX)(?=\s|$)' | sed -zE 's/\n/ /g;s/[^ ]+/X(&)/g;s/ $/\n/')
#Determine list of signals (using /bin/kill)
#SIGNALS=$(/bin/kill -L | grep -Po '(?<=\s)[A-Z]+[0-9]?(?<!RTMIN|RTMAX)(?=\s|$)' | sed -zE 's/\n/ /g;s/[^ ]+/X(SIG&)/g;s/ $/\n/')

#Compile
gcc -Os -s killwait.c -o killwait -DSIGNALS="$SIGNALS"
#gcc -Os -g killwait.c -o killwait -DSIGNALS="$SIGNALS"
#gcc -E killwait.c -DSIGNALS="$SIGNALS" | nano -
