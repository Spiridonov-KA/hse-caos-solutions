#!/bin/bash

set -euo pipefail

for task in $(./common/tools/list_all_tasks.sh); do
    echo "Testing $task"
    (cd $task && cargo xtask test --build-only)
done
