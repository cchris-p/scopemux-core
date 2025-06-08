/**
 * Demonstration of Modern C++ Features
 *
 * This example covers:
 * - auto type deduction
 * - Range-based for loops
 * - Lambda expressions
 * - Smart pointers
 * - Move semantics
 * - std::optional, std::variant, std::any
 * - Structured bindings
 * - if constexpr
 * - fold expressions
 */

#include <algorithm>
#include <any>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

/**
 * Demonstrate auto type deduction
 */
void demonstrate_auto() {
  std::cout << "=== Auto Type Deduction ===" << std::endl;

  // Basic auto
  auto i = 42;
  auto d = 3.14;
  auto s = "Hello";
  auto v = std::vector<int>{1, 2, 3};

  std::cout << "i = " << i << " (type: int)" << std::endl;
  std::cout << "d = " << d << " (type: double)" << std::endl;
  std::cout << "s = " << s << " (type: const char*)" << std::endl;
  std::cout << "v = [";
  for (const auto &elem : v) {
    std::cout << elem << " ";
  }
  std::cout << "] (type: std::vector<int>)" << std::endl;

  // Auto with references and const
  const auto &ref = i;
  std::cout << "ref = " << ref << " (type: const int&)" << std::endl;

  // Function return type deduction
  auto add = [](auto a, auto b) { return a + b; };
  std::cout << "add(5, 3) = " << add(5, 3) << std::endl;
  std::cout << "add(3.5, 2.7) = " << add(3.5, 2.7) << std::endl;

  // Trailing return type syntax
  auto multiply = [](auto a, auto b) -> decltype(a * b) { return a * b; };
  std::cout << "multiply(5, 3) = " << multiply(5, 3) << std::endl;
}

/**
 * Demonstrate range-based for loops
 */
void demonstrate_range_based_for() {
  std::cout << "\n=== Range-Based For Loops ===" << std::endl;

  // With vector
  std::vector<int> nums = {1, 2, 3, 4, 5};
  std::cout << "Vector elements: ";
  for (const auto &num : nums) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  // With array
  int arr[] = {10, 20, 30, 40, 50};
  std::cout << "Array elements: ";
  for (auto elem : arr) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // With initializer list
  std::cout << "Initializer list elements: ";
  for (auto c : {'a', 'b', 'c', 'd', 'e'}) {
    std::cout << c << " ";
  }
  std::cout << std::endl;

  // With string
  std::string text = "Hello";
  std::cout << "String characters: ";
  for (auto ch : text) {
    std::cout << ch << " ";
  }
  std::cout << std::endl;

  // With map
  std::map<std::string, int> ages = {{"Alice", 25}, {"Bob", 30}, {"Charlie", 35}};
  std::cout << "Map entries:" << std::endl;
  for (const auto &[name, age] : ages) { // C++17 structured binding
    std::cout << name << " is " << age << " years old" << std::endl;
  }
}

/**
 * Demonstrate lambda expressions
 */
void demonstrate_lambdas() {
  std::cout << "\n=== Lambda Expressions ===" << std::endl;

  // Basic lambda
  auto greet = []() { std::cout << "Hello, World!" << std::endl; };
  std::cout << "Basic lambda: ";
  greet();

  // Lambda with parameters
  auto add = [](int a, int b) { return a + b; };
  std::cout << "Lambda with parameters: 5 + 3 = " << add(5, 3) << std::endl;

  // Lambda with capture by value
  int x = 10;
  auto add_x = [x](int a) { return a + x; };
  std::cout << "Lambda capturing x by value: 5 + x = " << add_x(5) << std::endl;

  // Lambda with capture by reference
  auto modify_x = [&x]() { x *= 2; };
  std::cout << "Before lambda capture by reference: x = " << x << std::endl;
  modify_x();
  std::cout << "After lambda capture by reference: x = " << x << std::endl;

  // Lambda with capture all by value
  int y = 20;
  auto capture_all = [=]() { return x + y; };
  std::cout << "Lambda capturing all by value: x + y = " << capture_all() << std::endl;

  // Lambda with capture all by reference
  auto modify_all = [&]() {
    x += 5;
    y += 10;
  };
  std::cout << "Before lambda capture all by reference: x = " << x << ", y = " << y << std::endl;
  modify_all();
  std::cout << "After lambda capture all by reference: x = " << x << ", y = " << y << std::endl;

  // Lambda with mutable
  auto counter = [count = 0]() mutable { return ++count; };
  std::cout << "Mutable lambda counter: " << counter() << std::endl;
  std::cout << "Mutable lambda counter: " << counter() << std::endl;
  std::cout << "Mutable lambda counter: " << counter() << std::endl;

  // Using lambda with algorithms
  std::vector<int> values = {5, 1, 3, 4, 2};
  std::sort(values.begin(), values.end(), [](int a, int b) { return a > b; });
  std::cout << "Vector sorted with lambda (descending): ";
  for (auto v : values) {
    std::cout << v << " ";
  }
  std::cout << std::endl;

  // Generic lambda (C++14)
  auto generic_add = [](auto a, auto b) { return a + b; };
  std::cout << "Generic lambda: 5 + 3 = " << generic_add(5, 3) << std::endl;
  std::cout << "Generic lambda: 3.5 + 2.7 = " << generic_add(3.5, 2.7) << std::endl;
  std::cout << "Generic lambda: \"Hello, \" + \"World!\" = "
            << generic_add(std::string("Hello, "), std::string("World!")) << std::endl;
}

