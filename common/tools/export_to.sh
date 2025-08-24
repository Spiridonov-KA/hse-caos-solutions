#!/bin/bash
set -euo pipefail

if [[ $# != 1 ]]; then
    echo "Usage: $0 <public_dir>"
    exit 1
fi

cargo xtask compose --in-path . --out-path $1 --skip .git --skip target --skip .cargo-home --stat
