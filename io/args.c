#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>

#include "../dep/getopt.h"
#include "../std.h"
#include "args.h"

#define HELP "TODO: write help"

int sets = 0;

// Gets arguments from CLI, setting up the settings variable according to args.
/*
argc - argument count
argv - pointers to strings (arguments)
returns 0 on success
*/
int hndl_set(int argc, char *argv[]) {
    int opt;
    char *out = NULL;
    
    // Flags
    int res = 1 << OK;
    
    while (optind < argc) {
        if ((opt = getopt(argc, argv, "hdfco:")) != -1) {
            switch (opt) {
                case 'h':
                // Help
                puts(HELP);
                clrset(res, OK);
                break;
                
                case 'd':
                // Debug
                setset(res, DB);
                break;
                
                case 'f':
                // Feedback
                setset(res, FB);
                break;
                
                case 'c':
                // Cascade
                setset(res, CS);
                break;
                
                case 'o':
                // Output
                out = optarg;
                setset(res, OUT_TXT);
                break;
                
                case '?':
                printfc(optopt == 'o', "Option %c requires an argument.\n", optopt);
                clrset(res, OK);
                break;
                
                default:
                exit(1);
            }
        }
    }
    
    putsc(chkset(res, OK), "Argument parse successful.");
    putsc(!chkset(res, OK), "Argument parse unsuccessful.");
    
    printfc(out != NULL && chkset(res, DB), "Outputting transcript to \"%s\"\n", out);
    
    return res;
}

// Reads file as text
/*
filepath - string for filepath
returns string of file contants
*/
char *ldfile(char const *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL)
        perror("File read failed: ");
    
    fseek(file, 0L, SEEK_END);
    int file_size = ftell(file);
    rewind(file);
    
    char *text = (char *) malloc(sizeof(char) * (file_size + 1));
    
    fread(text, sizeof(char), file_size, file);
    text[file_size] = '\0';
    
    fclose(file);
    
    return text;
}

// Reads binary file as double
/*
filepath - path to file
res - pointer to result to be saved (will be allocated)
len - pointer to length of the result
returns 0 on success
*/
int ldbind(char const *filepath, double **res, int *len) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        printf("Failed to read \"%s\": ", filepath);
        perror("");
        return 1;
    }
    
    fseek(file, 0L, SEEK_END);
    *len = ftell(file);
    if (*len % sizeof(double)) {
        printf("Misaligned binary at \"%s\".\n", filepath);
        return 1;
    }
    
    *len /= sizeof(double);
    
    rewind(file);
    
    *res = (double *) malloc(sizeof(double) * (*len));
    
    fread(*res, sizeof(double), *len, file);
    
    fclose(file);
    
    return 0;
}
// fwrite(fid, trainedNet.Layers(1, 1).AverageImage, 'double');