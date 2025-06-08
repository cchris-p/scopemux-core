#!/usr/bin/env python3
"""
Demonstration of variables, loops, and conditional statements in Python

This example shows:
- Various variable declarations and types
- for and while loops
- if, elif, else conditions
- List, dictionary, and set comprehensions
- Type hints (Python 3.5+)
"""

from typing import List, Dict, Set, Tuple, Optional


def main() -> None:
    """
    Program entry point
    """
    # Variable declarations with type hints
    i: int = 0
    f: float = 3.14
    s: str = "Hello, Python"
    b: bool = True

    # Collection types
    my_list: List[int] = [1, 2, 3, 4, 5]
    my_dict: Dict[str, int] = {"one": 1, "two": 2, "three": 3}
    my_set: Set[int] = {10, 20, 30, 40, 50}
    my_tuple: Tuple[int, str, float] = (1, "two", 3.0)

    # For loop with a list
    print("For loop with list example:")
    for value in my_list:
        print(f"value: {value}")

    # For loop with range
    print("\nFor loop with range example:")
    for i in range(5):
        print(f"iteration: {i}")

    # For loop with enumeration
    print("\nFor loop with enumeration example:")
    for index, value in enumerate(my_list):
        print(f"index: {index}, value: {value}")

    # For loop with dictionary
    print("\nFor loop with dictionary example:")
    for key, value in my_dict.items():
        print(f"key: {key}, value: {value}")

    # While loop
    print("\nWhile loop example:")
    i = 0
    while i < 5:
        print(f"iteration: {i}")
        i += 1

    # If-else statements
    print("\nIf-else example:")
    if b:
        print("b is True")
    else:
        print("b is False")

    # If-elif-else
    print("\nIf-elif-else example:")
    if i < 3:
        print("i is less than 3")
    elif i == 5:
        print("i is equal to 5")
    else:
        print("i is greater than 5")

    # List comprehension
    print("\nList comprehension example:")
    squares = [x**2 for x in range(10) if x % 2 == 0]
    print(f"Squares of even numbers from 0-9: {squares}")

    # Dictionary comprehension
    print("\nDictionary comprehension example:")
    char_count = {char: s.count(char) for char in set(s) if char.isalpha()}
    print(f"Character count in '{s}': {char_count}")


if __name__ == "__main__":
    main()
