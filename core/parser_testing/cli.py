"""
Command-Line Interface

This module provides the command-line interface for the parser testing framework,
allowing users to generate expected JSON output for ScopeMux test cases.
"""

import os
import sys
import argparse
import gc
import signal
from typing import Dict, Any, List, Optional, Tuple

from .generators import JSONGenerator
from .converters import ASTConverter, CSTConverter
from .utils import (
    read_file,
    write_json_file,
    compare_files,
    get_expected_output_path,
    find_source_files,
    generate_json_diff,
    highlight_diff,
    safe_process_with_gc,
)

# Try to register a segfault handler if one isn't already set
# This helps prevent crashes if the scopemux_core module has issues
if signal.getsignal(signal.SIGSEGV) == signal.SIG_DFL:

    def cli_segfault_handler(signum, frame):
        print("Error: Segmentation fault detected in CLI")
        sys.exit(1)

    signal.signal(signal.SIGSEGV, cli_segfault_handler)


def parse_args() -> argparse.Namespace:
    """
    Parse command-line arguments.

    Returns:
        The parsed arguments
    """
    parser = argparse.ArgumentParser(
        description="Generate expected JSON output for ScopeMux test cases",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument("source_path", help="Source file or directory to process")
    parser.add_argument("--output-dir", help="Directory to write output files")
    parser.add_argument(
        "--mode",
        choices=["ast", "cst", "both"],
        default="both",
        help="Parse mode (default: both)",
    )
    parser.add_argument(
        "--update", action="store_true", help="Update existing .expected.json files"
    )
    parser.add_argument(
        "--review",
        action="store_true",
        help="Show diff before updating (implies --update)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="Don't actually write files"
    )
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument(
        "--rebuild",
        action="store_true",
        help="Run build_all_and_pybind.sh before starting",
    )

    args = parser.parse_args()

    # If review is specified, update is implied
    if args.review:
        args.update = True

    return args


def rebuild_bindings():
    """
    Rebuild the Python bindings using build_all_and_pybind.sh.

    Returns:
        True if successful, False otherwise
    """
    import subprocess
    import os

    # Get the project root directory
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))

    # Check if the build script exists
    build_script = os.path.join(project_root, "build_all_and_pybind.sh")
    if not os.path.isfile(build_script):
        print(f"Error: Build script not found at {build_script}")
        return False

    # Run the build script
    try:
        print(f"Running {build_script}...")
        result = subprocess.run(
            [build_script],
            cwd=project_root,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        print("Build completed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: Build failed with code {e.returncode}")
        print(f"Output: {e.stdout}")
        print(f"Error: {e.stderr}")
        return False
    except Exception as e:
        print(f"Error: Failed to run build script: {e}")
        return False


def process_file(
    file_path: str,
    generator: JSONGenerator,
    mode: str = "both",
    output_dir: Optional[str] = None,
    update: bool = False,
    review: bool = False,
    dry_run: bool = False,
    verbose: bool = False,
    root_dir: Optional[str] = None,
) -> bool:
    """
    Process a single source file and generate its expected JSON output.

    Args:
        file_path: Path to the source file
        generator: The JSON generator to use
        mode: Parse mode ("ast", "cst", or "both")
        output_dir: Directory to write output file (default: same as input)
        update: Whether to update existing files
        review: Whether to show diffs before updating
        dry_run: Don't actually write files
        verbose: Print verbose output
        root_dir: Root directory for preserving subdirectory structure

    Returns:
        True if successful, False otherwise
    """
    if verbose:
        print(f"Processing {file_path}...")

    # Determine output path
    output_path = get_expected_output_path(file_path, output_dir, root_dir)

    # Check if output file exists
    file_exists = os.path.exists(output_path)
    if file_exists and not update:
        print(f"Output file {output_path} already exists. Use --update to overwrite.")
        return False

    # Generate JSON representation
    try:
        # Use the memory-safe processing function
        result_dict = safe_process_with_gc(
            generator.generate_from_file, file_path, mode
        )

        # If no result, return failure
        if not result_dict:
            print(f"Failed to parse {file_path}")
            return False

        # Check if there was an error during processing
        if (
            "error" in result_dict
            and result_dict.get("ast") is None
            and result_dict.get("cst") is None
        ):
            print(f"Error processing {file_path}: {result_dict['error']}")
            return False

        # Show diff if requested and file exists
        if review and file_exists:
            try:
                existing_data = {}
                with open(output_path, "r") as f:
                    import json

                    existing_data = json.load(f)

                diff = generate_json_diff(existing_data, result_dict)

                if diff:
                    print("\nDiff for", output_path)
                    print(highlight_diff(diff))
                    print()
                else:
                    print(f"No changes for {output_path}")
            except Exception as e:
                print(f"Error comparing files: {e}")

        # Write the output file
        if not dry_run:
            try:
                write_json_file(output_path, result_dict)
                if verbose:
                    print(f"Wrote {output_path}")
                return True
            except Exception as e:
                print(f"Error writing {output_path}: {e}")
                return False
        else:
            if verbose:
                print(f"Would write {output_path} (dry run)")
            return True

    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        import traceback

        traceback.print_exc()
        return False


def process_directory(
    dir_path: str,
    generator: JSONGenerator,
    output_dir: Optional[str] = None,
    mode: str = "both",
    update: bool = False,
    review: bool = False,
    dry_run: bool = False,
    verbose: bool = False,
) -> Tuple[int, int]:
    """
    Process all source files in a directory and its subdirectories.

    Args:
        dir_path: Path to the directory
        generator: The JSON generator to use
        output_dir: Directory to write output files
        mode: Parse mode ("ast", "cst", or "both")
        update: Whether to update existing files
        review: Whether to show diffs before updating
        dry_run: Don't actually write files
        verbose: Print verbose output

    Returns:
        Tuple of (success_count, failure_count)
    """
    success_count = 0
    failure_count = 0

    # Find all source files
    supported_extensions = [".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts"]
    file_paths = find_source_files(dir_path, supported_extensions)

    # Process each file
    for file_path in file_paths:
        # Always pass root_dir as dir_path for correct relative path computation
        result = process_file(
            file_path,
            generator,
            mode=mode,
            output_dir=output_dir,
            update=update,
            review=review,
            dry_run=dry_run,
            verbose=verbose,
            root_dir=dir_path,
        )

        if result:
            success_count += 1
        else:
            failure_count += 1

    return success_count, failure_count


def main() -> int:
    """
    Main entry point for the CLI.

    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    print("Starting main()")
    # Parse command-line arguments
    args = parse_args()

    # Rebuild bindings if requested
    if args.rebuild:
        if not rebuild_bindings():
            print("Error: Failed to rebuild bindings")
            return 1

    # Create converters and generator
    ast_converter = ASTConverter()
    cst_converter = CSTConverter()
    generator = JSONGenerator(ast_converter, cst_converter)

    # Process the source path
    if os.path.isdir(args.source_path):
        print(f"Processing directory: {args.source_path}")
        success, failure = process_directory(
            args.source_path,
            generator,
            output_dir=args.output_dir,
            mode=args.mode,
            update=args.update,
            review=args.review,
            dry_run=args.dry_run,
            verbose=args.verbose,
        )
        print(
            f"Processed {success + failure} files: {success} succeeded, {failure} failed."
        )
        return 0 if failure == 0 else 1
    else:
        # Process a single file
        result = process_file(
            args.source_path,
            generator,
            mode=args.mode,
            output_dir=args.output_dir,
            update=args.update,
            review=args.review,
            dry_run=args.dry_run,
            verbose=args.verbose,
        )
        print("Success" if result else "Failed")
        return 0 if result else 1

    print("Before cleanup")
    # cleanup code
    print("After cleanup")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"Error in main execution: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
    finally:
        # Always run cleanup
        print("Performing final cleanup before exit...")
        gc.collect()
        print("Program exiting")
