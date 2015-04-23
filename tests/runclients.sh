#!/bin/sh

DIR="/tmp"

EXEC="../getmyfiles-client $1 $2 $DIR"

run_proc() {
#(
    $EXEC &
    local pid=$!
    sleep 180
    kill -9 $pid
    echo "killed"
#) &
}

COUNT=10000

while [ ! $COUNT -eq 0 ]; do
    echo $COUNT
    COUNT=$((COUNT -1))
    run_proc &
    sleep 0.01
done

#sleep 100
