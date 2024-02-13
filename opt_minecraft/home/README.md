# Minecraft server instance setup
1. Create a subfolder here for your server. Give the folder a useful name, like "vanilla_survival" or the name of your modpack.
2. Place your server files within the new folder, and run the install script (if there is one).
3. Create a `start.sh` script within the new folder, if there isn't one (use the vanilla example file as a reference).
4. Make sure that your `start.sh` script ends in a line that uses `exec` to run Java (and Minecraft). This will reduce memory consumption a tiny bit and ensure that the SIGINT shutdown signal will be routed to the Minecraft server correctly.
5. Install the systemd service instance by running this command as root:
    ```bash
    /opt/minecraft/bin/install NAME_OF_YOUR_FOLDER
    ```
6. Start the server by running this command as root:
    ```bash
    /opt/minecraft/bin/start NAME_OF_YOUR_FOLDER
    ```
7. If you haven't accepted the EULA yet, you'll need to do that now. Go edit the `eula.txt` file to indicate `eula=true`, then try starting the server again.
8. Let the server run for a little bit so it can create important files, like `server.properties`.
9. Stop the server by running this command as root:
    ```bash
    /opt/minecraft/bin/stop NAME_OF_YOUR_FOLDER
    ```
10. Edit `server.properties` to include this setting to enable RCON. This will allow the `broadcast` command to control your server while it's running (to send "The server is shutting down" warnings, etc.). DO NOT expose the RCON port through the firewall or else your server will get hacked.
    ```properties
    enable-rcon=true
    ```
11. Also edit `server.properties` to configure Minecraft as you wish. This is also a good time to edit mod configs, etc. Memory config for Java can be set in your `start.sh` script.
12. If you changed anything even remotely related to world generation (especially world type or mod configs), delete the Minecraft save folder.
13. Start the server again. It will use your new configuration.
14. You can connect to the server console by running this command:
    ```bash
    /opt/minecraft/bin/console NAME_OF_YOUR_FOLDER
    ```
15. Disconnect from the server console by pressing `CTRL+A`, and then `D`.
16. You can also check the status of your server by running this command:
    ```bash
    /opt/minecraft/bin/status NAME_OF_YOUR_FOLDER
    ```
17. Final note: if you login to the `minecraft` user, then all these commands are available in your `PATH` environment variable, so you won't need the `/opt/minecraft/bin/` prefix. Then you can just run `console` or `status` or `broadcast`. Note however that the `minecraft` user doesn't have enough privileges to run the `install`, `start`, and `stop` commands. (in the future, this might be possible using a custom `PolicyKit` script)
18. Additional things you may want to configure:
    - User whitelist
    - User command permission levels (ops, etc.)
    - If you're running a modded server, consider setting up the FTB *mods* for easier administration of backups, teleports, etc.
    - Ports (in `server.properties` and in the `ufw` firewall), if you're going to run multiple servers at the same time
