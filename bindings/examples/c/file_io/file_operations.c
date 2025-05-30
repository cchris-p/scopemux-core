/**
 * Demonstration of File I/O in C
 *
 * This example covers:
 * - Opening and closing files
 * - Reading from files (character, line, and binary reads)
 * - Writing to files (formatted, character, and binary writes)
 * - File positioning (seeking)
 * - Error handling with file operations
 * - Temporary files
 * - File buffering control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define TEST_FILENAME "test_file.txt"
#define TEST_BINARY_FILENAME "test_binary.dat"

/**
 * Demonstrate basic file opening and closing
 */
void demonstrate_file_open_close() {
    printf("\n=== File Opening and Closing ===\n");
    
    // Open a file for writing
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file for writing: %s\n", strerror(errno));
        return;
    }
    
    printf("File opened successfully for writing\n");
    
    // Write some data
    fprintf(file, "Hello, File I/O!\n");
    fprintf(file, "This is a test file.\n");
    
    // Close the file
    if (fclose(file) != 0) {
        printf("Error closing file: %s\n", strerror(errno));
        return;
    }
    
    printf("File closed successfully\n");
    
    // Open the file again for reading
    file = fopen(TEST_FILENAME, "r");
    
    if (file == NULL) {
        printf("Error opening file for reading: %s\n", strerror(errno));
        return;
    }
    
    printf("File opened successfully for reading\n");
    
    // Close the file again
    if (fclose(file) != 0) {
        printf("Error closing file: %s\n", strerror(errno));
        return;
    }
    
    printf("File closed successfully\n");
}

/**
 * Demonstrate character-by-character file reading
 */
void demonstrate_character_io() {
    printf("\n=== Character I/O ===\n");
    
    // Open file for writing
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Write characters to the file
    const char* text = "Character-by-character I/O example";
    for (int i = 0; text[i] != '\0'; i++) {
        if (fputc(text[i], file) == EOF) {
            printf("Error writing character: %s\n", strerror(errno));
            fclose(file);
            return;
        }
    }
    
    fclose(file);
    
    // Open file for reading
    file = fopen(TEST_FILENAME, "r");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Read characters from the file
    printf("Reading character by character: ");
    int c;
    while ((c = fgetc(file)) != EOF) {
        putchar(c);
    }
    printf("\n");
    
    fclose(file);
}

/**
 * Demonstrate line-by-line file reading
 */
void demonstrate_line_io() {
    printf("\n=== Line I/O ===\n");
    
    // Open file for writing
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Write lines to the file
    fputs("First line of text\n", file);
    fputs("Second line of text\n", file);
    fputs("Third line of text\n", file);
    fputs("Last line without newline", file);
    
    fclose(file);
    
    // Open file for reading
    file = fopen(TEST_FILENAME, "r");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Read lines from the file
    char buffer[BUFFER_SIZE];
    int line_number = 1;
    
    printf("Reading line by line:\n");
    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        printf("Line %d: %s", line_number++, buffer);
        
        // Add newline if not present
        if (buffer[strlen(buffer) - 1] != '\n') {
            printf("\n");
        }
    }
    
    fclose(file);
}

/**
 * Demonstrate formatted I/O with files
 */
void demonstrate_formatted_io() {
    printf("\n=== Formatted I/O ===\n");
    
    // Open file for writing
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Write formatted data
    int id = 12345;
    double value = 123.456;
    char name[] = "Example";
    
    fprintf(file, "ID: %d\n", id);
    fprintf(file, "Value: %.3f\n", value);
    fprintf(file, "Name: %s\n", name);
    
    // Write a table
    fprintf(file, "\nTable of data:\n");
    fprintf(file, "%-10s %-10s %-10s\n", "Column1", "Column2", "Column3");
    fprintf(file, "%-10d %-10.2f %-10s\n", 1, 10.5, "Row1");
    fprintf(file, "%-10d %-10.2f %-10s\n", 2, 20.5, "Row2");
    fprintf(file, "%-10d %-10.2f %-10s\n", 3, 30.5, "Row3");
    
    fclose(file);
    
    // Open file for reading
    file = fopen(TEST_FILENAME, "r");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Read formatted data
    int read_id;
    double read_value;
    char read_name[50];
    
    fscanf(file, "ID: %d\n", &read_id);
    fscanf(file, "Value: %lf\n", &read_value);
    fscanf(file, "Name: %s\n", read_name);
    
    printf("Read from file:\n");
    printf("ID: %d\n", read_id);
    printf("Value: %.3f\n", read_value);
    printf("Name: %s\n", read_name);
    
    // Read the rest of the file and print it
    char buffer[BUFFER_SIZE];
    printf("\nRest of the file:\n");
    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        printf("%s", buffer);
    }
    
    fclose(file);
}

/**
 * Demonstrate binary I/O with files
 */
typedef struct {
    int id;
    double value;
    char name[20];
} Record;

