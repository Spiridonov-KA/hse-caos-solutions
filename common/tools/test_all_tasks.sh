#!/bin/bash

set -euo pipefail

for task in $(./common/tools/list_all_tasks.sh | tac); do
    echo "Testing $task"
    (cd $task && cargo --locked xtask test --jail)
done
