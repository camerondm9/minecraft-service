#!/bin/bash

AVAIL_MB="$(df --output=avail --block-size=1MiB /opt/minecraft/home | grep -Po '[0-9]+')"
if [ "$AVAIL_MB" -lt 1024 ]; then
	../bin/broadcast "say §4Server has $AVAIL_MB MiB remaining disk space!"
elif [ "$AVAIL_MB" -lt 6144 ]; then
	if [ "$(date +%M)" -lt 20 ]; then
		../bin/broadcast "say §6Server has $AVAIL_MB MiB remaining disk space."
	fi
fi
