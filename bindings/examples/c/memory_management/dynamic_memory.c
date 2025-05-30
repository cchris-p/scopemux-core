/**
 * Demonstration of C Memory Management
 *
 * This example covers:
 * - Dynamic memory allocation (malloc, calloc, realloc, free)
 * - Memory manipulation functions (memcpy, memmove, memset)
 * - Pointer arithmetic
 * - Memory safety considerations
 * - Custom memory allocation patterns
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * Basic dynamic memory allocation with malloc
 */
void demonstrate_malloc() {
    printf("\n=== Basic malloc and free ===\n");
    
    // Allocate memory for 5 integers
    int* numbers = (int*)malloc(5 * sizeof(int));
    
    // Check if allocation was successful
    if (numbers == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // Initialize the allocated memory
    for (int i = 0; i < 5; i++) {
        numbers[i] = i * 10;
    }
    
    // Use the allocated memory
    printf("Allocated array contents: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Free the allocated memory
    free(numbers);
    numbers = NULL; // Best practice: set freed pointers to NULL
    
    // Trying to use freed memory would cause undefined behavior
    // numbers[0] = 100; // This would be a bug!
}

/**
 * Using calloc for zero-initialized memory
 */
void demonstrate_calloc() {
    printf("\n=== Using calloc ===\n");
    
    // Allocate and zero-initialize memory for 5 integers
    int* numbers = (int*)calloc(5, sizeof(int));
    
    if (numbers == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // calloc initializes memory to zero
    printf("calloc initialized array: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Now assign some values
    for (int i = 0; i < 5; i++) {
        numbers[i] = (i + 1) * 5;
    }
    
    printf("After assignment: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Free the allocated memory
    free(numbers);
    numbers = NULL;
}

/**
 * Using realloc to resize allocated memory
 */
void demonstrate_realloc() {
    printf("\n=== Using realloc ===\n");
    
    // Start with a small array
    int* numbers = (int*)malloc(3 * sizeof(int));
    
    if (numbers == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // Initialize the array
    for (int i = 0; i < 3; i++) {
        numbers[i] = i + 1;
    }
    
    printf("Initial array: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Resize the array to hold 5 integers
    int* new_numbers = (int*)realloc(numbers, 5 * sizeof(int));
    
    if (new_numbers == NULL) {
        printf("Memory reallocation failed\n");
        free(numbers); // Original memory must still be freed if realloc fails
        return;
    }
    
    // The original data is preserved; realloc copied it for us
    numbers = new_numbers; // Update our pointer
    
    // Initialize the new elements
    numbers[3] = 4;
    numbers[4] = 5;
    
    printf("After realloc (expanded): ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Shrink the array
    new_numbers = (int*)realloc(numbers, 2 * sizeof(int));
    
    if (new_numbers == NULL) {
        printf("Memory reallocation failed\n");
        free(numbers);
        return;
    }
    
    numbers = new_numbers;
    
    printf("After realloc (shrunk): ");
    for (int i = 0; i < 2; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Free the memory
    free(numbers);
    numbers = NULL;
}

/**
 * Demonstrate pointer arithmetic and memory access
 */
void demonstrate_pointer_arithmetic() {
    printf("\n=== Pointer Arithmetic ===\n");
    
    // Allocate an array of 5 integers
    int* arr = (int*)malloc(5 * sizeof(int));
    
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // Initialize array
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    
    // Basic array indexing
    printf("Array indexing: arr[2] = %d\n", arr[2]);
    
    // Equivalent pointer arithmetic
    printf("Pointer arithmetic: *(arr + 2) = %d\n", *(arr + 2));
    
    // Moving the pointer (unsafe, must keep track of original pointer for freeing)
    int* ptr = arr;
    printf("Original pointer position: *ptr = %d\n", *ptr);
    
    // Move pointer to next element
    ptr++;
    printf("After ptr++: *ptr = %d\n", *ptr);
    
    // Move pointer by an offset
    ptr = arr + 3;
    printf("After ptr = arr + 3: *ptr = %d\n", *ptr);
    
    // Difference between pointers
    int* ptr2 = arr + 4;
    ptrdiff_t diff = ptr2 - ptr;
    printf("Pointer difference (ptr2 - ptr): %td elements\n", diff);
    
    // Free the original pointer, not the moved one
    free(arr);
    arr = NULL;
    ptr = NULL; // Not necessary, but good practice
    ptr2 = NULL;
}

/**
 * Demonstrate memory manipulation functions
 */
void demonstrate_memory_functions() {
    printf("\n=== Memory Manipulation Functions ===\n");
    
    // Allocate memory
    char* buffer1 = (char*)malloc(20);
    char* buffer2 = (char*)malloc(20);
    
    if (buffer1 == NULL || buffer2 == NULL) {
        free(buffer1); // Handle partial allocation failure
        free(buffer2);
        printf("Memory allocation failed\n");
        return;
    }
    
    // memset: Fill memory with a specific byte
    memset(buffer1, 'A', 19);
    buffer1[19] = '\0'; // Null-terminate for printing
    printf("After memset: %s\n", buffer1);
    
    // memcpy: Copy non-overlapping memory
    const char* source = "Hello, World!";
    memcpy(buffer2, source, strlen(source) + 1); // +1 for the null terminator
    printf("After memcpy: %s\n", buffer2);
    
    // memmove: Safe for overlapping regions
    char overlap[20] = "abcdefghijklmnopqrs";
    printf("Before memmove: %s\n", overlap);
    
    // Move "defghi" to position starting at "bcdef"
    memmove(overlap + 1, overlap + 3, 6);
    printf("After memmove: %s\n", overlap);
    
    // memcmp: Compare memory regions
    char str1[] = "Hello";
    char str2[] = "Hello";
    char str3[] = "World";
    
    int cmp1 = memcmp(str1, str2, 5);
    int cmp2 = memcmp(str1, str3, 5);
    
    printf("memcmp(str1, str2) = %d (same content)\n", cmp1);
    printf("memcmp(str1, str3) = %d (different content)\n", cmp2);
    
    // Clean up
    free(buffer1);
    free(buffer2);
    buffer1 = NULL;
    buffer2 = NULL;
}

/**
 * Demonstrate a common pitfall: memory leaks
 */
void demonstrate_memory_leaks() {
    printf("\n=== Memory Leaks ===\n");
    
    // Allocate memory
    int* data = (int*)malloc(sizeof(int) * 10);
    
    if (data == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    printf("Memory allocated at %p\n", (void*)data);
    
    // Example of code paths that might cause leaks
    int error_condition = 0; // Change to 1 to simulate error path
    
    if (error_condition) {
        printf("Error occurred, returning early\n");
        return; // This would leak memory if we forgot to free
    }
    
    // Nested allocations make leak tracking harder
    int* more_data = (int*)malloc(sizeof(int) * 5);
    
    if (more_data == NULL) {
        printf("Second allocation failed\n");
        free(data); // Don't forget to free the first allocation
        return;
    }
    
    printf("More memory allocated at %p\n", (void*)more_data);
    
    // Proper cleanup
    free(more_data);
    free(data);
    more_data = NULL;
    data = NULL;
    
    printf("All memory properly freed\n");
}

/**
 * Demonstrate a simple custom memory pool
 */
#define POOL_SIZE 1024
static char memory_pool[POOL_SIZE];
static size_t pool_offset = 0;

void* pool_alloc(size_t size) {
    // Simple bump allocator
    if (pool_offset + size > POOL_SIZE) {
        return NULL; // Out of pool memory
    }
    
    void* ptr = &memory_pool[pool_offset];
    pool_offset += size;
    
    // Ensure alignment (simplified)
    pool_offset = (pool_offset + 7) & ~7;
    
    return ptr;
}

void pool_reset() {
    pool_offset = 0;
}

void demonstrate_memory_pool() {
    printf("\n=== Custom Memory Pool ===\n");
    
    // Reset the pool to start fresh
    pool_reset();
    
    // Allocate some memory from the pool
    int* numbers = (int*)pool_alloc(5 * sizeof(int));
    
    if (numbers == NULL) {
        printf("Pool allocation failed\n");
        return;
    }
    
    // Initialize the memory
    for (int i = 0; i < 5; i++) {
        numbers[i] = i * 100;
    }
    
    printf("Pool allocated array: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Allocate another block
    char* text = (char*)pool_alloc(20);
    
    if (text == NULL) {
        printf("Pool allocation failed\n");
        return;
    }
    
    strcpy(text, "Hello from pool");
    printf("Pool allocated string: %s\n", text);
    
    printf("Pool usage: %zu bytes\n", pool_offset);
    
    // No need to free individual allocations with this simple pool
    // Just reset the entire pool when done
    pool_reset();
    printf("Pool reset, usage: %zu bytes\n", pool_offset);
    
    // The memory for numbers and text is now available for reuse
    // but accessing them would be dangerous as the memory may be reused
}

/**
 * Demonstrate handling of multi-dimensional arrays with dynamic allocation
 */
void demonstrate_multidimensional_arrays() {
    printf("\n=== Multi-dimensional Arrays ===\n");
    
    int rows = 3;
    int cols = 4;
    
    // Method 1: Allocate a single block for 2D array
    int* matrix1 = (int*)malloc(rows * cols * sizeof(int));
    
    if (matrix1 == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // Initialize and access using row-major indexing math
    printf("Method 1 (single block):\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix1[i * cols + j] = i * 10 + j;
            printf("%2d ", matrix1[i * cols + j]);
        }
        printf("\n");
    }
    
    // Method 2: Array of pointers (jagged array)
    int** matrix2 = (int**)malloc(rows * sizeof(int*));
    
    if (matrix2 == NULL) {
        printf("Memory allocation failed\n");
        free(matrix1);
        return;
    }
    
    // Allocate each row
    for (int i = 0; i < rows; i++) {
        matrix2[i] = (int*)malloc(cols * sizeof(int));
        
        if (matrix2[i] == NULL) {
            // Clean up previously allocated rows
            for (int j = 0; j < i; j++) {
                free(matrix2[j]);
            }
            free(matrix2);
            free(matrix1);
            printf("Memory allocation failed for row %d\n", i);
            return;
        }
    }
    
    // Initialize and access using natural 2D indexing
    printf("\nMethod 2 (array of pointers):\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix2[i][j] = i * 10 + j;
            printf("%2d ", matrix2[i][j]);
        }
        printf("\n");
    }
    
    // Clean up method 2 (must free each row)
    for (int i = 0; i < rows; i++) {
        free(matrix2[i]);
    }
    free(matrix2);
    
    // Clean up method 1
    free(matrix1);
    
    matrix1 = NULL;
    matrix2 = NULL;
}

int main() {
    printf("C Memory Management Demonstration\n");
    
    demonstrate_malloc();
    demonstrate_calloc();
    demonstrate_realloc();
    demonstrate_pointer_arithmetic();
    demonstrate_memory_functions();
    demonstrate_memory_leaks();
    demonstrate_memory_pool();
    demonstrate_multidimensional_arrays();
    
    printf("\nAll demonstrations completed successfully\n");
    return 0;
}
