/** \file jsondump.c */

/** This program shows how to use jstream: after compiling
    it by `clang jsondump.c jstream.c -o jsondump`, use it as
    
        jsondump file1 ... filen
    
    to read, store as objects and dump on the terminal again
    in compressed Json format the contents of file1 ... filen. */

#include <stdio.h>
#include "jstream.h"

static FILE *f = NULL;

static int get(void)
{
    return fgetc(f);
}

int main(int n, char **a)
{
    for (int i = 1; i < n; ++ i ) {
        if ((f = fopen(a[i], "r")) == NULL) {
            perror(a[i]);
            continue;
        }
        printf("\nProcessing file %s:\n", a[i]);
        struct jstream_param_s param;
        param.get = get;
        jstream_t obj = jstream(&param);
        fclose(f);
        if (param.error) {
            printf("Error #%i (last char = '%c').\n", param.error,
                param.clast);
        } else {
            if (0) {
                puts("Binary dump:");
                for (int i = 0; i < param.size; ++ i)
                    printf("%16p: %08x\n", obj + i, obj[i]);
                putchar('\n');
            }
            jstream_dump(stdout, obj);
            putchar('\n');
        }
    }
    return 0;
}
