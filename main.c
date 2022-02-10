#include <stdio.h>
#include <stdlib.h>

#include "io/args.h"
#include "io/audio.h"

int main(int argc, char *argv[]) {
#ifndef __CUDACC__
    puts("No CUDA C support in build. Running un-accelerated.");
#endif
    
    sets = hndl_set(argc, argv);
    if (chkset(sets, DB))
        // If this occurs, DB must already be set.
        printf("Settings: %d -> OK: %s, RT: %s, DB: %s, FB: %s, CS: %s\n",
               sets, chkset(sets, OK)? "True" : "False", chkset(sets, RT)? "True" : "False",
               "True", chkset(sets, FB)? "True" : "False", chkset(sets, CS)? "True" : "False");
    
    // If OK flag is clr, abort.
    if (!chkset(sets, OK)) return 1;
    
    if (auinit()) return 2;
    if (austrt()) return 3;
    getchar();
    if (aucln()) return 4;
    
    if (chkset(sets, DB))
        puts("Gracefully resolved!");
    
    return 0;
}