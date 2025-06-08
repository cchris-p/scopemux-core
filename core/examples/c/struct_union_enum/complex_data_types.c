/**
 * @file complex_data_types.c
 * @brief Demonstration of structures, unions, and enumerations in C
 *
 * This example shows:
 * - Structure definitions and usage
 * - Nested structures
 * - Unions
 * - Enumerations
 * - Typedef
 * - Bit fields
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Simple enumeration
enum Color {
    RED,    // 0 by default
    GREEN,  // 1
    BLUE,   // 2
    YELLOW = 10,
    PURPLE, // 11
    ORANGE  // 12
};

// Enumeration with typedef
typedef enum {
    MONDAY = 1,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
} Weekday;

// Simple structure
struct Point {
    int x;
    int y;
};

// Structure with typedef
typedef struct {
    char title[50];
    char author[50];
    int year;
    float price;
} Book;

// Nested structure
struct Rectangle {
    struct Point topLeft;
    struct Point bottomRight;
};

// Structure with bit fields
typedef struct {
    unsigned int is_active : 1;    // 1 bit
    unsigned int permission : 3;   // 3 bits (0-7)
    unsigned int user_level : 4;   // 4 bits (0-15)
} Flags;

// Union example
union Data {
    int i;
    float f;
    char str[20];
};

// Structure containing a union
typedef struct {
    enum Color color;
    union {
        int i_value;
        float f_value;
        char c_value;
    } value;
} ColoredValue;

/**
 * @brief Print details about a Point structure
 * @param p The Point to print
 */
void print_point(struct Point p) {
    printf("Point(%d, %d)\n", p.x, p.y);
}

/**
 * @brief Calculate the area of a Rectangle
 * @param rect The Rectangle to calculate the area of
 * @return The area of the rectangle
 */
int rectangle_area(struct Rectangle rect) {
    int width = rect.bottomRight.x - rect.topLeft.x;
    int height = rect.bottomRight.y - rect.topLeft.y;
    return width * height;
}

/**
 * @brief Convert a Color enum to its string representation
 * @param color The Color to convert
 * @return String representation of the color
 */
const char* color_to_string(enum Color color) {
    switch (color) {
        case RED:    return "Red";
        case GREEN:  return "Green";
        case BLUE:   return "Blue";
        case YELLOW: return "Yellow";
        case PURPLE: return "Purple";
        case ORANGE: return "Orange";
        default:     return "Unknown";
    }
}

/**
 * @brief Program entry point
 * @return Exit status code
 */
int main() {
    // Using enumerations
    enum Color background = BLUE;
    Weekday today = WEDNESDAY;
    
    printf("Background color: %s (value: %d)\n", color_to_string(background), background);
    printf("Today is day %d of the week\n", today);
    
    // Using structures
    struct Point p1 = {10, 20};
    struct Point p2 = {.y = 5, .x = 15}; // Designated initializers
    
    printf("\nPoints:\n");
    print_point(p1);
    print_point(p2);
    
    // Using nested structures
    struct Rectangle rect = {
        .topLeft = {0, 0},
        .bottomRight = {5, 10}
    };
    
    printf("\nRectangle:\n");
    printf("Top-left: ");
    print_point(rect.topLeft);
    printf("Bottom-right: ");
    print_point(rect.bottomRight);
    printf("Area: %d\n", rectangle_area(rect));
    
    // Using typedef structures
    Book book1;
    strcpy(book1.title, "C Programming");
    strcpy(book1.author, "Dennis Ritchie");
    book1.year = 1978;
    book1.price = 29.99;
    
    printf("\nBook:\n");
    printf("Title: %s\n", book1.title);
    printf("Author: %s\n", book1.author);
    printf("Year: %d\n", book1.year);
    printf("Price: $%.2f\n", book1.price);
    
    // Using bit fields
    Flags user_flags = {1, 5, 10};
    
    printf("\nUser flags:\n");
    printf("Active: %d\n", user_flags.is_active);
    printf("Permission: %d\n", user_flags.permission);
    printf("User level: %d\n", user_flags.user_level);
    
    // Using unions
    union Data data;
    
    data.i = 10;
    printf("\nUnion as int: %d\n", data.i);
    
    data.f = 220.5;
    printf("Union as float: %.1f\n", data.f);
    
    strcpy(data.str, "C Programming");
    printf("Union as string: %s\n", data.str);
    
    // Using structure with union
    ColoredValue cv = {RED, .value.i_value = 100};
    printf("\nColored value:\n");
    printf("Color: %s\n", color_to_string(cv.color));
    printf("Integer value: %d\n", cv.value.i_value);
    
    cv.value.f_value = 99.9;
    printf("Float value (same memory): %f\n", cv.value.f_value);
    
    return 0;
}
