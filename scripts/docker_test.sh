#!/usr/bin/env bash
#
# Run scopemux-core test scripts inside a Linux container.
#
# The native test harness relies on Linux/GNU tooling, so this containerizes the
# build/test run. The current working tree is copied into the container, so the
# run tests the source as it exists on disk (including uncommitted changes); the
# host checkout is never modified.
#
# Usage:
#   scripts/docker_test.sh                       # C, C++, and interfile tests
#   scripts/docker_test.sh scripts/run_c_tests.sh
#   scripts/docker_test.sh <script> [<script>...]
#
# Override the image name with SCOPEMUX_TEST_IMAGE.

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${SCOPEMUX_TEST_IMAGE:-scopemux-core-test}"

if [ "$#" -gt 0 ]; then
    scripts=("$@")
else
    scripts=(scripts/run_c_tests.sh scripts/run_cpp_tests.sh scripts/run_interfile_tests.sh)
fi

echo "[docker_test] Building image '${IMAGE}'..."
docker build -t "${IMAGE}" -f "${PROJECT_ROOT}/docker/Dockerfile.test" "${PROJECT_ROOT}"

echo "[docker_test] Running: ${scripts[*]}"
COPYFILE_DISABLE=1 tar -C "${PROJECT_ROOT}" \
    --exclude=.git \
    --exclude=build \
    --exclude='build-*' \
    --exclude='*.egg-info' \
    --exclude='*/._*' \
    --exclude='._*' \
    -cf - . 2>/dev/null |
    docker run --rm -i "${IMAGE}" bash -lc '
        set -e
        mkdir -p /work
        tar -C /work -xf -
        cd /work
        for s in "$@"; do
            echo "=== ${s} ==="
            bash "${s}"
        done
    ' bash "${scripts[@]}"