/**
 * Demonstrate smart pointers
 */
void demonstrate_smart_pointers() {
  std::cout << "\n=== Smart Pointers ===" << std::endl;

  // unique_ptr - exclusive ownership
  std::cout << "std::unique_ptr:" << std::endl;
  {
    std::unique_ptr<int> ptr1 = std::make_unique<int>(42);
    std::cout << "ptr1 value: " << *ptr1 << std::endl;

    // Cannot copy a unique_ptr
    // std::unique_ptr<int> ptr2 = ptr1;  // Compilation error

    // But can move it
    std::unique_ptr<int> ptr2 = std::move(ptr1);
    std::cout << "ptr2 value (after move): " << *ptr2 << std::endl;
    std::cout << "ptr1 is " << (ptr1 ? "not null" : "null") << " after move" << std::endl;

    // Custom deleter
    auto custom_deleter = [](int *p) {
      std::cout << "Custom deleter called for value: " << *p << std::endl;
      delete p;
    };
    std::unique_ptr<int, decltype(custom_deleter)> ptr3(new int(100), custom_deleter);
  } // ptr1, ptr2, ptr3 automatically deleted here
  std::cout << "All unique_ptr instances destroyed at end of scope" << std::endl;

  // shared_ptr - shared ownership
  std::cout << "\nstd::shared_ptr:" << std::endl;
  {
    std::shared_ptr<int> ptr1 = std::make_shared<int>(42);
    std::cout << "ptr1 value: " << *ptr1 << ", use count: " << ptr1.use_count() << std::endl;

    // Can copy a shared_ptr
    std::shared_ptr<int> ptr2 = ptr1;
    std::cout << "ptr2 value: " << *ptr2 << std::endl;
    std::cout << "ptr1 use count after copy: " << ptr1.use_count() << std::endl;

    // Another scope
    {
      std::shared_ptr<int> ptr3 = ptr1;
      std::cout << "ptr1 use count in nested scope: " << ptr1.use_count() << std::endl;
    }
    std::cout << "ptr1 use count after nested scope: " << ptr1.use_count() << std::endl;

    // Can reset to break the reference
    ptr2.reset();
    std::cout << "ptr1 use count after ptr2 reset: " << ptr1.use_count() << std::endl;
  } // Both ptr1 and ptr2 automatically deleted here
  std::cout << "All shared_ptr instances destroyed at end of scope" << std::endl;

  // weak_ptr - non-owning reference
  std::cout << "\nstd::weak_ptr:" << std::endl;
  {
    std::shared_ptr<int> shared = std::make_shared<int>(42);
    std::weak_ptr<int> weak = shared;

    std::cout << "shared use count: " << shared.use_count() << std::endl;
    std::cout << "weak ptr is " << (weak.expired() ? "expired" : "valid") << std::endl;

    // Using weak_ptr
    if (auto temp = weak.lock()) {
      std::cout << "weak_ptr value: " << *temp << std::endl;
    }

    // Reset the shared_ptr
    shared.reset();
    std::cout << "After shared.reset(), weak ptr is " << (weak.expired() ? "expired" : "valid")
              << std::endl;

    // Try to use the weak_ptr after shared_ptr is gone
    if (auto temp = weak.lock()) {
      std::cout << "weak_ptr value: " << *temp << std::endl;
    } else {
      std::cout << "weak_ptr cannot be locked (shared object is gone)" << std::endl;
    }
  }
}

/**
 * Demonstrate move semantics and perfect forwarding
 */
