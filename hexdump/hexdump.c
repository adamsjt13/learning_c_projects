#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

// structure to hold display options, command line args
typedef struct {
    int bytes_per_line; // default 16, controls number of bytes printed per line
    int group_size; // default 8, controls spacing between groups
    int show_ascii; // show ASCII at the end
} DisplayOptions;

void print_usage(const char *program_name);
void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset, const DisplayOptions *opts);
void print_hex_values(const unsigned char *buffer, size_t bytes_read, const DisplayOptions *opts);
void print_ascii_representation(const unsigned char *buffer, size_t bytes_read);
int process_file(const char *filename, const DisplayOptions *opts);

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <file>\n", program_name);
}

void print_hex_values(const unsigned char *buffer, size_t bytes_read, const DisplayOptions *opts) {
    for (size_t i = 0; i < opts->bytes_per_line; i++) {
        if (i < bytes_read){
            printf("%02x ", buffer[i]);
        } else {
            printf("   ");
        }
        // add space between groups
        if (i % opts->group_size){
            printf(" ");
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

void print_hex_line(const unsigned char *buffer, size_t bytes_read, size_t offset, const DisplayOptions *opts) {
    printf("%08lx  ", offset);

    print_hex_values(buffer, bytes_read, opts);

    if (opts->show_ascii){
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

    unsigned char *buffer = malloc(opts->bytes_per_line * sizeof(unsigned char));
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    size_t bytes_read;
    size_t offset = 0;

    while((bytes_read = fread(buffer, 1, opts->bytes_per_line, fp)) > 0) {
        print_hex_line(buffer, bytes_read, offset, opts);
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

    DisplayOptions opts = {
        .bytes_per_line = 8,
        .group_size = 2,
        .show_ascii = 0
    };

    return process_file(argv[1], &opts);
}
