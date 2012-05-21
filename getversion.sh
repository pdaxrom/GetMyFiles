#!/bin/sh

cat ${1}debian/changelog | grep "(*.)" | sed 's/.* (\{1\}\(.*\))\{1,\}.*$/\1/'

