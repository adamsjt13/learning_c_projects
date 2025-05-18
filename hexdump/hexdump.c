#include <stdio.h>

int main(int argc, char *argv[])
{
    // open file
    FILE *fp;
    char buffer[16];
    size_t bytes_read;
    char *prog = argv[0];
    size_t offset = 0;

    fp = fopen(*++argv, "rb");

    if (argc == 1){
        printf("Please provide a file to read\n");
    }
    else {
        while (--argc > 0){
            if (fp == NULL){
                fprintf(
                    stderr,
                    "%s: can't open %s\n",
                    prog,
                    *argv
                );
                return 1;
            }
            else {
                while((bytes_read = fread(buffer, 1, 16, fp)) > 0){
                    printf("%08lx   ", offset);
                    for(int i = 0; i < bytes_read; i++){
                        if(i % 8 == 0){
                            printf(" ");
                        }
                        printf("%.2x ", buffer[i]);
                    }
                    if(bytes_read < 8){
                            printf(" ");
                        }
                    for(int i = 0; i < (16 - bytes_read); i++){
                        printf("   ");
                    }
                    printf("|");
                    for(int j = 0; j < bytes_read; j++){
                        printf("%c", buffer[j]);
                    }
                    printf("|\n");
                    offset += bytes_read;
                }
                fclose(fp);
            }
        }
    }

    if (ferror(stdout)){
        fprintf(
            stderr,
            "%s: error writing stdout\n",
            prog);
        return 2;
    }
    return 0;
}