class MovableObject {
private:
  int *data;
  size_t size;

public:
  // Constructor
  MovableObject(size_t size) : size(size) {
    std::cout << "Constructor called" << std::endl;
    data = new int[size];
    for (size_t i = 0; i < size; ++i) {
      data[i] = static_cast<int>(i);
    }
  }

  // Destructor
  ~MovableObject() {
    std::cout << "Destructor called" << std::endl;
    delete[] data;
  }

  // Copy constructor
  MovableObject(const MovableObject &other) : size(other.size) {
    std::cout << "Copy constructor called" << std::endl;
    data = new int[size];
    for (size_t i = 0; i < size; ++i) {
      data[i] = other.data[i];
    }
  }

  // Move constructor
  MovableObject(MovableObject &&other) noexcept : data(other.data), size(other.size) {
    std::cout << "Move constructor called" << std::endl;
    other.data = nullptr;
    other.size = 0;
  }

  // Copy assignment
  MovableObject &operator=(const MovableObject &other) {
    std::cout << "Copy assignment called" << std::endl;
    if (this != &other) {
      delete[] data;
      size = other.size;
      data = new int[size];
      for (size_t i = 0; i < size; ++i) {
        data[i] = other.data[i];
      }
    }
    return *this;
  }

  // Move assignment
  MovableObject &operator=(MovableObject &&other) noexcept {
    std::cout << "Move assignment called" << std::endl;
    if (this != &other) {
      delete[] data;
      data = other.data;
      size = other.size;
      other.data = nullptr;
      other.size = 0;
    }
    return *this;
  }

  // Utility functions
  size_t get_size() const { return size; }

  void print() const {
    if (!data) {
      std::cout << "Object has been moved from" << std::endl;
      return;
    }
    std::cout << "Data: [";
    for (size_t i = 0; i < size && i < 5; ++i) {
      std::cout << data[i];
      if (i < size - 1 && i < 4)
        std::cout << ", ";
    }
    if (size > 5)
      std::cout << ", ...";
    std::cout << "]" << std::endl;
  }
};

// Function that takes an lvalue reference
void process_lvalue(MovableObject &obj) {
  std::cout << "Processing lvalue" << std::endl;
  obj.print();
}

// Function that takes an rvalue reference
void process_rvalue(MovableObject &&obj) {
  std::cout << "Processing rvalue" << std::endl;
  obj.print();
}

// Perfect forwarding with universal reference
template <typename T> void forward_object(T &&obj) {
  std::cout << "Forwarding object" << std::endl;
  process_rvalue(std::forward<T>(obj)); // Forward the object with its original value category
}

void demonstrate_move_semantics() {
  std::cout << "\n=== Move Semantics and Perfect Forwarding ===" << std::endl;

  std::cout << "\nCreating original object:" << std::endl;
  MovableObject obj1(10);
  obj1.print();

  std::cout << "\nCopy construction:" << std::endl;
  MovableObject obj2 = obj1; // Copy constructor
  obj2.print();

  std::cout << "\nMove construction:" << std::endl;
  MovableObject obj3 = std::move(obj1); // Move constructor
  std::cout << "Original object after move:" << std::endl;
  obj1.print();
  std::cout << "New object after move:" << std::endl;
  obj3.print();

  std::cout << "\nCopy assignment:" << std::endl;
  MovableObject obj4(5);
  obj4 = obj2; // Copy assignment
  obj4.print();

  std::cout << "\nMove assignment:" << std::endl;
  MovableObject obj5(3);
  obj5 = std::move(obj2); // Move assignment
  std::cout << "Original object after move assignment:" << std::endl;
  obj2.print();
  std::cout << "New object after move assignment:" << std::endl;
  obj5.print();

  std::cout << "\nPerfect forwarding:" << std::endl;
  MovableObject obj6(5);
  std::cout << "Forwarding lvalue:" << std::endl;
  forward_object(obj6); // T is deduced as MovableObject&
  std::cout << "Forwarding rvalue:" << std::endl;
  forward_object(std::move(obj6)); // T is deduced as MovableObject
}

/**
 * Demonstrate std::optional, std::variant, and std::any
 */
