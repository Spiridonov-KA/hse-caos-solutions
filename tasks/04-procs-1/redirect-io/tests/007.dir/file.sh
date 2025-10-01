#! /bin/bash

open_files=$(find /proc/$$/fd -exec readlink {} \+)
# count only reopened stdin/stdout and their duplicates
open_cnt=$(echo "$open_files" | grep -c "$(readlink /proc/$$/fd/0)\|$(readlink /proc/$$/fd/1)")

if [ "$open_cnt" != 2 ]; then
    echo fd leak detected >&2
    kill $(grep -i ppid /proc/$$/status | cut -f 2)
    exit 1
fi

echo ok
