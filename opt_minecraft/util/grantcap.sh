#!/bin/bash

if [ $# -eq 0 ]; then
	echo 'Specify a script file. Capabilities can also be specified.'
	exit 1
fi

#Resolve script as absolute path (for security reasons)
SCRIPT=$(realpath "$1")
NAME=$(basename "$1" .sh)

#Format capabilities consistently...
shift
CAPS=''
for var in "$@"; do
	var="${var^^}"
	[[ "$var" == CAP_* ]] || var="CAP_$var"
	[[ "$var" == *, ]] && var="${var%,}"
	[[ -z "$CAPS" ]] || CAPS="$CAPS, "
	CAPS="$CAPS$var"
done

#Tell the user what we're doing
echo "Building an instance of grantcap.c:"
echo "        Name: $NAME"
echo "      Target: /bin/bash $SCRIPT"
echo "Capabilities: $CAPS"

#Compile
gcc -Os -s grantcap.c -lcap -o "$NAME" -DTARGET='"/bin/bash"' -DARGS_PREFIX="\"$SCRIPT\"" -DCAPS="$CAPS"
