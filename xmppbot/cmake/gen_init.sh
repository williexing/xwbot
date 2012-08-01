#!/bin/sh

find ./ -type f -name "*.c" | xargs grep PLUGIN_INIT | sed -n 's/.*PLUGIN_INIT//p' | sed 's/[();]//g' | while read p; do echo $p"();";done > $1 

