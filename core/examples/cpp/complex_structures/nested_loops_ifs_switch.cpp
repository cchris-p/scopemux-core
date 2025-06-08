/**
 * @file nested_loops_ifs_switch.cpp
 * @brief Demonstration of complex nested control structures in C++
 *
 * This example shows:
 * - Deeply nested loops
 * - Complex nested conditional structures
 * - Switch statements within loops and conditions
 * - Try-catch blocks within control structures
 */

#include <iostream>
#include <stdexcept>
#include <vector>

/**
 * @brief Process a set of nested data with complex control flow
 * @param data The multi-dimensional data to process
 * @return Number of elements processed
 */
int process_nested_data(const std::vector<std::vector<int>> &data) {
  int processed = 0;

  for (size_t i = 0; i < data.size(); ++i) {
    const auto &row = data[i];

    std::cout << "Processing row " << i << " (size: " << row.size() << ")" << std::endl;

    for (size_t j = 0; j < row.size(); ++j) {
      const int value = row[j];

      // Skip processing for certain values
      if (value < 0) {
        std::cout << "  Skipping negative value at [" << i << "][" << j << "]: " << value
                  << std::endl;
        continue;
      }

      // Process based on value categories
      if (value == 0) {
        std::cout << "  Found zero value at [" << i << "][" << j << "]" << std::endl;
      } else if (value > 0 && value <= 10) {
        std::cout << "  Processing small value at [" << i << "][" << j << "]: " << value
                  << std::endl;

        // Nested switch for small values
        switch (value) {
        case 1:
        case 2:
          std::cout << "    Very small value (1-2)" << std::endl;
          break;
        case 3:
        case 4:
        case 5:
          std::cout << "    Small value (3-5)" << std::endl;
          break;
        default:
          std::cout << "    Medium-small value (6-10)" << std::endl;
          break;
        }

        processed++;
      } else {
        std::cout << "  Processing large value at [" << i << "][" << j << "]: " << value
                  << std::endl;

        // Nested try-catch within the condition
        try {
          if (value > 100) {
            throw std::overflow_error("Value is too large to process safely");
          }

          // More nested conditions
          if (value % 2 == 0) {
            std::cout << "    Large even value" << std::endl;
          } else {
            std::cout << "    Large odd value" << std::endl;
          }

          processed++;
        } catch (const std::exception &e) {
          std::cerr << "    Error: " << e.what() << std::endl;
        }
      }
    }
  }

  return processed;
}

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
  // Create a sample nested data structure
  std::vector<std::vector<int>> nested_data = {
      {1, 2, 3, 4, 5}, {10, 20, 30, -5, 0}, {-1, 50, 150, 75, 42}};

  std::cout << "Starting complex nested control flow example..." << std::endl;

  // Process data with complex control flow
  int result = process_nested_data(nested_data);

  std::cout << "Processed " << result << " elements successfully" << std::endl;

  return 0;
}
