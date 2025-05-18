#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define BYTES_PER_LINE 16

void print_usage(const char *program_name);
void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset);
void print_hex_values(const unsigned char *buffer, size_t bytes_read);
void print_ascii_representation(const unsigned char *buffer, size_t bytes_read);
int process_file(const char *filename);

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <file>\n", program_name);
}

void print_hex_values(const unsigned char *buffer, size_t bytes_read) {
    for (size_t i = 0; i < BYTES_PER_LINE; i++) {
        if (i < bytes_read){
            printf("%02x ", buffer[i]);
        } else {
            printf("   ");
        }
    }
}

void print_ascii_representation(const unsigned char *buffer, size_t bytes_read) {
    printf("|");
    for(size_t i = 0; i < bytes_read; i++){
        printf("%c", isprint(buffer[i]) ? buffer[i] : '.');
    }
    printf("|");
} 

void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset) {
    printf("%08lx   ", offset);

    print_hex_values(buffer, bytes_read);

    print_ascii_representation(buffer, bytes_read);

    printf("\n");
}

int process_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        return 1;
    }

    unsigned char *buffer = malloc(BYTES_PER_LINE * sizeof(unsigned char));
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    size_t bytes_read;
    size_t offset = 0;

    while((bytes_read = fread(buffer, 1, BYTES_PER_LINE, fp)) > 0) {
        print_hex_line(buffer, bytes_read, offset);
        offset += bytes_read;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Error: Failed to read file %s\n", filename);
        free(buffer);
        buffer = NULL;
        fclose(fp);
        return 1;
    }

    free(buffer);
    buffer = NULL;
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    return process_file(argv[1]);
}
