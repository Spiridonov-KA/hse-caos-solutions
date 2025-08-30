#!/bin/bash
set -euo pipefail

if (( $# != 1 )); then
    echo "Usage: $0 <student's repo>"
    exit 1
fi

repo=$1

for task in $(./common/tools/list_all_tasks.sh); do
    if $(cd $repo; git diff --quiet HEAD~1..HEAD -- "$task"); then
        continue
    fi
    echo "Found changes in $task"
    (cd $task && cargo xtask test)
done
