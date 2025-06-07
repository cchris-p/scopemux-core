#!/usr/bin/env python3
"""
Demonstration of Python decorators

This example shows:
- Basic function decorators
- Decorators with arguments
- Class decorators
- Method decorators
- Decorator stacking
- Functools wraps to preserve metadata
- Creating decorators with classes
"""

import functools
import time
from typing import Callable, Any, TypeVar, cast


# Basic function decorator
def simple_decorator(func: Callable) -> Callable:
    """A simple decorator that prints before and after function execution"""

    def wrapper(*args, **kwargs):
        print(f"Before calling {func.__name__}")
        result = func(*args, **kwargs)
        print(f"After calling {func.__name__}")
        return result

    return wrapper


# Using the simple decorator
@simple_decorator
def greet(name: str) -> str:
    """Return a greeting message"""
    return f"Hello, {name}!"


# Decorator with arguments
def repeat(times: int) -> Callable:
    """A decorator that repeats the function call a specified number of times"""

    def decorator(func: Callable) -> Callable:
        def wrapper(*args, **kwargs):
            results = []
            for _ in range(times):
                results.append(func(*args, **kwargs))
            return results

        return wrapper

    return decorator


# Using the decorator with arguments
@repeat(3)
def say_hi(name: str) -> str:
    """Say hi to someone"""
    return f"Hi, {name}!"


# Using functools.wraps to preserve metadata
def timing_decorator(func: Callable) -> Callable:
    """A decorator that times function execution"""

    @functools.wraps(func)  # Preserves the original function's metadata
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        print(f"{func.__name__} took {end_time - start_time:.4f} seconds to run")
        return result

    return wrapper


# Using the timing decorator
@timing_decorator
def slow_function() -> None:
    """A function that takes some time to execute"""
    time.sleep(0.5)
    print("Slow function executed")


# Stacking decorators
@simple_decorator
@timing_decorator
def combined_decorators() -> None:
    """Function with multiple decorators"""
    print("Function with stacked decorators")
    time.sleep(0.1)


# Class decorator
def add_greeting(cls):
    """A class decorator that adds a greeting method"""

    def say_hello(self):
        return f"Hello from {self.__class__.__name__}!"

    cls.say_hello = say_hello
    return cls


# Using the class decorator
@add_greeting
class Person:
    """A simple person class"""

    def __init__(self, name: str, age: int):
        self.name = name
        self.age = age

    def __str__(self) -> str:
        return f"{self.name}, {self.age} years old"


# Method decorator
def validate_age(method: Callable) -> Callable:
    """A decorator to validate age parameter"""

    @functools.wraps(method)
    def wrapper(self, age, *args, **kwargs):
        if age < 0 or age > 120:
            raise ValueError("Age must be between 0 and 120")
        return method(self, age, *args, **kwargs)

    return wrapper


# Class with a decorated method
class Student:
    """A student class with a decorated method"""

    def __init__(self, name: str, age: int):
        self.name = name
        self.set_age(age)

    @validate_age
    def set_age(self, age: int) -> None:
        """Set student age with validation"""
        self.age = age

    def __str__(self) -> str:
        return f"Student: {self.name}, {self.age} years old"


# Creating a decorator using a class
class CountCalls:
    """A decorator implemented as a class that counts function calls"""

    def __init__(self, func: Callable):
        functools.update_wrapper(self, func)
        self.func = func
        self.count = 0

    def __call__(self, *args, **kwargs):
        self.count += 1
        print(f"{self.func.__name__} has been called {self.count} times")
        return self.func(*args, **kwargs)


# Using the class-based decorator
@CountCalls
def increment(x: int) -> int:
    """Increment a number"""
    return x + 1


# Generic decorator with TypeVar
T = TypeVar("T")


def debug(func: Callable[..., T]) -> Callable[..., T]:
    """Print function arguments and return value"""

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        args_repr = [repr(a) for a in args]
        kwargs_repr = [f"{k}={v!r}" for k, v in kwargs.items()]
        signature = ", ".join(args_repr + kwargs_repr)
        print(f"Calling {func.__name__}({signature})")
        result = func(*args, **kwargs)
        print(f"{func.__name__} returned {result!r}")
        return result

    return cast(Callable[..., T], wrapper)


# Using the debug decorator
@debug
def divide(a: float, b: float) -> float:
    """Divide two numbers"""
    return a / b


def main():
    """Main function to demonstrate decorators"""
    print("=== Basic Decorator ===")
    message = greet("Alice")
    print(f"Result: {message}")

    print("\n=== Decorator with Arguments ===")
    results = say_hi("Bob")
    print(f"Results: {results}")

    print("\n=== Timing Decorator ===")
    slow_function()

    print("\n=== Stacked Decorators ===")
    combined_decorators()

    print("\n=== Class Decorator ===")
    person = Person("Charlie", 30)
    print(person)
    print(person.say_hello())

    print("\n=== Method Decorator ===")
    student = Student("David", 20)
    print(student)
    try:
        invalid_student = Student("Eve", 150)  # Should raise an error
    except ValueError as e:
        print(f"Validation error: {e}")

    print("\n=== Class-Based Decorator ===")
    print(increment(5))
    print(increment(10))
    print(increment(15))

    print("\n=== Debug Decorator ===")
    result = divide(10, 2)
    print(f"Final result: {result}")


if __name__ == "__main__":
    main()
