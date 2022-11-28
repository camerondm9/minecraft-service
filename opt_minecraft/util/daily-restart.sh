#!/bin/bash

UPTIME="$(cat /proc/uptime)"
UPTIME="${UPTIME%% *}"
if [ "$UPTIME" -lt 7200 ]; then
	echo 'Uptime is less than 2 hours, skipping scheduled restart.'
	exit 1
fi

# Schedule the restart
../bin/broadcast "say Server will restart in 5 minutes"
shutdown -r +5

# Wait until it's almost time
sleep 4m

# Warn users if the restart is still scheduled
if [ -f "/run/systemd/shutdown/scheduled" ]; then
	../bin/broadcast "say Server will restart in 1 minute"
fi
