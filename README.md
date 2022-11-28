# Minecraft service setup
Scripts and programs for setting up a Minecraft server on Ubuntu (probably works on Debian too, and maybe other distros that use systemd).
Won't work on Fedora/RedHat/Oracle Linux unless exceptions are created for SELinux. I don't have time to figure this out right now.

## Goal
- To fully integrate Minecraft with the system so it starts and stops smoothly like any other service.
  - None of the scripts I found online seemed to be able to do this, so I made my own.
- Additionally, to make server administration easier by keeping useful logs, etc.
