#!/bin/sh
find "$1" -iname "*.png" -exec pngcrush -ow -rem -allb -reduce "{}" \;