void demonstrate_binary_io() {
    printf("\n=== Binary I/O ===\n");
    
    // Create some records
    Record records[3] = {
        {1, 10.5, "Record1"},
        {2, 20.5, "Record2"},
        {3, 30.5, "Record3"}
    };
    
    // Open file for binary writing
    FILE* file = fopen(TEST_BINARY_FILENAME, "wb");
    
    if (file == NULL) {
        printf("Error opening binary file: %s\n", strerror(errno));
        return;
    }
    
    // Write the number of records
    int num_records = 3;
    fwrite(&num_records, sizeof(int), 1, file);
    
    // Write records to the file
    size_t records_written = fwrite(records, sizeof(Record), num_records, file);
    printf("Wrote %zu records to binary file\n", records_written);
    
    fclose(file);
    
    // Open file for binary reading
    file = fopen(TEST_BINARY_FILENAME, "rb");
    
    if (file == NULL) {
        printf("Error opening binary file: %s\n", strerror(errno));
        return;
    }
    
    // Read the number of records
    int read_num_records;
    fread(&read_num_records, sizeof(int), 1, file);
    
    // Read records from the file
    Record read_records[3];
    size_t records_read = fread(read_records, sizeof(Record), read_num_records, file);
    
    printf("Read %zu records from binary file:\n", records_read);
    for (int i = 0; i < records_read; i++) {
        printf("Record %d: ID=%d, Value=%.1f, Name=%s\n", 
               i+1, read_records[i].id, read_records[i].value, read_records[i].name);
    }
    
    fclose(file);
}

/**
 * Demonstrate file positioning with fseek, ftell, and rewind
 */
void demonstrate_file_positioning() {
    printf("\n=== File Positioning ===\n");
    
    // Open file for writing
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Write data
    fprintf(file, "Line 1: This is the first line\n");
    fprintf(file, "Line 2: This is the second line\n");
    fprintf(file, "Line 3: This is the third line\n");
    fprintf(file, "Line 4: This is the fourth line\n");
    fprintf(file, "Line 5: This is the fifth line\n");
    
    fclose(file);
    
    // Open file for reading and writing
    file = fopen(TEST_FILENAME, "r+");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Get current position
    long position = ftell(file);
    printf("Initial position: %ld\n", position);
    
    // Read the first line
    char buffer[BUFFER_SIZE];
    fgets(buffer, BUFFER_SIZE, file);
    printf("Read: %s", buffer);
    
    // Get new position
    position = ftell(file);
    printf("Position after reading first line: %ld\n", position);
    
    // Skip to the third line (seek from current position)
    fgets(buffer, BUFFER_SIZE, file); // Read second line
    printf("Read: %s", buffer);
    
    // Get new position
    position = ftell(file);
    printf("Position after reading second line: %ld\n", position);
    
    // Seek to the beginning of the file
    rewind(file);
    position = ftell(file);
    printf("Position after rewind: %ld\n", position);
    
    // Seek to a specific position (start of third line)
    char temp[BUFFER_SIZE];
    fgets(temp, BUFFER_SIZE, file); // Skip first line
    fgets(temp, BUFFER_SIZE, file); // Skip second line
    position = ftell(file);
    
    // Read the third line
    fgets(buffer, BUFFER_SIZE, file);
    printf("Third line: %s", buffer);
    
    // Seek to end of file
    fseek(file, 0, SEEK_END);
    position = ftell(file);
    printf("Position at end of file: %ld\n", position);
    
    // Seek backward from the end to read the last line
    // Find the last newline character
    long offset = -2;  // Start with -2 to skip the final newline
    int character;
    
    // Seek backward until we find a newline
    while (1) {
        fseek(file, offset, SEEK_END);
        character = fgetc(file);
        if (character == '\n' || offset < -position) {
            break;
        }
        offset--;
    }
    
    // Now read the last line
    if (character == '\n') {
        fgets(buffer, BUFFER_SIZE, file);
        printf("Last line: %s", buffer);
    }
    
    fclose(file);
}

/**
 * Demonstrate error handling with file operations
 */
void demonstrate_error_handling() {
    printf("\n=== Error Handling ===\n");
    
    // Try to open a non-existent file
    FILE* file = fopen("non_existent_file.txt", "r");
    
    if (file == NULL) {
        printf("Expected error opening non-existent file:\n");
        printf("  errno: %d\n", errno);
        printf("  error message: %s\n", strerror(errno));
        perror("  perror message");
    } else {
        printf("Unexpected success opening non-existent file!\n");
        fclose(file);
    }
    
    // Try to write to a read-only file
    // First create and close the file
    file = fopen(TEST_FILENAME, "w");
    if (file != NULL) {
        fclose(file);
        
        // Now try to open it in an invalid mode
        file = fopen(TEST_FILENAME, "x");  // Invalid mode
        
        if (file == NULL) {
            printf("\nExpected error opening with invalid mode:\n");
            printf("  errno: %d\n", errno);
            printf("  error message: %s\n", strerror(errno));
            perror("  perror message");
        } else {
            printf("Unexpected success with invalid mode!\n");
            fclose(file);
        }
    }
    
    // Demonstrate clearerr and ferror
    file = fopen(TEST_FILENAME, "r");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Try to write to a read-only file
    printf("\nTrying to write to a read-only file:\n");
    int result = fputs("This should fail", file);
    
    if (result == EOF) {
        printf("  Write failed as expected\n");
        if (ferror(file)) {
            printf("  ferror indicates an error occurred\n");
            clearerr(file);
            printf("  Error cleared with clearerr\n");
            if (!ferror(file)) {
                printf("  ferror now indicates no error\n");
            }
        }
    } else {
        printf("  Unexpected write success!\n");
    }
    
    fclose(file);
}

