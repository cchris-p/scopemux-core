#!/usr/bin/env python3
"""
Demonstration of Python Metaprogramming

This example covers:
- Metaclasses
- Class decorators
- Dynamic attribute access (__getattr__, __setattr__)
- Descriptors
- Dynamic class creation (type)
- Attribute access customization
- Runtime class modification
"""

from typing import Any, Dict, List, Callable, Type, Optional, TypeVar, cast
import functools
import inspect
import time


def log_calls(cls):
    """A class decorator that logs method calls"""

    # Store the original __getattribute__
    original_getattribute = cls.__getattribute__

    # Define a new __getattribute__ that wraps method calls with logging
    def __getattribute__(self, name):
        attr = original_getattribute(self, name)
        if callable(attr) and not name.startswith("__"):

            @functools.wraps(attr)
            def wrapper(*args, **kwargs):
                print(f"Calling {cls.__name__}.{name}")
                result = attr(*args, **kwargs)
                print(f"Finished {cls.__name__}.{name}, returned {result}")
                return result

            return wrapper
        return attr

    # Replace the original __getattribute__ with our version
    cls.__getattribute__ = __getattribute__
    return cls


def singleton(cls):
    """A class decorator that implements the Singleton pattern"""

    instances = {}

    @functools.wraps(cls)
    def get_instance(*args, **kwargs):
        if cls not in instances:
            instances[cls] = cls(*args, **kwargs)
        return instances[cls]

    return get_instance


class ValidateAttributes(type):
    """A metaclass that enforces type validation for attributes"""

    def __new__(mcs, name, bases, attrs):
        # Create _validators dictionary to store validation functions
        attrs["_validators"] = {}

        # Look for validate_<attr> methods and register them
        for key, value in list(attrs.items()):
            if key.startswith("validate_"):
                attr_name = key[9:]  # Remove 'validate_' prefix
                attrs["_validators"][attr_name] = value

        return super().__new__(mcs, name, bases, attrs)

    def __init__(cls, name, bases, attrs):
        super().__init__(name, bases, attrs)

        # Store original __setattr__
        original_setattr = cls.__setattr__

        # Define a new __setattr__ that validates attributes
        def __setattr__(self, name, value):
            if name in self._validators:
                # Call the validator method
                validator = getattr(self, f"validate_{name}")
                value = validator(value)

            # Call the original __setattr__
            original_setattr(self, name, value)

        # Replace __setattr__ with our version
        cls.__setattr__ = __setattr__


class LazyProperty:
    """A descriptor that computes the value only when first accessed"""

    def __init__(self, func):
        self.func = func
        self.__doc__ = func.__doc__
        self.__name__ = func.__name__

    def __get__(self, instance, owner):
        if instance is None:
            return self

        # Compute the value
        value = self.func(instance)

        # Cache the value on the instance
        setattr(instance, self.__name__, value)

        return value


class TimingDescriptor:
    """A descriptor that measures the execution time of methods"""

    def __init__(self, func):
        self.func = func
        functools.update_wrapper(self, func)

    def __get__(self, instance, owner):
        if instance is None:
            return self

        @functools.wraps(self.func)
        def wrapper(*args, **kwargs):
            start_time = time.time()
            result = self.func(instance, *args, **kwargs)
            end_time = time.time()
            print(f"{self.func.__name__} took {end_time - start_time:.6f} seconds")
            return result

        return wrapper


def timing_method(func):
    """A method decorator that measures execution time"""

    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        start_time = time.time()
        result = func(self, *args, **kwargs)
        end_time = time.time()
        print(f"{func.__name__} took {end_time - start_time:.6f} seconds")
        return result

    return wrapper


class AttributeProxy:
    """A descriptor that delegates attribute access to another object"""

    def __init__(self, target_attr):
        self.target_attr = target_attr

    def __get__(self, instance, owner):
        if instance is None:
            return self

        # Get the target object
        target = getattr(instance, self.target_attr)
        return target


# Example of a class with metaclass
class Person(metaclass=ValidateAttributes):
    """A person with validated attributes"""

    def __init__(self, name: str, age: int, email: str):
        self.name = name
        self.age = age
        self.email = email

    def validate_name(self, value):
        """Validate that name is a non-empty string"""
        if not isinstance(value, str) or not value:
            raise ValueError("Name must be a non-empty string")
        return value

    def validate_age(self, value):
        """Validate that age is a positive integer"""
        if not isinstance(value, int) or value <= 0:
            raise ValueError("Age must be a positive integer")
        return value

    def validate_email(self, value):
        """Validate that email contains @"""
        if not isinstance(value, str) or "@" not in value:
            raise ValueError("Email must be a string containing @")
        return value

    @LazyProperty
    def formatted_info(self):
        """Return formatted information about the person"""
        print("Computing formatted_info (this happens only once)")
        return f"{self.name} ({self.age}) - {self.email}"

    @timing_method
    def do_work(self, duration: float = 0.1):
        """Simulate doing some work"""
        time.sleep(duration)
        return f"{self.name} completed work"


