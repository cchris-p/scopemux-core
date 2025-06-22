"""
Memory Utilities

This module provides utilities for memory management and garbage collection
to help avoid memory leaks when working with C/C++ bindings.
"""

import gc
from typing import Any, Callable, TypeVar, Optional

T = TypeVar('T')


def safe_process_with_gc(func: Callable[..., T], *args: Any, **kwargs: Any) -> T:
    """
    Execute a function with guaranteed garbage collection before and after.

    This is useful for operations that involve C/C++ bindings where Python's
    reference counting might not be sufficient for proper cleanup.

    Args:
        func: The function to execute
        *args: Positional arguments to pass to the function
        **kwargs: Keyword arguments to pass to the function

    Returns:
        The result of calling the function
    """
    # Force garbage collection before execution
    gc.collect()

    # Execute the function
    result = None
    try:
        result = func(*args, **kwargs)
    finally:
        # Force garbage collection after execution
        gc.collect()

    return result


def clear_references(*objects: Any) -> None:
    """
    Clear references to objects to help garbage collection.

    This function sets the provided objects to None and then triggers
    garbage collection.

    Args:
        *objects: Objects to clear references to
    """
    for i, _ in enumerate(objects):
        objects[i] = None

    # Force garbage collection
    gc.collect()


def deep_cleanup(obj: Any, attr_names: Optional[list[str]] = None) -> None:
    """
    Recursively clean up an object and its attributes.

    This function sets attributes of the provided object to None to help
    break reference cycles and then triggers garbage collection.

    Args:
        obj: The object to clean up
        attr_names: A list of attribute names to clean up, or None to clean up all attributes
    """
    if obj is None:
        return

    # Get all attributes if not specified
    if attr_names is None:
        attr_names = [attr for attr in dir(obj) if not attr.startswith('__')]

    # Clear each attribute
    for attr_name in attr_names:
        try:
            if hasattr(obj, attr_name):
                attr_value = getattr(obj, attr_name)
                if isinstance(attr_value, (list, dict, set)):
                    # Clear collections
                    if isinstance(attr_value, list):
                        for i in range(len(attr_value)):
                            attr_value[i] = None
                    elif isinstance(attr_value, dict):
                        for key in list(attr_value.keys()):
                            attr_value[key] = None
                    elif isinstance(attr_value, set):
                        attr_value.clear()
                # Set attribute to None
                setattr(obj, attr_name, None)
        except (AttributeError, TypeError):
            # Skip attributes that can't be set
            pass

    # Force garbage collection
    gc.collect()