void demonstrate_optional_variant_any() {
  std::cout << "\n=== std::optional, std::variant, and std::any ===" << std::endl;

  // std::optional
  std::cout << "\nstd::optional:" << std::endl;

  std::optional<int> op1;      // No value
  std::optional<int> op2 = 42; // Has value

  std::cout << "op1 has value: " << std::boolalpha << op1.has_value() << std::endl;
  std::cout << "op2 has value: " << op2.has_value() << std::endl;

  // Accessing value
  if (op2) {
    std::cout << "op2 value: " << *op2 << std::endl;
  }

  // Setting value
  op1 = 100;
  std::cout << "op1 after setting: " << *op1 << std::endl;

  // Clearing value
  op1.reset();
  std::cout << "op1 has value after reset: " << op1.has_value() << std::endl;

  // value_or
  std::cout << "op1 value_or: " << op1.value_or(-1) << std::endl;
  std::cout << "op2 value_or: " << op2.value_or(-1) << std::endl;

  // Using optional for functions that might not return a value
  auto find_in_vector = [](const std::vector<int> &v, int val) -> std::optional<size_t> {
    for (size_t i = 0; i < v.size(); ++i) {
      if (v[i] == val)
        return i;
    }
    return std::nullopt;
  };

  std::vector<int> numbers = {10, 20, 30, 40, 50};
  if (auto pos = find_in_vector(numbers, 30)) {
    std::cout << "Found 30 at position: " << *pos << std::endl;
  }

  if (auto pos = find_in_vector(numbers, 99)) {
    std::cout << "Found 99 at position: " << *pos << std::endl;
  } else {
    std::cout << "99 not found in vector" << std::endl;
  }

  // std::variant
  std::cout << "\nstd::variant:" << std::endl;

  // Creating variants
  std::variant<int, double, std::string> var1 = 10;
  std::variant<int, double, std::string> var2 = 3.14;
  std::variant<int, double, std::string> var3 = "Hello";

  // Checking types
  std::cout << "var1 holds int: " << std::holds_alternative<int>(var1) << std::endl;
  std::cout << "var2 holds double: " << std::holds_alternative<double>(var2) << std::endl;
  std::cout << "var3 holds string: " << std::holds_alternative<std::string>(var3) << std::endl;

  // Getting values
  std::cout << "var1 value: " << std::get<int>(var1) << std::endl;
  std::cout << "var2 value: " << std::get<double>(var2) << std::endl;
  std::cout << "var3 value: " << std::get<std::string>(var3) << std::endl;

  // Using get_if for safer access
  if (auto pval = std::get_if<int>(&var1)) {
    std::cout << "var1 int value: " << *pval << std::endl;
  }

  if (auto pval = std::get_if<std::string>(&var1)) {
    std::cout << "var1 string value: " << *pval << std::endl;
  } else {
    std::cout << "var1 does not hold a string" << std::endl;
  }

  // Changing variant type
  var1 = "Now I'm a string";
  std::cout << "var1 after change: " << std::get<std::string>(var1) << std::endl;

  // Using std::visit for type-safe access
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, int>)
      std::cout << "int: " << arg << std::endl;
    else if constexpr (std::is_same_v<T, double>)
      std::cout << "double: " << arg << std::endl;
    else if constexpr (std::is_same_v<T, std::string>)
      std::cout << "string: " << arg << std::endl;
  };

  std::cout << "Visiting var1: ";
  std::visit(visitor, var1);
  std::cout << "Visiting var2: ";
  std::visit(visitor, var2);
  std::cout << "Visiting var3: ";
  std::visit(visitor, var3);

  // std::any
  std::cout << "\nstd::any:" << std::endl;

  std::any a1 = 42;
  std::any a2 = 3.14;
  std::any a3 = std::string("Hello, any!");
  std::any a4; // empty

  std::cout << "a1 has value: " << a1.has_value() << std::endl;
  std::cout << "a4 has value: " << a4.has_value() << std::endl;

  // Getting values
  std::cout << "a1 value: " << std::any_cast<int>(a1) << std::endl;
  std::cout << "a2 value: " << std::any_cast<double>(a2) << std::endl;
  std::cout << "a3 value: " << std::any_cast<std::string>(a3) << std::endl;

  // Safe access with any_cast with pointer
  if (int *i = std::any_cast<int>(&a1)) {
    std::cout << "a1 int value: " << *i << std::endl;
  }

  if (std::string *s = std::any_cast<std::string>(&a1)) {
    std::cout << "a1 string value: " << *s << std::endl;
  } else {
    std::cout << "a1 does not hold a string" << std::endl;
  }

  // Changing type
  a1 = std::string("Now I'm a string too");
  std::cout << "a1 after change: " << std::any_cast<std::string>(a1) << std::endl;

  // Bad any_cast will throw an exception
  try {
    std::cout << std::any_cast<int>(a1) << std::endl;
  } catch (const std::bad_any_cast &e) {
    std::cout << "Exception: " << e.what() << std::endl;
  }
}

/**
 * Demonstrate structured bindings (C++17)
 */
