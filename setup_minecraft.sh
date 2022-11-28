#!/bin/bash

if [ $EUID -ne 0 ]; then
	echo 'Please run as root!'
	exit 1
fi

# Create user
useradd --system --no-create-home --home-dir /opt/minecraft/home --user-group minecraft

# Move files into place
mv opt_minecraft /opt/minecraft
ln -s /opt/minecraft/util/minecraft@.service /etc/systemd/system/minecraft@.service
ln -s /opt/minecraft/util/daily-restart.service /etc/systemd/system/daily-restart.service
ln -s /opt/minecraft/util/daily-restart.timer /etc/systemd/system/daily-restart.timer

# Set permissions
chown -R root:root /opt/minecraft
chmod -R 644 /opt/minecraft
chmod -R +X /opt/minecraft
chmod +x /opt/minecraft/util/*.sh
chmod +x /opt/minecraft/bin/*

# Compile programs
pushd /opt/minecraft/util
gcc -Os fatalitee.c -o fatalitee
./killwait.sh
# wget https://raw.githubusercontent.com/Tiiffi/mcrcon/master/mcrcon.c
gcc -std=gnu99 -Wall -Wextra -Wpedantic -Os -s mcrcon.c -o mcrcon
popd

# Reload systemd
systemctl daemon-reload
systemctl enable daily-restart.timer
systemctl start daily-restart.timer

# Display final instructions
echo "Minecraft service components have been set up."
echo "Create a service instance by following the instructions in /opt/minecraft/home/README.md"
