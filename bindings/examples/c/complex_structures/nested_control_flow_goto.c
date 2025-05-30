/**
 * @file nested_control_flow_goto.c
 * @brief Demonstration of complex nested control flow and goto statements in C
 *
 * This example shows:
 * - Deeply nested if/else structures
 * - Nested loops (for, while)
 * - The use of goto statements for control flow
 * - Break and continue statements
 */

#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
    int i, j, k;
    bool found = false;

    // Example 1: Nested loops with break
    printf("Example 1: Nested loops with break\n");
    
    for (i = 0; i < 5; i++) {
        printf("Outer loop i=%d\n", i);
        
        for (j = 0; j < 3; j++) {
            printf("  Middle loop j=%d\n", j);
            
            for (k = 0; k < 2; k++) {
                printf("    Inner loop k=%d\n", k);
                
                if (i == 2 && j == 1 && k == 0) {
                    printf("    Found target values! Breaking inner loop\n");
                    break;
                }
            }
            
            if (i == 2 && j == 1) {
                printf("  Breaking middle loop\n");
                break;
            }
        }
        
        if (i == 2) {
            printf("Breaking outer loop\n");
            break;
        }
    }

    // Example 2: Goto for error handling
    printf("\nExample 2: Goto for error handling\n");
    
    int result = 0;
    int *data = NULL;
    
    // Simulating an initialization sequence with error handling
    if (result != 0) {
        printf("Step 1 failed\n");
        goto cleanup;
    }
    
    printf("Step 1 succeeded\n");
    
    if (data == NULL) {
        printf("Step 2 failed\n");
        result = -1;
        goto cleanup;
    }
    
    printf("Step 2 succeeded\n");
    
    // Imagine more steps here...
    
    // Normal exit point
    printf("All steps completed successfully\n");
    result = 0;
    
cleanup:
    // Cleanup code runs regardless of how we reached this point
    printf("Performing cleanup (result=%d)\n", result);
    
    // Example 3: Complex nested conditions
    printf("\nExample 3: Complex nested conditions\n");
    
    int a = 5, b = 10, c = 15;
    
    if (a < 10) {
        printf("a < 10\n");
        
        if (b > 5) {
            printf("  b > 5\n");
            
            if (c > 20) {
                printf("    c > 20\n");
            } else if (c > 10) {
                printf("    c > 10\n");
                
                if (a + b > c) {
                    printf("      a + b > c\n");
                } else {
                    printf("      a + b <= c\n");
                }
            } else {
                printf("    c <= 10\n");
            }
        } else {
            printf("  b <= 5\n");
        }
    } else {
        printf("a >= 10\n");
    }

    return 0;
}