/**
 * Demonstrate temporary files
 */
void demonstrate_temporary_files() {
    printf("\n=== Temporary Files ===\n");
    
    // Method 1: Using tmpfile()
    FILE* temp_file = tmpfile();
    
    if (temp_file == NULL) {
        printf("Error creating temporary file: %s\n", strerror(errno));
        return;
    }
    
    printf("Temporary file created successfully\n");
    
    // Write to the temporary file
    fprintf(temp_file, "This is data in a temporary file.\n");
    fprintf(temp_file, "It will be automatically deleted when closed.\n");
    
    // Rewind to the beginning of the file
    rewind(temp_file);
    
    // Read from the temporary file
    char buffer[BUFFER_SIZE];
    printf("Reading from temporary file:\n");
    while (fgets(buffer, BUFFER_SIZE, temp_file) != NULL) {
        printf("%s", buffer);
    }
    
    // Close the temporary file (automatically deletes it)
    fclose(temp_file);
    
    // Method 2: Using tmpnam() (less secure, but illustrative)
    char temp_name[L_tmpnam];
    
    // Get a temporary file name
    if (tmpnam(temp_name) == NULL) {
        printf("Error generating temporary filename\n");
        return;
    }
    
    printf("\nGenerated temporary filename: %s\n", temp_name);
    printf("Note: tmpnam() is less secure than tmpfile() and should be avoided in production code\n");
    
    // We won't actually create this file, as tmpnam is deprecated in favor of mkstemp in modern code
}

/**
 * Demonstrate file buffering control
 */
void demonstrate_file_buffering() {
    printf("\n=== File Buffering ===\n");
    
    // Open a file
    FILE* file = fopen(TEST_FILENAME, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    // Get the current buffer size
    printf("Default buffering for this file:\n");
    
    // Set different buffering modes
    
    // 1. No buffering
    if (setvbuf(file, NULL, _IONBF, 0) != 0) {
        printf("Error setting unbuffered mode\n");
    } else {
        printf("Set to unbuffered mode\n");
        fprintf(file, "This line is written with no buffering.\n");
    }
    
    // 2. Line buffering
    if (setvbuf(file, NULL, _IOLBF, BUFSIZ) != 0) {
        printf("Error setting line buffered mode\n");
    } else {
        printf("Set to line buffered mode\n");
        fprintf(file, "This line is written with line buffering.\n");
    }
    
    // 3. Full buffering
    if (setvbuf(file, NULL, _IOFBF, BUFSIZ) != 0) {
        printf("Error setting fully buffered mode\n");
    } else {
        printf("Set to fully buffered mode\n");
        fprintf(file, "This line is written with full buffering.\n");
    }
    
    // Force a buffer flush
    printf("Manually flushing the buffer...\n");
    fflush(file);
    
    // Custom buffer
    char custom_buffer[BUFFER_SIZE];
    if (setvbuf(file, custom_buffer, _IOFBF, BUFFER_SIZE) != 0) {
        printf("Error setting custom buffer\n");
    } else {
        printf("Set custom buffer of size %d\n", BUFFER_SIZE);
        fprintf(file, "This line is written with a custom buffer.\n");
    }
    
    fclose(file);
}

/**
 * Clean up files created during the demonstration
 */
void cleanup_files() {
    printf("\n=== Cleaning Up ===\n");
    
    if (remove(TEST_FILENAME) == 0) {
        printf("Deleted %s\n", TEST_FILENAME);
    } else {
        printf("Error deleting %s: %s\n", TEST_FILENAME, strerror(errno));
    }
    
    if (remove(TEST_BINARY_FILENAME) == 0) {
        printf("Deleted %s\n", TEST_BINARY_FILENAME);
    } else {
        printf("Error deleting %s: %s\n", TEST_BINARY_FILENAME, strerror(errno));
    }
}

int main() {
    printf("C File I/O Demonstration\n");
    
    demonstrate_file_open_close();
    demonstrate_character_io();
    demonstrate_line_io();
    demonstrate_formatted_io();
    demonstrate_binary_io();
    demonstrate_file_positioning();
    demonstrate_error_handling();
    demonstrate_temporary_files();
    demonstrate_file_buffering();
    
    cleanup_files();
    
    printf("\nAll demonstrations completed\n");
    return 0;
}
