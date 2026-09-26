"""Structured JSON command-line surface for the ScopeMux native core.

The CLI is a secondary, in-scope surface for CI and pipelines. Every command
writes a single JSON object to stdout so callers can parse results without
depending on the Python API. A non-zero exit code signals a failed command, and
the failure is reported as a JSON object with an ``error`` key.
"""

from __future__ import annotations

import argparse
import json
import sys
from typing import Any, Optional, Sequence

from scopemux_core import (
    LANG_C,
    LANG_CPP,
    LANG_JAVASCRIPT,
    LANG_PYTHON,
    LANG_TYPESCRIPT,
    LANG_UNKNOWN,
    ParserContext,
    __version__,
    detect_language,
)

_LANGUAGE_NAMES = {
    LANG_UNKNOWN: "unknown",
    LANG_C: "c",
    LANG_CPP: "cpp",
    LANG_PYTHON: "python",
    LANG_JAVASCRIPT: "javascript",
    LANG_TYPESCRIPT: "typescript",
}

_LANGUAGE_IDS = {
    "c": LANG_C,
    "cpp": LANG_CPP,
    "c++": LANG_CPP,
    "python": LANG_PYTHON,
    "py": LANG_PYTHON,
    "javascript": LANG_JAVASCRIPT,
    "js": LANG_JAVASCRIPT,
    "typescript": LANG_TYPESCRIPT,
    "ts": LANG_TYPESCRIPT,
}


def _language_name(value: int) -> str:
    return _LANGUAGE_NAMES.get(value, "unknown")


def _cmd_version(args: argparse.Namespace) -> dict[str, Any]:
    return {"component": "scopemux-core", "version": __version__}


def _cmd_detect(args: argparse.Namespace) -> dict[str, Any]:
    value = detect_language(args.path)
    return {"path": args.path, "language": _language_name(value), "language_id": value}


def _cmd_parse(args: argparse.Namespace) -> dict[str, Any]:
    parser = ParserContext()
    language_id = None
    if args.language is not None:
        language_id = _LANGUAGE_IDS.get(args.language.lower())
        if language_id is None:
            raise ValueError(f"unsupported language: {args.language}")

    if language_id is None:
        parser.parse_file(args.path)
    else:
        parser.parse_file(args.path, language=language_id)

    root = parser.get_ast_root()
    return {
        "path": args.path,
        "root_type": root.get_type() if root is not None else None,
        "root_name": root.get_name() if root is not None else None,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="scopemux",
        description="Inspect source files with the ScopeMux native core (JSON output).",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("version", help="Print the core version as JSON.").set_defaults(
        func=_cmd_version
    )

    detect = subparsers.add_parser(
        "detect", help="Detect the language of a file and print it as JSON."
    )
    detect.add_argument("path", help="Path to the source file to inspect.")
    detect.set_defaults(func=_cmd_detect)

    parse = subparsers.add_parser(
        "parse", help="Parse a file and print the AST root summary as JSON."
    )
    parse.add_argument("path", help="Path to the source file to parse.")
    parse.add_argument(
        "--language",
        default=None,
        help="Override language detection (for example: python, c, cpp).",
    )
    parse.set_defaults(func=_cmd_parse)

    return parser


def _emit(payload: dict[str, Any]) -> None:
    json.dump(payload, sys.stdout, sort_keys=True)
    sys.stdout.write("\n")


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        result = args.func(args)
    except Exception as exc:  # noqa: BLE001 - report any failure as JSON
        _emit({"error": str(exc)})
        return 1
    _emit(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
