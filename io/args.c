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
                    printf("Option %c requires an argument.\n", optopt);
                clrset(res, OK);
                break;
                
                default:
                exit(1);
            }
        }
    }
    
    if (chkset(res, DB))
        printf("ARGS_H: Argument parse %s\n", chkset(res, OK)? "successful" : "UNSUCCESSFUL");
    if (out != NULL && chkset(res, DB))
        printf("Outputting transcript to \"%s\"\n", out);
    
    return res;
}