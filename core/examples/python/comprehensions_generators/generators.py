#!/usr/bin/env python3
"""
Demonstration of Python generators and coroutines

This example shows:
- Basic generators with yield
- Generator functions vs generator expressions
- Infinite generators
- Bidirectional communication with generators (coroutines)
- Generator delegation with yield from
- Context managers with generators
- Exception handling in generators
"""

from typing import Generator, Iterator, List, Dict, Any, Optional, TypeVar, Generic
import time
import contextlib


def simple_generator() -> Generator[int, None, None]:
    """A simple generator that yields a sequence of values"""
    yield 1
    yield 2
    yield 3
    yield 4
    yield 5


def countdown(n: int) -> Generator[int, None, None]:
    """Generate a countdown sequence from n to 1"""
    while n > 0:
        yield n
        n -= 1


def fibonacci(limit: int) -> Generator[int, None, None]:
    """Generate Fibonacci numbers up to the given limit"""
    a, b = 0, 1
    while a <= limit:
        yield a
        a, b = b, a + b


def infinite_counter() -> Generator[int, None, None]:
    """An infinite generator that counts from 0"""
    i = 0
    while True:
        yield i
        i += 1


def demonstrate_basic_generators() -> None:
    """Demonstrate basic generator usage"""
    print("=== Basic Generators ===")
    
    # Using a simple generator
    gen = simple_generator()
    print(f"Generator object: {gen}")
    
    # Consuming with next()
    print("\nConsuming with next():")
    print(next(gen))
    print(next(gen))
    
    # Consuming with a loop
    print("\nConsuming remaining values with a loop:")
    for value in gen:
        print(value)
    
    # Exhausted generator
    try:
        print("\nTrying to get more values from exhausted generator:")
        print(next(gen))
    except StopIteration:
        print("StopIteration exception: Generator is exhausted")
    
    # Using a parameterized generator
    print("\nCountdown from 5:")
    for number in countdown(5):
        print(number)
    
    # Fibonacci sequence
    print("\nFibonacci sequence up to 100:")
    for number in fibonacci(100):
        print(number, end=" ")
    print()


def demonstrate_generator_expressions() -> None:
    """Demonstrate generator expressions"""
    print("\n=== Generator Expressions ===")
    
    # Basic generator expression
    squares_gen = (x**2 for x in range(1, 6))
    print("Squares of 1-5:")
    for square in squares_gen:
        print(square)
    
    # Filtering with a generator expression
    evens_gen = (x for x in range(10) if x % 2 == 0)
    print("\nEven numbers from 0-9:")
    print(list(evens_gen))
    
    # Complex expressions
    expression = ((a, b) for a in range(3) for b in range(3) if a != b)
    print("\nPairs (a,b) where a≠b from range(3):")
    for pair in expression:
        print(pair)
    
    # Using an infinite generator with limiting
    print("\nFirst 5 numbers from infinite counter:")
    counter = infinite_counter()
    for _ in range(5):
        print(next(counter))


def echo_coroutine() -> Generator[None, str, None]:
    """A coroutine that echoes back what it receives"""
    print("Starting echo coroutine")
    while True:
        received = yield
        print(f"Received: {received}")


def averager() -> Generator[float, float, None]:
    """A coroutine that computes a running average"""
    total = 0.0
    count = 0
    average = 0.0
    
    while True:
        value = yield average
        total += value
        count += 1
        average = total / count


def demonstrate_coroutines() -> None:
    """Demonstrate coroutines (bidirectional generators)"""
    print("\n=== Coroutines (Bidirectional Generators) ===")
    
    # Echo coroutine
    print("\nEcho coroutine:")
    echo = echo_coroutine()
    next(echo)  # Prime the coroutine
    
    echo.send("Hello")
    echo.send("World")
    echo.send("Goodbye")
    
    # Averager coroutine
    print("\nRunning average coroutine:")
    avg = averager()
    print(f"Initial return: {next(avg)}")
    
    print(f"After sending 10: {avg.send(10)}")
    print(f"After sending 20: {avg.send(20)}")
    print(f"After sending 30: {avg.send(30)}")
    
    # Closing a coroutine
    print("\nClosing a coroutine:")
    echo.close()
    try:
        echo.send("This will fail")
    except StopIteration:
        print("StopIteration raised: Coroutine is closed")


def subgenerator(n: int) -> Generator[int, None, int]:
    """A subgenerator that yields values and returns a result"""
    # Yield n values, then return their sum
    result = 0
    for i in range(n):
        result += i
        yield i
    return result


def delegating_generator(n: int) -> Generator[int, None, int]:
    """A generator that delegates to another generator"""
    # Delegate to subgenerator and capture its return value
    result = yield from subgenerator(n)
    print(f"\nSubgenerator returned: {result}")
    return result * 2


def demonstrate_yield_from() -> None:
    """Demonstrate generator delegation with yield from"""
    print("\n=== Generator Delegation with yield from ===")
    
    # Basic yield from with an iterable
    print("Delegating to a list:")
    def gen_from_list():
        yield from [1, 2, 3, 4, 5]
    
    for item in gen_from_list():
        print(item, end=" ")
    print()
    
    # Delegating to another generator
    print("\nDelegating to subgenerator:")
    
    gen = delegating_generator(5)
    for item in gen:
        print(item, end=" ")
    
    try:
        # This would capture the final return value
        next(gen)
    except StopIteration as e:
        print(f"\nFinal return value from delegating generator: {e.value}")


@contextlib.contextmanager
def generator_context_manager(name: str) -> Generator[str, None, None]:
    """A context manager implemented with a generator"""
    print(f"Entering context: {name}")
    try:
        yield name.upper()
        print(f"Exiting context normally: {name}")
    except Exception as e:
        print(f"Exiting context with exception: {name}, {type(e).__name__}")
        raise


def demonstrate_context_managers() -> None:
    """Demonstrate generators as context managers"""
    print("\n=== Generators as Context Managers ===")
    
    # Normal usage
    with generator_context_manager("example") as value:
        print(f"Inside context manager with value: {value}")
        print("Doing some work...")
    
    # With exception
    try:
        with generator_context_manager("error_example") as value:
            print(f"Inside context manager with value: {value}")
            print("About to raise an exception...")
            raise ValueError("Deliberate error")
    except ValueError:
        print("Caught ValueError outside context manager")


def exception_handling_generator() -> Generator[int, None, None]:
    """A generator that demonstrates exception handling"""
    try:
        yield 1
        yield 2
        yield 3
    except ValueError:
        print("ValueError caught in generator")
        yield 999
    finally:
        print("Generator's finally block executed")


def demonstrate_exception_handling() -> None:
    """Demonstrate exception handling in generators"""
    print("\n=== Exception Handling in Generators ===")
    
    gen = exception_handling_generator()
    print(f"First value: {next(gen)}")
    print(f"Second value: {next(gen)}")
    
    try:
        print("Throwing ValueError into generator")
        print(f"Result after throw: {gen.throw(ValueError('Test exception'))}")
    except StopIteration:
        print("Generator stopped after throw")


def main() -> None:
    """Run all demonstrations"""
    demonstrate_basic_generators()
    demonstrate_generator_expressions()
    demonstrate_coroutines()
    demonstrate_yield_from()
    demonstrate_context_managers()
    demonstrate_exception_handling()


if __name__ == "__main__":
    main()