# Example of a class with a class decorator
@log_calls
class Calculator:
    """A simple calculator with logging"""

    def add(self, a, b):
        return a + b

    def subtract(self, a, b):
        return a - b

    def multiply(self, a, b):
        return a * b

    def divide(self, a, b):
        if b == 0:
            raise ValueError("Division by zero")
        return a / b


# Example of a singleton class
@singleton
class AppConfig:
    """A singleton configuration class"""

    def __init__(self):
        self.settings = {}
        print("AppConfig instance created")

    def get_setting(self, key, default=None):
        return self.settings.get(key, default)

    def set_setting(self, key, value):
        self.settings[key] = value


class DynamicAttributes:
    """A class that handles missing attributes dynamically"""

    def __init__(self):
        self._attributes = {}

    def __getattr__(self, name):
        """Called when an attribute is not found"""
        if name in self._attributes:
            return self._attributes[name]

        # Create a dynamic method if asked for get_<name>
        if name.startswith("get_"):
            attr_name = name[4:]  # Remove 'get_' prefix
            return lambda: self._attributes.get(attr_name)

        raise AttributeError(f"{self.__class__.__name__} has no attribute '{name}'")

    def __setattr__(self, name, value):
        """Called when an attribute is set"""
        if name == "_attributes":
            # Allow setting the _attributes dictionary directly
            super().__setattr__(name, value)
        else:
            # Store other attributes in the _attributes dictionary
            self._attributes[name] = value


def demonstrate_metaclass():
    print("\n=== Metaclass Example ===")

    try:
        # Create a person with valid attributes
        person = Person("Alice", 30, "alice@example.com")
        print(f"Created person: {person.name}, {person.age}, {person.email}")

        # Access the lazy property
        print(f"Formatted info: {person.formatted_info}")
        # Access it again (should be cached)
        print(f"Formatted info again: {person.formatted_info}")

        # Try with invalid attributes
        try:
            person.age = -5
            print("This should not be printed")
        except ValueError as e:
            print(f"Validation error: {e}")

        # Call the timed method
        result = person.do_work()
        print(f"Work result: {result}")

    except Exception as e:
        print(f"Error: {e}")


def demonstrate_class_decorator():
    print("\n=== Class Decorator Example ===")

    # Create a calculator instance
    calc = Calculator()

    # Call methods (should be logged)
    result1 = calc.add(5, 3)
    print(f"5 + 3 = {result1}")

    result2 = calc.multiply(4, 6)
    print(f"4 * 6 = {result2}")


def demonstrate_singleton():
    print("\n=== Singleton Example ===")

    # Create multiple instances (should be the same object)
    config1 = AppConfig()
    config1.set_setting("theme", "dark")

    config2 = AppConfig()
    theme = config2.get_setting("theme")
    print(f"Theme from config2: {theme}")

    # Prove they're the same object
    print(f"config1 is config2: {config1 is config2}")


def demonstrate_dynamic_attributes():
    print("\n=== Dynamic Attributes Example ===")

    # Create an instance
    obj = DynamicAttributes()

    # Set some attributes
    obj.name = "Dynamic Object"
    obj.value = 42

    # Get attributes
    print(f"Name: {obj.name}")
    print(f"Value: {obj.value}")

    # Use a dynamic getter method
    get_name = obj.get_name
    print(f"get_name(): {get_name()}")

    # Try to access a non-existent attribute
    try:
        print(obj.nonexistent)
    except AttributeError as e:
        print(f"Error: {e}")


def demonstrate_dynamic_class_creation():
    print("\n=== Dynamic Class Creation Example ===")

    # Define methods that will be part of the dynamically created class
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def calculate(self):
        return self.x * self.y

    def __str__(self):
        return f"DynamicClass({self.x}, {self.y})"

    # Create a new class dynamically
    attributes = {
        "__init__": __init__,
        "calculate": calculate,
        "__str__": __str__,
    }

    DynamicClass = type("DynamicClass", (), attributes)

    # Create an instance of the dynamic class
    instance = DynamicClass(10, 20)
    print(f"Instance: {instance}")
    print(f"Calculation result: {instance.calculate()}")

    # Dynamically add a new method to the class
    def new_method(self):
        return self.x + self.y

    DynamicClass.add = new_method

    # Use the newly added method
    print(f"Addition result: {instance.add()}")


def main():
    print("Python Metaprogramming Demonstration")

    demonstrate_metaclass()
    demonstrate_class_decorator()
    demonstrate_singleton()
    demonstrate_dynamic_attributes()
    demonstrate_dynamic_class_creation()


if __name__ == "__main__":
    main()
