#!/usr/bin/env python3
"""
Demonstration of Python comprehensions

This example shows:
- List comprehensions
- Set comprehensions
- Dictionary comprehensions
- Nested comprehensions
- Conditional comprehensions
- Generator expressions
"""

from typing import List, Dict, Set, Generator, Iterator, Tuple
import sys


def simple_list_comprehensions() -> None:
    """Demonstrate basic list comprehensions"""
    print("=== Simple List Comprehensions ===")
    
    # Basic list comprehension
    numbers = [1, 2, 3, 4, 5]
    squares = [x * x for x in numbers]
    print(f"Original numbers: {numbers}")
    print(f"Squares: {squares}")
    
    # Creating a list from a range
    even_numbers = [x for x in range(10) if x % 2 == 0]
    print(f"Even numbers from 0-9: {even_numbers}")
    
    # Transforming strings
    names = ["Alice", "Bob", "Charlie", "David"]
    upper_names = [name.upper() for name in names]
    print(f"Original names: {names}")
    print(f"Uppercase names: {upper_names}")
    
    # Flattening a 2D list
    matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
    flattened = [num for row in matrix for num in row]
    print(f"Matrix: {matrix}")
    print(f"Flattened: {flattened}")


def conditional_list_comprehensions() -> None:
    """Demonstrate list comprehensions with conditions"""
    print("\n=== Conditional List Comprehensions ===")
    
    # Filtering with a condition
    numbers = list(range(1, 21))
    divisible_by_3 = [x for x in numbers if x % 3 == 0]
    print(f"Numbers from 1-20: {numbers}")
    print(f"Divisible by 3: {divisible_by_3}")
    
    # Using if-else inside comprehension
    parity = ["even" if x % 2 == 0 else "odd" for x in range(5)]
    print(f"Parity of 0-4: {parity}")
    
    # Multiple conditions
    filtered = [x for x in range(50) if x % 2 == 0 if x % 3 == 0]
    print(f"Numbers from 0-49 divisible by both 2 and 3: {filtered}")
    
    # Alternative for multiple conditions
    filtered_alt = [x for x in range(50) if x % 2 == 0 and x % 3 == 0]
    print(f"Same result with 'and': {filtered_alt}")


def set_comprehensions() -> None:
    """Demonstrate set comprehensions"""
    print("\n=== Set Comprehensions ===")
    
    # Basic set comprehension
    numbers = [1, 2, 2, 3, 4, 4, 5]
    unique_squares = {x * x for x in numbers}
    print(f"Original numbers with duplicates: {numbers}")
    print(f"Unique squares: {unique_squares}")
    
    # Character set from a string
    text = "hello world"
    char_set = {c for c in text if c.isalpha()}
    print(f"Original text: '{text}'")
    print(f"Unique letters: {char_set}")
    
    # Removing duplicates from a list
    duplicates = [1, 2, 3, 1, 2, 4, 5, 4]
    unique = list({x for x in duplicates})
    print(f"List with duplicates: {duplicates}")
    print(f"Without duplicates: {unique}")


def dict_comprehensions() -> None:
    """Demonstrate dictionary comprehensions"""
    print("\n=== Dictionary Comprehensions ===")
    
    # Basic dictionary comprehension
    numbers = [1, 2, 3, 4, 5]
    squares_dict = {x: x * x for x in numbers}
    print(f"Numbers to squares mapping: {squares_dict}")
    
    # Creating a dict from two lists
    keys = ["a", "b", "c", "d"]
    values = [1, 2, 3, 4]
    combined = {k: v for k, v in zip(keys, values)}
    print(f"Dictionary from keys {keys} and values {values}: {combined}")
    
    # Filtering items in a dictionary
    original_dict = {"a": 1, "b": 2, "c": 3, "d": 4, "e": 5}
    filtered_dict = {k: v for k, v in original_dict.items() if v % 2 == 0}
    print(f"Original dictionary: {original_dict}")
    print(f"Filtered (even values): {filtered_dict}")
    
    # Swapping keys and values
    swapped = {v: k for k, v in original_dict.items()}
    print(f"Swapped keys and values: {swapped}")
    
    # Character frequency counter
    word = "mississippi"
    char_freq = {c: word.count(c) for c in set(word)}
    print(f"Character frequency in '{word}': {char_freq}")


def nested_comprehensions() -> None:
    """Demonstrate nested comprehensions"""
    print("\n=== Nested Comprehensions ===")
    
    # Creating a matrix with list comprehensions
    matrix = [[i + j for j in range(3)] for i in range(3)]
    print("Matrix created with nested comprehensions:")
    for row in matrix:
        print(row)
    
    # Nested dictionary comprehension
    nested_dict = {i: {j: i * j for j in range(1, 4)} for i in range(1, 4)}
    print("\nNested dictionary:")
    for k, v in nested_dict.items():
        print(f"{k}: {v}")
    
    # Generating a multiplication table
    mult_table = [[i * j for j in range(1, 6)] for i in range(1, 6)]
    print("\nMultiplication table (1-5):")
    for row in mult_table:
        print(row)


def generator_expressions() -> None:
    """Demonstrate generator expressions"""
    print("\n=== Generator Expressions ===")
    
    # Basic generator expression
    gen = (x * x for x in range(5))
    print(f"Generator object: {gen}")
    print("Consuming generator values:")
    for value in gen:
        print(value, end=" ")
    print()
    
    # Memory comparison with list comprehension
    print("\nMemory comparison:")
    big_list = [x for x in range(1000000)]
    big_gen = (x for x in range(1000000))
    
    list_size = sys.getsizeof(big_list)
    gen_size = sys.getsizeof(big_gen)
    
    print(f"List comprehension size: {list_size} bytes")
    print(f"Generator expression size: {gen_size} bytes")
    print(f"Ratio: {list_size / gen_size:.1f}x")
    
    # Filtering with a generator expression
    filtered_gen = (x for x in range(10) if x % 2 == 0)
    print("\nFiltered generator values:")
    print(list(filtered_gen))
    
    # Chaining generator expressions
    chain = sum((x * x for x in range(10)))
    print(f"\nSum of squares from 0-9: {chain}")


def main() -> None:
    """Run all examples"""
    simple_list_comprehensions()
    conditional_list_comprehensions()
    set_comprehensions()
    dict_comprehensions()
    nested_comprehensions()
    generator_expressions()


if __name__ == "__main__":
    main()
