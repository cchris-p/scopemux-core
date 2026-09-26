#!/bin/bash

# Verify the public `scopemux` Python package surface and its JSON CLI.
#
# The native build targets Linux/GNU tooling, so run this through the test
# container on macOS hosts:
#
#   ./scripts/docker_test.sh scripts/run_python_binding_tests.sh
#
# The script installs the package from ./core into a throwaway virtualenv and
# checks the public import surface and the console entry point. Override the
# interpreter with PYTHON_BIN (default: python3).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT_DIR}"

PYTHON_BIN="${PYTHON_BIN:-python3}"
VENV_DIR="$(mktemp -d)/scopemux-venv"

echo "[run_python_binding_tests.sh] Creating virtualenv at ${VENV_DIR}"
"${PYTHON_BIN}" -m venv "${VENV_DIR}"
# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

echo "[run_python_binding_tests.sh] Installing scopemux (editable)"
python -m pip install --quiet --upgrade pip
python -m pip install --quiet -e ./core

echo "[run_python_binding_tests.sh] Checking the public import surface"
python - <<'PY'
import scopemux

assert scopemux.__version__, "scopemux.__version__ is empty"
assert "ParserContext" in scopemux.__all__
assert "detect_language" in scopemux.__all__
assert "scopemux_core" not in scopemux.__all__

parser = scopemux.ParserContext()
parser.parse_string("def add(a, b):\n    return a + b\n", filename="add.py")
root = parser.get_ast_root()
assert root is not None, "expected an AST root for the parsed sample"
print("import surface OK:", scopemux.__version__)
PY

echo "[run_python_binding_tests.sh] Checking the JSON CLI"
python - <<'PY'
import json
import subprocess
import sys


def run(*args):
    out = subprocess.run(
        ["scopemux", *args],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(out.stdout)


version = run("version")
assert version["component"] == "scopemux-core", version
assert version["version"], version

detected = run("detect", "README.md")
assert detected["language"] == "unknown", detected

parsed = run("parse", "core/src/bindings/module.c", "--language", "c")
assert "error" not in parsed, parsed
assert parsed["root_type"], parsed

module = subprocess.run(
    [sys.executable, "-m", "scopemux", "version"],
    check=True,
    capture_output=True,
    text=True,
)
assert json.loads(module.stdout)["version"], module.stdout

print("CLI JSON OK:", version, detected, parsed["root_type"])
PY

echo "[run_python_binding_tests.sh] ALL TESTS PASSED"
