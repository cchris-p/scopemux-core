/**
 * Demonstration of C++ Templates
 *
 * This example covers:
 * - Function templates
 * - Class templates
 * - Template specialization
 * - Variadic templates
 * - SFINAE (Substitution Failure Is Not An Error)
 * - Template metaprogramming basics
 */

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

/**
 * Basic function template - swaps two values of any type
 */
template <typename T> void swap_values(T &a, T &b) {
  T temp = a;
  a = b;
  b = temp;
}

/**
 * Function template with multiple template parameters
 */
template <typename T, typename U> auto add(T a, U b) -> decltype(a + b) { return a + b; }

/**
 * Function template specialization for a specific type
 */
template <typename T> T max_value(T a, T b) { return (a > b) ? a : b; }

// Specialization for C-strings
template <> const char *max_value<const char *>(const char *a, const char *b) {
  return (std::string(a) > std::string(b)) ? a : b;
}

/**
 * Basic class template - a generic stack
 */
template <typename T, size_t MaxSize = 100> class Stack {
private:
  T elements[MaxSize];
  size_t size;

public:
  Stack() : size(0) {}

  void push(const T &element) {
    if (size < MaxSize) {
      elements[size++] = element;
    } else {
      throw std::overflow_error("Stack overflow");
    }
  }

  T pop() {
    if (size > 0) {
      return elements[--size];
    } else {
      throw std::underflow_error("Stack underflow");
    }
  }

  bool empty() const { return size == 0; }

  size_t get_size() const { return size; }

  size_t capacity() const { return MaxSize; }
};

/**
 * Class template specialization
 */
template <size_t MaxSize> class Stack<bool, MaxSize> {
private:
  std::vector<bool> elements;
  size_t max_size;

public:
  Stack() : max_size(MaxSize) {}

  void push(bool element) {
    if (elements.size() < max_size) {
      elements.push_back(element);
    } else {
      throw std::overflow_error("Stack overflow");
    }
  }

  bool pop() {
    if (!elements.empty()) {
      bool value = elements.back();
      elements.pop_back();
      return value;
    } else {
      throw std::underflow_error("Stack underflow");
    }
  }

  bool empty() const { return elements.empty(); }

  size_t get_size() const { return elements.size(); }

  size_t capacity() const { return max_size; }
};

/**
 * Template with non-type parameters
 */
template <typename T, int N> class Array {
private:
  T data[N];

public:
  Array() {
    for (int i = 0; i < N; i++) {
      data[i] = T();
    }
  }

  T &operator[](int index) {
    if (index < 0 || index >= N) {
      throw std::out_of_range("Index out of range");
    }
    return data[index];
  }

  const T &operator[](int index) const {
    if (index < 0 || index >= N) {
      throw std::out_of_range("Index out of range");
    }
    return data[index];
  }

  int size() const { return N; }
};

/**
 * Variadic templates - allows any number of arguments
 */
template <typename... Args> void print_all(Args... args) {
  (std::cout << ... << args) << std::endl; // C++17 fold expression
}

// Classic recursive implementation (pre-C++17)
template <typename T> void print_args(T value) { std::cout << value << std::endl; }

template <typename T, typename... Args> void print_args(T first, Args... args) {
  std::cout << first << ", ";
  print_args(args...);
}

/**
 * Template metaprogramming - factorial at compile time
 */
template <unsigned int N> struct Factorial {
  static constexpr unsigned int value = N * Factorial<N - 1>::value;
};

template <> struct Factorial<0> {
  static constexpr unsigned int value = 1;
};

/**
 * SFINAE - enable_if to conditionally compile template functions
 * based on type traits
 */
template <typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type is_even(T value) {
  return value % 2 == 0;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type is_even(T value) {
  return int(value) % 2 == 0;
}

/**
 * C++20 Concepts (if your compiler supports it)
 * Uncomment to test with C++20
 */
/*
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<Numeric T>
T square(T value) {
    return value * value;
}
*/

// Demonstrating function templates
void demonstrate_function_templates() {
  std::cout << "=== Function Templates ===" << std::endl;

  // Basic function template
  int a = 5, b = 10;
  std::cout << "Before swap: a = " << a << ", b = " << b << std::endl;
  swap_values(a, b);
  std::cout << "After swap: a = " << a << ", b = " << b << std::endl;

  // Multiple template parameters
  auto sum1 = add(5, 3.5);
  std::cout << "5 + 3.5 = " << sum1 << std::endl;
  auto sum2 = add(std::string("Hello, "), "World!");
  std::cout << "String concatenation: " << sum2 << std::endl;

  // Template specialization
  std::cout << "Max of 10 and 20: " << max_value(10, 20) << std::endl;
  std::cout << "Max of 'apple' and 'banana': " << max_value("apple", "banana") << std::endl;
}

// Demonstrating class templates
void demonstrate_class_templates() {
  std::cout << "\n=== Class Templates ===" << std::endl;

  // Basic class template
  Stack<int, 5> int_stack;
  std::cout << "Created stack with capacity: " << int_stack.capacity() << std::endl;

  for (int i = 1; i <= 5; i++) {
    int_stack.push(i * 10);
    std::cout << "Pushed " << i * 10 << ", stack size: " << int_stack.get_size() << std::endl;
  }

  while (!int_stack.empty()) {
    std::cout << "Popped: " << int_stack.pop() << std::endl;
  }

  // Class template specialization
  Stack<bool, 10> bool_stack;
  bool_stack.push(true);
  bool_stack.push(false);
  bool_stack.push(true);
  std::cout << "Bool stack size: " << bool_stack.get_size() << std::endl;
  std::cout << "Popped from bool stack: " << (bool_stack.pop() ? "true" : "false") << std::endl;

  // Template with non-type parameters
  Array<double, 3> double_array;
  double_array[0] = 1.1;
  double_array[1] = 2.2;
  double_array[2] = 3.3;
  std::cout << "Array elements: " << double_array[0] << ", " << double_array[1] << ", "
            << double_array[2] << std::endl;
}

// Demonstrating variadic templates
void demonstrate_variadic_templates() {
  std::cout << "\n=== Variadic Templates ===" << std::endl;

  print_all("Hello", " Variadic", " Templates", "!");

  print_args(1, 2.5, "three", 'X');
}

// Demonstrating template metaprogramming
void demonstrate_template_metaprogramming() {
  std::cout << "\n=== Template Metaprogramming ===" << std::endl;

  std::cout << "Factorial of 5 (computed at compile time): " << Factorial<5>::value << std::endl;

  std::cout << "Is 10 even? " << (is_even(10) ? "Yes" : "No") << std::endl;
  std::cout << "Is 3.14 even? " << (is_even(3.14) ? "Yes" : "No") << std::endl;
}

int main() {
  std::cout << "C++ Templates Demonstration\n" << std::endl;

  demonstrate_function_templates();
  demonstrate_class_templates();
  demonstrate_variadic_templates();
  demonstrate_template_metaprogramming();

  return 0;
}
