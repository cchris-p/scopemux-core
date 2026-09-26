"""Enable ``python -m scopemux`` as an alias for the ``scopemux`` command."""

from scopemux.cli import main

if __name__ == "__main__":
    raise SystemExit(main())
