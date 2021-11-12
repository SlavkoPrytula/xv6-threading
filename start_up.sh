#!/bin/bash +x

set -o errexit
set -o nounset
set -o pipefail

# INFO
echo "Found files:"
ls
echo
echo

# shellcheck disable=SC2164
cd "./xv6-public"


# QEMU
echo "Executing Make..."
make -B
echo

echo "Starting xv6..."
echo
make qemu
