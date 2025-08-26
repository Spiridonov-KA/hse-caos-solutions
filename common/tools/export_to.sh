#!/bin/bash
set -euo pipefail

if (( $# < 1 )); then
    echo "Usage: $0 <public_dir> [extra-args ...]"
    exit 1
fi

public_dir=$1
shift

cargo xtask compose \
    --in-path . \
    --out-path $public_dir \
    --skip .git \
    --skip target \
    --skip .cargo-home \
    --stat \
    $@
