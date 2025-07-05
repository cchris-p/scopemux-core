"""
JSON Generator

This module provides functionality for generating JSON representations of
AST and CST structures from source code files.
"""

import os
import gc
import json
import sys
import ctypes
import signal
from typing import Dict, Any, Optional, Tuple, Union

from ..converters import ASTConverter, CSTConverter


class JSONGenerator:
    """
    Generator for creating JSON representations of parsed source code.

    This class handles the parsing of source code files using ScopeMux's
    parser bindings and the generation of canonical JSON representations
    of their AST/CST structures.
    """

    def __init__(
        self,
        ast_converter: Optional[ASTConverter] = None,
        cst_converter: Optional[CSTConverter] = None,
    ):
        """
        Initialize a JSON generator.

        Args:
            ast_converter: The AST converter to use
            cst_converter: The CST converter to use
        """
        self.ast_converter = ast_converter or ASTConverter()
        self.cst_converter = cst_converter or CSTConverter()
        self._scopemux_core = None

        # Register a dummy segfault handler to avoid crashes
        self._register_segfault_handler()

    def _register_segfault_handler(self):
        """Register a segfault handler to prevent crashes."""
        # Only register if there isn't already a handler
        if signal.getsignal(signal.SIGSEGV) == signal.SIG_DFL:

            def generator_segfault_handler(signum, frame):
                print("Warning: Segmentation fault detected in JSON generator")
                # Raise an exception instead of crashing
                raise RuntimeError("Segmentation fault detected")

            signal.signal(signal.SIGSEGV, generator_segfault_handler)

    def _import_scopemux_core(self):
        """
        Import the scopemux_core module with proper error handling.

        This method handles the initialization of the scopemux_core module,
        ensuring that all necessary symbols are available.

        Returns:
            The imported scopemux_core module

        Raises:
            ImportError: If the module cannot be imported or initialized
        """
        if self._scopemux_core is not None:
            return self._scopemux_core

        # Try only the new build/core path
        possible_paths = [
            os.path.abspath(
                os.path.join(os.path.dirname(__file__), "../../../build/core")
            ),
            None,  # None will use the default Python path
        ]

        original_error = None
        for path in possible_paths:
            try:
                if path is not None and path not in sys.path:
                    sys.path.insert(0, path)

                # Clear any previous import errors and try again
                if "scopemux_core" in sys.modules:
                    del sys.modules["scopemux_core"]

                # Before importing, try to preload the segfault_handler symbol
                self._ensure_segfault_handler()

                # Try to import the module
                import scopemux_core

                # Store for future use
                self._scopemux_core = scopemux_core
                return scopemux_core

            except ImportError as e:
                if original_error is None:
                    original_error = e
                # Try next path
                continue
            except Exception as e:
                # Other exceptions are unexpected and should be raised
                raise ImportError(f"Error importing scopemux_core: {str(e)}") from e

        # If we get here, all import attempts failed
        raise ImportError(
            "Failed to import scopemux_core. Make sure the ScopeMux core Python bindings are installed.\n"
            "You may need to run 'build_all_and_pybind.sh' from the project root directory."
        ) from original_error

    def _ensure_segfault_handler(self):
        """
        Ensure the segfault_handler symbol is available.

        This function tries to locate the shared library and define the
        segfault_handler symbol if it's missing.
        """
        # Try to find the .so file
        project_root = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "../../..")
        )
        possible_so_paths = [
            os.path.join(
                project_root, "build/core/scopemux_core.cpython-310-x86_64-linux-gnu.so"
            ),
        ]

        for so_path in possible_so_paths:
            if os.path.exists(so_path):
                try:
                    # Try to load the library directly
                    lib = ctypes.CDLL(so_path)

                    # Check if segfault_handler is already defined
                    try:
                        lib.segfault_handler
                        print(f"Found segfault_handler in {so_path}")
                        return
                    except AttributeError:
                        # If not found, define it
                        print(f"Defining segfault_handler in {so_path}")

                        # Define a simple C function for segfault handling
                        @ctypes.CFUNCTYPE(None, ctypes.c_int)
                        def segfault_handler_func(sig):
                            print(f"Caught signal {sig} in custom segfault handler")
                            return

                        # Try to add our function to the library
                        try:
                            setattr(lib, "segfault_handler", segfault_handler_func)
                            return
                        except Exception as e:
                            print(f"Could not set segfault_handler: {e}")
                except Exception as e:
                    print(f"Error loading {so_path}: {e}")

        print("Warning: Could not find scopemux_core.so to patch")

    def generate_from_file(self, file_path: str, mode: str = "both") -> Dict[str, Any]:
        """
        Generate JSON representation from a source file.

        Args:
            file_path: The path to the source file
            mode: Parse mode ('ast', 'cst', or 'both')

        Returns:
            A dictionary containing the JSON representation
        """
        # Import scopemux_core module
        try:
            scopemux_core = self._import_scopemux_core()
        except ImportError as e:
            print(f"Import error: {e}")
            # Return a minimal structure as fallback
            return {"language": "Unknown", "ast": None, "cst": None, "error": str(e)}

        # Initialize parser
        parser = None
        ast_dict = None
        cst_dict = None
        lang_name = "Unknown"

        try:
            parser = scopemux_core.ParserContext()

            # Detect language
            detected_lang = scopemux_core.detect_language(file_path)
            lang_name = self._get_language_name(detected_lang, file_path)

            # Read the file content
            if not os.path.exists(file_path):
                raise FileNotFoundError(f"File not found: {file_path}")

            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()

            # Parse the file
            success = False
            if hasattr(parser, "parse_string"):
                success = parser.parse_string(content, file_path, detected_lang)
            elif hasattr(parser, "parse_file"):
                success = parser.parse_file(file_path, detected_lang)
            else:
                raise RuntimeError("No suitable parse method found on ParserContext")

            if not success:
                get_err = getattr(parser, "get_last_error", lambda: "Unknown error")
                raise RuntimeError(f"Error parsing {file_path}: {get_err()}")

            # Process AST if needed
            if mode in ("ast", "both"):
                if not hasattr(parser, "get_ast_root"):
                    raise RuntimeError("Parser bindings do not expose get_ast_root()")

                ast_root = parser.get_ast_root()
                if not ast_root:
                    raise RuntimeError(f"Parser did not return AST for {file_path}")

                # Convert the AST to a dictionary
                ast_dict = self.ast_converter.convert(ast_root, detected_lang)

                # Explicitly dereference to help garbage collection
                ast_root = None

            # Process CST if needed
            if mode in ("cst", "both"):
                if not hasattr(parser, "get_cst_root"):
                    raise RuntimeError("Parser bindings do not expose get_cst_root()")

                cst_root = parser.get_cst_root()
                if cst_root is None:
                    # Create a minimal valid structure
                    cst_dict = {
                        "type": "ROOT",
                        "content": "",
                        "range": {
                            "start": {"line": 0, "column": 0},
                            "end": {"line": 0, "column": 0},
                        },
                        "children": [],
                    }
                else:
                    # Convert the CST to a dictionary
                    cst_dict = self.cst_converter.convert(cst_root, detected_lang)

                # Explicitly dereference to help garbage collection
                cst_root = None

            # Generate combined JSON
            result = self.generate_combined_json(ast_dict, cst_dict, lang_name)

            # Force garbage collection
            gc.collect()

            return result
        except Exception as e:
            print(f"Error processing {file_path}: {str(e)}")
            return {"language": lang_name, "ast": None, "cst": None, "error": str(e)}
        finally:
            # Ensure parser is explicitly deleted to trigger cleanup
            if parser is not None:
                parser = None
                gc.collect()

    def generate_combined_json(
        self,
        ast_dict: Optional[Dict[str, Any]],
        cst_dict: Optional[Dict[str, Any]],
        language: Optional[str] = None,
    ) -> Dict[str, Any]:
        """
        Generate a combined JSON structure containing both AST and CST data.

        Args:
            ast_dict: The AST dictionary
            cst_dict: The CST dictionary
            language: The language name

        Returns:
            A dictionary containing the combined JSON representation
        """
        # Ensure language is always properly set
        effective_language = language

        # If language is not provided or is Unknown, try to determine from AST
        if not effective_language or effective_language == "Unknown":
            effective_language = ast_dict.get("language") if ast_dict else None

        # If still not determined, use a reasonable default
        if not effective_language or effective_language == "Unknown":
            effective_language = "C"  # Default to C if unknown

        # Create the combined JSON with the canonical structure
        result = {
            "language": effective_language,
            "ast": ast_dict,
            "cst": cst_dict,
        }

        return result

    def _get_language_name(self, language_type: int, file_path: str = None) -> str:
        """
        Get the human-readable language name from a language type.
        Falls back to determining language from file extension if language_type is unknown.

        Args:
            language_type: The language type identifier
            file_path: Optional file path to determine language from extension

        Returns:
            The human-readable language name
        """
        # This needs to be kept in sync with the LANG_MAPPING in scopemux_core
        language_map = {
            0: "Unknown",
            1: "C",
            2: "C++",
            3: "Python",
            4: "JavaScript",
            5: "TypeScript",
        }

        language_name = language_map.get(language_type, "Unknown")

        # If language is unknown and we have a file path, try to determine from extension
        if (language_name == "Unknown" or language_type == 0) and file_path:
            ext = os.path.splitext(file_path)[1].lower()
            ext_map = {
                ".c": "C",
                ".h": "C",
                ".cpp": "C++",
                ".hpp": "C++",
                ".cc": "C++",
                ".py": "Python",
                ".js": "JavaScript",
                ".jsx": "JavaScript",
                ".ts": "TypeScript",
                ".tsx": "TypeScript",
            }
            if ext in ext_map:
                return ext_map[ext]

        return language_name
