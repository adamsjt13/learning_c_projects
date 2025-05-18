#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Structure to hold display options (useful for future command-line arguments)
typedef struct {
    int bytes_per_line;    // Default 16, but could be configurable
    int group_size;        // Default 8, for spacing between groups
    int show_ascii;        // Whether to show ASCII representation
} DisplayOptions;

// Function prototypes
void print_usage(const char *program_name);
void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset, const DisplayOptions *opts);
void print_hex_values(const unsigned char *buffer, size_t bytes_read, const DisplayOptions *opts);
void print_ascii_representation(const unsigned char *buffer, size_t bytes_read);
int process_file(const char *filename, const DisplayOptions *opts);

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <file>\n", program_name);
}

void print_hex_values(const unsigned char *buffer, size_t bytes_read, const DisplayOptions *opts) {
    // Print hex values with proper spacing
    for (size_t i = 0; i < opts->bytes_per_line; i++) {
        if (i == opts->group_size) {
            printf(" ");
        }
        if (i < bytes_read) {
            printf("%02x ", buffer[i]);
        } else {
            printf("   ");
        }
    }
}

void print_ascii_representation(const unsigned char *buffer, size_t bytes_read) {
    printf("|");
    for (size_t i = 0; i < bytes_read; i++) {
        printf("%c", isprint(buffer[i]) ? buffer[i] : '.');
    }
    printf("|");
}

void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset, const DisplayOptions *opts) {
    // Print offset
    printf("%08lx  ", offset);

    // Print hex values
    print_hex_values(buffer, bytes_read, opts);

    // Print ASCII representation if enabled
    if (opts->show_ascii) {
        printf(" ");
        print_ascii_representation(buffer, bytes_read);
    }
    
    printf("\n");
}

int process_file(const char *filename, const DisplayOptions *opts) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        return 1;
    }

    unsigned char *buffer = malloc(opts->bytes_per_line);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    size_t bytes_read;
    size_t offset = 0;

    while ((bytes_read = fread(buffer, 1, opts->bytes_per_line, fp)) > 0) {
        print_hex_line(buffer, bytes_read, offset, opts);
        offset += bytes_read;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Error: Failed to read file %s\n", filename);
        free(buffer);
        fclose(fp);
        return 1;
    }

    free(buffer);
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Initialize default options (can be modified later for command-line arguments)
    DisplayOptions opts = {
        .bytes_per_line = 16,
        .group_size = 8,
        .show_ascii = 1
    };

    return process_file(argv[1], &opts);
} 