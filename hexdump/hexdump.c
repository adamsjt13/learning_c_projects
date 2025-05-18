#include <stdio.h>

int main(int argc, char *argv[])
{
    // open file
    FILE *fp;
    void fileprint(FILE *);
    char *prog = argv[0];

    if (argc == 1){
        fileprint(stdin);
    }
    else {
        while (--argc > 0){
            if ((fp = fopen(*++argv, "rb")) == NULL){
                fprintf(
                    stderr,
                    "%s: can't open %s\n",
                    prog,
                    *argv
                );
                return 1;
            }
            else {
                fileprint(fp);
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

void fileprint(FILE *ifp){
    int c;

    while((c = getc(ifp)) != EOF){
        putchar(c);
    }

    printf("\n");
}
