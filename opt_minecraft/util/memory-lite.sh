#!/bin/bash
systemctl status "minecraft@${1#mc-}" | grep 'Memory:' | grep -Po '[0-9]+\.?[0-9]*\s*[KMG]'
