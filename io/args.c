#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>

#include "../dep/getopt.h"
#include "args.h"

#define HELP "TODO: write help"

int sets = 0;

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
                if (optopt == 'o')
                    printf("%s: Option %c requires an argument.\n", __FILE__, optopt);
                clrset(res, OK);
                break;
                
                default:
                exit(1);
            }
        }
    }
    
    if (chkset(res, DB))
        printf("%s: Argument parse %s\n", __FILE__, chkset(res, OK)? "successful" : "UNSUCCESSFUL");
    if (out != NULL && chkset(res, DB))
        printf("%s: Outputting transcript to \"%s\"\n", __FILE__, out);
    
    return res;
}

char* ldfile(char const *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL)
        perror("File read failed: ");
    
    int file_size;
    char *text;
    
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    text = (char *) malloc(sizeof(char) * (file_size + 1));
    fread(text, sizeof(char), file_size, file);
    text[file_size] = '\0';
    fclose(file);
    return text;
}