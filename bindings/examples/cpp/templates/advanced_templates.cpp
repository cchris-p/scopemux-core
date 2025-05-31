/**
 * @file advanced_templates.cpp
 * @brief Demonstration of C++ templates features
 *
 * This example shows:
 * - Function templates
 * - Class templates
 * - Template specialization
 * - Variadic templates
 * - SFINAE (Substitution Failure Is Not An Error)
 * - Concepts (C++20)
 */

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// Simple function template
template <typename T> T max_value(T a, T b) { return (a > b) ? a : b; }

// Function template with multiple type parameters
template <typename T, typename U> auto add(T a, U b) -> decltype(a + b) { return a + b; }

// Class template
template <typename T> class Container {
private:
  T element;

public:
  Container(T value) : element(value) {}

  T get() const { return element; }

  void set(T value) { element = value; }

  // Member function template
  template <typename U> U convert() const { return static_cast<U>(element); }
};

// Template specialization for std::string
template <> class Container<std::string> {
private:
  std::string element;

public:
  Container(const std::string &value) : element(value) {}

  std::string get() const { return element; }

  void set(const std::string &value) { element = value; }

  // Additional methods specific to string specialization
  int length() const { return element.length(); }

  bool empty() const { return element.empty(); }
};

// Variadic templates
template <typename T> T sum(T value) { return value; }

template <typename T, typename... Args> T sum(T first, Args... args) {
  return first + sum<T>(args...);
}

// SFINAE example: enable_if
template <typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type is_even(T value) {
  return value % 2 == 0;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type is_even(T value) {
  return static_cast<int>(value) % 2 == 0;
}

// Template with non-type parameter
template <typename T, int Size> class FixedArray {
private:
  T data[Size];

public:
  FixedArray() {
    for (int i = 0; i < Size; ++i) {
      data[i] = T();
    }
  }

  T &operator[](int index) { return data[index]; }

  const T &operator[](int index) const { return data[index]; }

  int size() const { return Size; }
};

// Class template with template template parameter
template <typename T, template <typename...> class Container> class GenericContainer {
private:
  Container<T> data;

public:
  void add(const T &item) { data.push_back(item); }

  typename Container<T>::size_type size() const { return data.size(); }

  T &operator[](size_t index) { return data[index]; }
};

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
  // Using function templates
  std::cout << "Function templates:" << std::endl;
  std::cout << "  max_value(5, 10): " << max_value(5, 10) << std::endl;
  std::cout << "  max_value(3.14, 2.71): " << max_value(3.14, 2.71) << std::endl;
  std::cout << "  max_value(\"abc\", \"xyz\"): "
            << max_value(std::string("abc"), std::string("xyz")) << std::endl;

  // Using add template with different types
  std::cout << "\nFunction templates with multiple types:" << std::endl;
  std::cout << "  add(5, 10): " << add(5, 10) << std::endl;
  std::cout << "  add(5, 3.14): " << add(5, 3.14) << std::endl;

  // Using class templates
  std::cout << "\nClass templates:" << std::endl;
  Container<int> intContainer(42);
  std::cout << "  intContainer.get(): " << intContainer.get() << std::endl;

  Container<double> doubleContainer(3.14159);
  std::cout << "  doubleContainer.get(): " << doubleContainer.get() << std::endl;
  std::cout << "  doubleContainer.convert<int>(): " << doubleContainer.convert<int>() << std::endl;

  // Using template specialization
  std::cout << "\nTemplate specialization:" << std::endl;
  Container<std::string> stringContainer("Hello, Templates!");
  std::cout << "  stringContainer.get(): " << stringContainer.get() << std::endl;
  std::cout << "  stringContainer.length(): " << stringContainer.length() << std::endl;

  // Using variadic templates
  std::cout << "\nVariadic templates:" << std::endl;
  std::cout << "  sum(1, 2, 3, 4, 5): " << sum(1, 2, 3, 4, 5) << std::endl;
  std::cout << "  sum(1.1, 2.2, 3.3): " << sum(1.1, 2.2, 3.3) << std::endl;

  // Using SFINAE
  std::cout << "\nSFINAE (enable_if):" << std::endl;
  std::cout << "  is_even(42): " << (is_even(42) ? "true" : "false") << std::endl;
  std::cout << "  is_even(3.14): " << (is_even(3.14) ? "true" : "false") << std::endl;

  // Using templates with non-type parameters
  std::cout << "\nTemplates with non-type parameters:" << std::endl;
  FixedArray<int, 5> intArray;
  for (int i = 0; i < intArray.size(); ++i) {
    intArray[i] = i * 10;
  }

  for (int i = 0; i < intArray.size(); ++i) {
    std::cout << "  intArray[" << i << "]: " << intArray[i] << std::endl;
  }

  // Using template template parameters
  std::cout << "\nTemplate template parameters:" << std::endl;
  GenericContainer<int, std::vector> vecContainer;
  vecContainer.add(10);
  vecContainer.add(20);
  vecContainer.add(30);

  std::cout << "  vecContainer size: " << vecContainer.size() << std::endl;
  for (size_t i = 0; i < vecContainer.size(); ++i) {
    std::cout << "  vecContainer[" << i << "]: " << vecContainer[i] << std::endl;
  }

  return 0;
}
