/**
 * @file arrays_pointers.c
 * @brief Demonstration of arrays and pointers in C
 *
 * This example shows:
 * - Array declarations and initialization
 * - Pointer declarations and usage
 * - Pointer arithmetic
 * - Arrays of pointers
 * - Pointers to arrays
 * - Function pointers
 * - Dynamic memory allocation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Simple function to add two integers
 * @param a First integer
 * @param b Second integer
 * @return Sum of a and b
 */
int add(int a, int b) {
    return a + b;
}

/**
 * @brief Simple function to subtract two integers
 * @param a First integer
 * @param b Second integer
 * @return Difference between a and b
 */
int subtract(int a, int b) {
    return a - b;
}

/**
 * @brief Function that takes a function pointer as an argument
 * @param a First integer
 * @param b Second integer
 * @param operation Function pointer to the operation to perform
 * @return Result of the operation
 */
int perform_operation(int a, int b, int (*operation)(int, int)) {
    return operation(a, b);
}

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
    // 1. Basic array declaration and initialization
    int numbers[5] = {10, 20, 30, 40, 50};
    printf("1. Basic array:\n");
    for (int i = 0; i < 5; i++) {
        printf("   numbers[%d] = %d\n", i, numbers[i]);
    }
    
    // 2. Basic pointer declaration and usage
    int *p_num = &numbers[0];  // Point to the first element
    printf("\n2. Basic pointer usage:\n");
    printf("   *p_num = %d (value at address %p)\n", *p_num, (void*)p_num);
    
    // 3. Pointer arithmetic
    printf("\n3. Pointer arithmetic:\n");
    for (int i = 0; i < 5; i++) {
        printf("   *(p_num + %d) = %d\n", i, *(p_num + i));
    }
    
    // 4. Pointers and arrays are closely related
    printf("\n4. Array name as pointer:\n");
    printf("   numbers[2] = %d\n", numbers[2]);
    printf("   *(numbers + 2) = %d\n", *(numbers + 2));
    
    // 5. Array of pointers
    int a = 100, b = 200, c = 300;
    int *ptr_array[3] = {&a, &b, &c};
    
    printf("\n5. Array of pointers:\n");
    for (int i = 0; i < 3; i++) {
        printf("   *ptr_array[%d] = %d\n", i, *ptr_array[i]);
    }
    
    // 6. Pointer to an array
    int (*arr_ptr)[5] = &numbers;  // Pointer to an array of 5 integers
    
    printf("\n6. Pointer to an array:\n");
    for (int i = 0; i < 5; i++) {
        printf("   (*arr_ptr)[%d] = %d\n", i, (*arr_ptr)[i]);
    }
    
    // 7. 2D arrays and pointers
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };
    
    printf("\n7. 2D array access:\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            printf("   matrix[%d][%d] = %d\n", i, j, matrix[i][j]);
        }
    }
    
    // 8. Dynamic memory allocation with pointers
    int *dynamic_array = (int*)malloc(5 * sizeof(int));
    
    if (dynamic_array != NULL) {
        printf("\n8. Dynamic array:\n");
        for (int i = 0; i < 5; i++) {
            dynamic_array[i] = i * 10;
            printf("   dynamic_array[%d] = %d\n", i, dynamic_array[i]);
        }
        
        // Don't forget to free!
        free(dynamic_array);
    }
    
    // 9. Function pointers
    int (*operation)(int, int);
    
    printf("\n9. Function pointers:\n");
    
    operation = add;
    printf("   add(5, 3) = %d\n", operation(5, 3));
    
    operation = subtract;
    printf("   subtract(5, 3) = %d\n", operation(5, 3));
    
    // 10. Passing function pointers to functions
    printf("\n10. Function pointer as argument:\n");
    printf("   perform_operation(10, 5, add) = %d\n", perform_operation(10, 5, add));
    printf("   perform_operation(10, 5, subtract) = %d\n", perform_operation(10, 5, subtract));
    
    // 11. Char arrays and pointers (strings)
    char str1[] = "Hello";
    char *str2 = "World";  // Note: string literals should be const char* in modern C
    
    printf("\n11. Strings (char arrays and pointers):\n");
    printf("   str1 = %s\n", str1);
    printf("   str2 = %s\n", str2);
    
    // 12. Array of strings
    char *fruits[] = {"Apple", "Banana", "Cherry", "Date"};
    
    printf("\n12. Array of strings:\n");
    for (int i = 0; i < 4; i++) {
        printf("   fruits[%d] = %s\n", i, fruits[i]);
    }
    
    return 0;
}
