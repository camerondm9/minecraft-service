#!/bin/bash

if [ $EUID -ne 0 ]; then
	echo 'Please run as root!'
	exit 1
fi

# Install required packages and configure automatic security updates
if apt --help >/dev/null 2>&1; then
	apt update
	apt upgrade
	apt install ufw unattended-upgrades screen mosh gcc
	systemctl enable apt-daily.timer
	dpkg-reconfigure --priority=low unattended-upgrades

else
	echo "Unknown package manager!"
	exit 2
fi

# Enable firewall (TCP for main connection, UDP for query protocol)
if ufw --help >/dev/null 2>&1; then
	ufw limit ssh
	ufw allow 25565 #For Minecraft
	ufw allow 60001:60010/udp #For mosh
	ufw enable
	ufw status
else
	echo "Unknown firewall!"
	exit 1
fi

# Display final instructions
echo "Firewall has been configured and necessary dependencies are now installed!"
echo "Reminder that while you should use a regular (non-root) user under normal circumstances, you may want to set up a root login key for emergencies."
echo "Next, run ./setup_minecraft.sh"