void demonstrate_structured_bindings() {
  std::cout << "\n=== Structured Bindings ===" << std::endl;

  // With array
  int arr[3] = {1, 2, 3};
  auto [a, b, c] = arr;
  std::cout << "Array decomposition: a=" << a << ", b=" << b << ", c=" << c << std::endl;

  // With tuple
  std::tuple<int, double, std::string> tuple_data(42, 3.14, "Hello");
  auto [id, value, name] = tuple_data;
  std::cout << "Tuple decomposition: id=" << id << ", value=" << value << ", name=" << name
            << std::endl;

  // With pair
  std::pair<int, std::string> pair_data(123, "Pair");
  auto [first, second] = pair_data;
  std::cout << "Pair decomposition: first=" << first << ", second=" << second << std::endl;

  // With custom struct
  struct Point {
    int x;
    int y;
    int z;
  };

  Point point = {10, 20, 30};
  auto [x, y, z] = point;
  std::cout << "Struct decomposition: x=" << x << ", y=" << y << ", z=" << z << std::endl;

  // With map iterator
  std::map<std::string, int> ages = {{"Alice", 25}, {"Bob", 30}, {"Charlie", 35}};

  std::cout << "Map entries using structured binding:" << std::endl;
  for (const auto &[name, age] : ages) {
    std::cout << name << " is " << age << " years old" << std::endl;
  }
}

/**
 * Demonstrate if constexpr and fold expressions (C++17)
 */
template <typename T> void check_type(const T &value) {
  if constexpr (std::is_integral_v<T>) {
    std::cout << "Value " << value << " is an integral type" << std::endl;
    if constexpr (std::is_signed_v<T>) {
      std::cout << "  It is signed" << std::endl;
    } else {
      std::cout << "  It is unsigned" << std::endl;
    }
  } else if constexpr (std::is_floating_point_v<T>) {
    std::cout << "Value " << value << " is a floating-point type" << std::endl;
    if constexpr (std::is_same_v<T, float>) {
      std::cout << "  It is a float" << std::endl;
    } else if constexpr (std::is_same_v<T, double>) {
      std::cout << "  It is a double" << std::endl;
    }
  } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char *>) {
    std::cout << "Value \"" << value << "\" is a string-like type" << std::endl;
  } else {
    std::cout << "Value is some other type" << std::endl;
  }
}

// Fold expressions with various operators
template <typename... Args> auto sum(Args... args) {
  return (args + ...); // Unary right fold with + operator
}

template <typename... Args> void print_all(Args... args) {
  (std::cout << ... << args) << std::endl; // Unary right fold with << operator
}

template <typename T, typename... Args> bool is_any_equal(T first, Args... args) {
  return (... || (first == args)); // Unary right fold with || operator
}

template <typename T, typename... Args> bool are_all_equal(T first, Args... args) {
  return (... && (first == args)); // Unary right fold with && operator
}

void demonstrate_if_constexpr_and_fold() {
  std::cout << "\n=== if constexpr and Fold Expressions ===" << std::endl;

  // if constexpr examples
  std::cout << "\nif constexpr examples:" << std::endl;
  check_type(42);
  check_type(42u);
  check_type(3.14159);
  check_type(3.14f);
  check_type(std::string("Hello"));
  check_type("World");
  check_type(std::vector<int>{1, 2, 3});

  // Fold expression examples
  std::cout << "\nFold expression examples:" << std::endl;
  std::cout << "Sum of 1, 2, 3, 4, 5: " << sum(1, 2, 3, 4, 5) << std::endl;
  std::cout << "Concatenation: ";
  print_all("Hello", ' ', "fold", ' ', "expressions", '!');

  std::cout << "Is any equal to 10 in (5, 10, 15)? " << std::boolalpha
            << is_any_equal(10, 5, 10, 15) << std::endl;
  std::cout << "Is any equal to 20 in (5, 10, 15)? " << is_any_equal(20, 5, 10, 15) << std::endl;

  std::cout << "Are all equal in (5, 5, 5)? " << are_all_equal(5, 5, 5) << std::endl;
  std::cout << "Are all equal in (5, 5, 10)? " << are_all_equal(5, 5, 10) << std::endl;
}

int main() {
  std::cout << "Modern C++ Features Demonstration\n" << std::endl;

  demonstrate_auto();
  demonstrate_range_based_for();
  demonstrate_lambdas();
  demonstrate_smart_pointers();
  demonstrate_move_semantics();
  demonstrate_optional_variant_any();
  demonstrate_structured_bindings();
  demonstrate_if_constexpr_and_fold();

  return 0;
}
