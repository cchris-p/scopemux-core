/**
 * @file variables_loops_conditions.cpp
 * @brief Demonstration of variables, loops, and conditional statements in C++
 *
 * This example shows:
 * - Various variable declarations and types including auto
 * - for, range-based for, while, and do-while loops
 * - if, else if, else conditions
 * - switch statements
 */

#include <iostream>
#include <vector>
#include <string>

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
    // Variable declarations
    int i = 0;
    float f = 3.14f;
    double d = 2.71828;
    char c = 'A';
    bool b = true;
    std::string s = "Hello, C++";
    auto a = 42;  // Type inference
    
    // Standard array
    int array[5] = {1, 2, 3, 4, 5};
    
    // STL container
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // Standard for loop
    std::cout << "Standard for loop example:" << std::endl;
    for (i = 0; i < 5; i++) {
        std::cout << "array[" << i << "] = " << array[i] << std::endl;
    }
    
    // Range-based for loop (C++11)
    std::cout << "\nRange-based for loop example:" << std::endl;
    for (const auto& value : vec) {
        std::cout << "value: " << value << std::endl;
    }
    
    // While loop
    std::cout << "\nWhile loop example:" << std::endl;
    i = 0;
    while (i < 5) {
        std::cout << "iteration " << i << std::endl;
        i++;
    }
    
    // Do-while loop
    std::cout << "\nDo-while loop example:" << std::endl;
    i = 0;
    do {
        std::cout << "iteration " << i << std::endl;
        i++;
    } while (i < 5);
    
    // If-else statements
    std::cout << "\nIf-else example:" << std::endl;
    if (b) {
        std::cout << "b is true" << std::endl;
    } else {
        std::cout << "b is false" << std::endl;
    }
    
    // If-else if-else
    std::cout << "\nIf-else if-else example:" << std::endl;
    if (i < 3) {
        std::cout << "i is less than 3" << std::endl;
    } else if (i == 5) {
        std::cout << "i is equal to 5" << std::endl;
    } else {
        std::cout << "i is greater than 5" << std::endl;
    }
    
    // Switch statement
    std::cout << "\nSwitch example:" << std::endl;
    switch (c) {
        case 'A':
            std::cout << "c is 'A'" << std::endl;
            break;
        case 'B':
            std::cout << "c is 'B'" << std::endl;
            break;
        default:
            std::cout << "c is not 'A' or 'B'" << std::endl;
            break;
    }
    
    return 0;
}
