#!/bin/bash

exec /usr/bin/java -server -Xms1024M -Xmx2048M -XX:+UseG1GC -XX:ParallelGCThreads=2 -XX:MinHeapFreeRatio=5 -XX:MaxHeapFreeRatio=10 -jar minecraft_server.jar nogui
