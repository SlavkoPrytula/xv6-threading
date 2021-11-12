#!/bin/bash +x

set -o errexit
set -o nounset
set -o pipefail

# Tests
echo "Executing Testing Protocol..."
echo

test_threads_01
