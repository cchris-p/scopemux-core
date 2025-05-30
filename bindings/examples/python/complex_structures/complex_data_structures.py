#!/usr/bin/env python3
"""
Demonstration of complex data structures in Python

This example shows:
- Built-in container types (list, dict, set, tuple)
- Nested data structures
- Custom classes with special methods
- Collections module specialized containers
- Type annotations for complex structures
"""

from typing import List, Dict, Set, Tuple, Optional, Union, Any
from collections import defaultdict, Counter, deque, namedtuple
from dataclasses import dataclass
import json


# Named tuple - a simple way to create lightweight immutable classes
Point = namedtuple('Point', ['x', 'y'])

# Dataclass - a more feature-rich way to create classes primarily for storing data
@dataclass
class Rectangle:
    """
    A rectangle defined by its top-left and bottom-right corners
    """
    top_left: Point
    bottom_right: Point
    
    def area(self) -> float:
        """Calculate the area of the rectangle"""
        width = self.bottom_right.x - self.top_left.x
        height = self.bottom_right.y - self.top_left.y
        return width * height
    
    def __str__(self) -> str:
        return f"Rectangle({self.top_left} to {self.bottom_right})"


# Regular class with special methods
class Person:
    """
    A person with a name, age, and a list of friends
    """
    def __init__(self, name: str, age: int):
        self.name = name
        self.age = age
        self.friends: List['Person'] = []
    
    def add_friend(self, friend: 'Person') -> None:
        """Add a friend to this person's friend list"""
        self.friends.append(friend)
    
    def __repr__(self) -> str:
        return f"Person({self.name!r}, {self.age})"
    
    def __str__(self) -> str:
        return f"{self.name} ({self.age} years old)"


def demonstrate_basic_containers():
    """Demonstrate Python's basic container types"""
    print("1. Basic containers:")
    
    # List - ordered, mutable collection
    numbers = [1, 2, 3, 4, 5]
    print(f"  List: {numbers}")
    numbers.append(6)
    print(f"  After append: {numbers}")
    
    # Dictionary - key-value pairs
    person = {"name": "Alice", "age": 30, "city": "New York"}
    print(f"  Dictionary: {person}")
    person["job"] = "Engineer"
    print(f"  After adding key: {person}")
    
    # Set - unordered collection of unique elements
    unique_numbers = {1, 2, 3, 3, 2, 1, 4, 5}
    print(f"  Set (removes duplicates): {unique_numbers}")
    
    # Tuple - immutable ordered collection
    coordinates = (10, 20, 30)
    print(f"  Tuple: {coordinates}")
    
    # Converting between types
    print(f"  List to set: {set(numbers)}")
    print(f"  Dictionary keys to list: {list(person.keys())}")
    print(f"  Dictionary values to tuple: {tuple(person.values())}")


def demonstrate_nested_structures():
    """Demonstrate nested data structures"""
    print("\n2. Nested data structures:")
    
    # List of dictionaries
    students = [
        {"name": "Alice", "grades": [90, 85, 88]},
        {"name": "Bob", "grades": [75, 80, 85]},
        {"name": "Charlie", "grades": [95, 90, 92]}
    ]
    print(f"  List of dictionaries: {students}")
    
    # Dictionary of lists
    class_grades = {
        "Math": [90, 85, 88, 75, 80],
        "Science": [95, 85, 92, 80, 75],
        "History": [85, 90, 85, 88, 92]
    }
    print(f"  Dictionary of lists: {class_grades}")
    
    # Dictionary with nested dictionaries
    school = {
        "name": "Lincoln High",
        "location": {"city": "New York", "state": "NY"},
        "students": 500,
        "departments": {
            "Math": {"teachers": 5, "students": 150},
            "Science": {"teachers": 4, "students": 120},
            "History": {"teachers": 3, "students": 100}
        }
    }
    print(f"  Accessing nested data: {school['departments']['Math']['teachers']} Math teachers")
    
    # Complex nesting
    university = {
        "name": "State University",
        "colleges": [
            {
                "name": "Engineering",
                "departments": [
                    {"name": "Computer Science", "students": [(1001, "Alice"), (1002, "Bob")]},
                    {"name": "Electrical", "students": [(1003, "Charlie"), (1004, "Diana")]}
                ]
            },
            {
                "name": "Arts",
                "departments": [
                    {"name": "History", "students": [(2001, "Eve"), (2002, "Frank")]}
                ]
            }
        ]
    }
    print(f"  Deeply nested access: {university['colleges'][0]['departments'][0]['students'][1][1]}")


