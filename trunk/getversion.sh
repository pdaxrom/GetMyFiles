#!/bin/sh

cat ${1}debian/changelog | head -n1 | grep "(*.)" | sed 's/.* (\{1\}\(.*\))\{1,\}.*$/\1/'

