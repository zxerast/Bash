#!/bin/bash
while read line; do
    echo "Processing: $line" >&2
    echo "$line"
    sleep 0.1
done
