#!/bin/sh

DIR="/tmp"

EXEC="../getmyfiles-client $DIR"

run_proc() {
#(
    $EXEC &
    local pid=$!
    sleep 180
    kill -9 $pid
    echo "killed"
#) &
}

COUNT=100

while [ ! $COUNT -eq 0 ]; do
    echo $COUNT
    COUNT=$((COUNT -1))
    run_proc &
    sleep 0.02
done

#sleep 100
