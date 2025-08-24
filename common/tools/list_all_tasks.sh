#!/bin/bash
set -euo pipefail

find tasks -type f -name 'testing.yaml' | xargs dirname | sort
