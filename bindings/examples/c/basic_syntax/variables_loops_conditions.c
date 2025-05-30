/**
 * @file variables_loops_conditions.c
 * @brief Demonstration of variables, loops, and conditional statements in C
 *
 * This example shows:
 * - Various variable declarations and types
 * - for, while, and do-while loops
 * - if, else if, else conditions
 * - switch statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
    int array[5] = {1, 2, 3, 4, 5};
    
    // For loop
    printf("For loop example:\n");
    for (i = 0; i < 5; i++) {
        printf("array[%d] = %d\n", i, array[i]);
    }
    
    // While loop
    printf("\nWhile loop example:\n");
    i = 0;
    while (i < 5) {
        printf("iteration %d\n", i);
        i++;
    }
    
    // Do-while loop
    printf("\nDo-while loop example:\n");
    i = 0;
    do {
        printf("iteration %d\n", i);
        i++;
    } while (i < 5);
    
    // If-else statements
    printf("\nIf-else example:\n");
    if (b) {
        printf("b is true\n");
    } else {
        printf("b is false\n");
    }
    
    // If-else if-else
    printf("\nIf-else if-else example:\n");
    if (i < 3) {
        printf("i is less than 3\n");
    } else if (i == 5) {
        printf("i is equal to 5\n");
    } else {
        printf("i is greater than 5\n");
    }
    
    // Switch statement
    printf("\nSwitch example:\n");
    switch (c) {
        case 'A':
            printf("c is 'A'\n");
            break;
        case 'B':
            printf("c is 'B'\n");
            break;
        default:
            printf("c is not 'A' or 'B'\n");
            break;
    }
    
    return 0;
}
