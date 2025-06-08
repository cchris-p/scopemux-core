/**
 * @file complex_data_structures.cpp
 * @brief Demonstration of complex data structures in C++
 *
 * This example shows:
 * - STL containers (vector, map, set, list, etc.)
 * - Container adaptors (stack, queue, priority_queue)
 * - Custom data structures with composition
 * - Nested containers
 * - Working with iterators
 */

#include <algorithm>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief A simple custom data structure
 */
class Point {
public:
  Point(int x = 0, int y = 0) : x_(x), y_(y) {}

  int x() const { return x_; }
  int y() const { return y_; }

  // For comparison in sets and maps
  bool operator<(const Point &other) const {
    if (x_ == other.x_)
      return y_ < other.y_;
    return x_ < other.x_;
  }

  friend std::ostream &operator<<(std::ostream &os, const Point &p) {
    os << "(" << p.x_ << ", " << p.y_ << ")";
    return os;
  }

private:
  int x_;
  int y_;
};

/**
 * @brief A more complex data structure with composition
 */
class Shape {
public:
  virtual ~Shape() = default;
  virtual double area() const = 0;
  virtual void print() const = 0;
};

class Circle : public Shape {
public:
  Circle(const Point &center, double radius) : center_(center), radius_(radius) {}

  double area() const override { return 3.14159 * radius_ * radius_; }

  void print() const override {
    std::cout << "Circle at " << center_ << " with radius " << radius_ << std::endl;
  }

private:
  Point center_;
  double radius_;
};

class Rectangle : public Shape {
public:
  Rectangle(const Point &topLeft, const Point &bottomRight)
      : topLeft_(topLeft), bottomRight_(bottomRight) {}

  double area() const override {
    return std::abs((bottomRight_.x() - topLeft_.x()) * (bottomRight_.y() - topLeft_.y()));
  }

  void print() const override {
    std::cout << "Rectangle from " << topLeft_ << " to " << bottomRight_ << std::endl;
  }

private:
  Point topLeft_;
  Point bottomRight_;
};

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
  // 1. Basic vector usage
  std::cout << "1. Basic vector usage:" << std::endl;
  std::vector<int> numbers = {1, 2, 3, 4, 5};

  for (const auto &num : numbers) {
    std::cout << "  " << num << std::endl;
  }

  // 2. Map example
  std::cout << "\n2. Map example:" << std::endl;
  std::map<std::string, int> ages;
  ages["Alice"] = 30;
  ages["Bob"] = 25;
  ages["Charlie"] = 40;

  for (const auto &[name, age] : ages) {
    std::cout << "  " << name << " is " << age << " years old" << std::endl;
  }

  // 3. Set of custom objects
  std::cout << "\n3. Set of custom objects:" << std::endl;
  std::set<Point> points;
  points.insert(Point(1, 2));
  points.insert(Point(3, 4));
  points.insert(Point(1, 2)); // Duplicate, will be ignored
  points.insert(Point(5, 6));

  for (const auto &point : points) {
    std::cout << "  " << point << std::endl;
  }

  // 4. Nested containers
  std::cout << "\n4. Nested containers:" << std::endl;
  std::map<std::string, std::vector<int>> studentScores;
  studentScores["Alice"] = {90, 85, 92};
  studentScores["Bob"] = {78, 80, 85};

  for (const auto &[student, scores] : studentScores) {
    std::cout << "  " << student << "'s scores: ";
    for (const auto &score : scores) {
      std::cout << score << " ";
    }
    std::cout << std::endl;
  }

  // 5. Container adaptors
  std::cout << "\n5. Stack example:" << std::endl;
  std::stack<int> intStack;
  intStack.push(10);
  intStack.push(20);
  intStack.push(30);

  while (!intStack.empty()) {
    std::cout << "  Popped: " << intStack.top() << std::endl;
    intStack.pop();
  }

  // 6. Priority queue
  std::cout << "\n6. Priority queue example:" << std::endl;
  std::priority_queue<int> pq;
  pq.push(10);
  pq.push(30);
  pq.push(20);
  pq.push(5);

  while (!pq.empty()) {
    std::cout << "  Popped: " << pq.top() << std::endl;
    pq.pop();
  }

  // 7. Working with algorithms
  std::cout << "\n7. Algorithm example:" << std::endl;
  std::vector<int> data = {5, 2, 8, 1, 9};

  std::cout << "  Original data: ";
  for (const auto &num : data) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  std::sort(data.begin(), data.end());

  std::cout << "  Sorted data: ";
  for (const auto &num : data) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  // 8. Complex object hierarchy with smart pointers
  std::cout << "\n8. Complex object hierarchy:" << std::endl;
  std::vector<std::unique_ptr<Shape>> shapes;

  shapes.push_back(std::make_unique<Circle>(Point(0, 0), 5.0));
  shapes.push_back(std::make_unique<Rectangle>(Point(0, 0), Point(4, 3)));
  shapes.push_back(std::make_unique<Circle>(Point(5, 5), 2.5));

  for (const auto &shape : shapes) {
    shape->print();
    std::cout << "  Area: " << shape->area() << std::endl;
  }

  return 0;
}