def demonstrate_specialized_containers():
    """Demonstrate specialized containers from the collections module"""
    print("\n3. Specialized containers:")
    
    # defaultdict - provides default values for missing keys
    word_counts = defaultdict(int)
    for word in "the quick brown fox jumps over the lazy dog".split():
        word_counts[word] += 1
    print(f"  defaultdict: {dict(word_counts)}")
    
    # Counter - specialized for counting items
    colors = ["red", "blue", "red", "green", "blue", "blue"]
    color_counter = Counter(colors)
    print(f"  Counter: {color_counter}")
    print(f"  Most common: {color_counter.most_common(2)}")
    
    # deque - double-ended queue
    queue = deque([1, 2, 3])
    queue.append(4)        # Add to right
    queue.appendleft(0)    # Add to left
    print(f"  deque: {queue}")
    
    # namedtuple - tuple with named fields
    p = Point(10, 20)
    print(f"  namedtuple: {p}")
    print(f"  Accessing by name: p.x = {p.x}, p.y = {p.y}")
    print(f"  Accessing by index: p[0] = {p[0]}, p[1] = {p[1]}")


def demonstrate_custom_classes():
    """Demonstrate custom classes and their usage in data structures"""
    print("\n4. Custom classes in data structures:")
    
    # Creating instances of our custom classes
    rect = Rectangle(Point(0, 0), Point(5, 10))
    print(f"  Rectangle: {rect}")
    print(f"  Area: {rect.area()}")
    
    # Using our Person class
    alice = Person("Alice", 30)
    bob = Person("Bob", 25)
    charlie = Person("Charlie", 40)
    
    # Creating a friend network
    alice.add_friend(bob)
    alice.add_friend(charlie)
    bob.add_friend(charlie)
    
    # Storing custom objects in containers
    people = [alice, bob, charlie]
    print(f"  List of custom objects: {people}")
    
    # Dictionary with custom objects as values
    people_dict = {person.name: person for person in people}
    print(f"  Dictionary with custom objects: {list(people_dict.keys())}")
    
    # Nested structure with custom objects
    friendship_graph = {
        alice: alice.friends,
        bob: bob.friends,
        charlie: charlie.friends
    }
    print(f"  Alice's friends: {[str(friend) for friend in friendship_graph[alice]]}")


def demonstrate_type_annotations():
    """Demonstrate type annotations for complex structures"""
    print("\n5. Type annotations for complex structures:")
    
    # Define some complex type annotations
    print("  # Complex type definitions:")
    print("  Vector = List[float]")
    print("  Matrix = List[Vector]")
    print("  Student = Dict[str, Union[str, int, List[int]]]")
    print("  SchoolData = Dict[str, Dict[str, Union[int, List[Student]]]]")
    
    # Implementing the annotated functions
    print("  # Example function with complex type annotations:")
    print("  def process_student_data(students: List[Student]) -> Dict[str, float]:")
    print("      # Calculate average grades for each student")
    print("      return {student['name']: sum(student['grades'])/len(student['grades'])")
    print("              for student in students}")


def main():
    """Main function to run all demonstrations"""
    print("Complex Data Structures in Python\n" + "="*30)
    
    demonstrate_basic_containers()
    demonstrate_nested_structures()
    demonstrate_specialized_containers()
    demonstrate_custom_classes()
    demonstrate_type_annotations()
    
    print("\nAll examples completed!")


if __name__ == "__main__":
    main()
