#include <stdio.h>
#include <stdlib.h>

#include "io/args.h"
#include "io/audio.h"
#include "math/math.h"

int main(int argc, char *argv[]) {
    sets = hndl_set(argc, argv);
    if (chkset(sets, DB))
        // If this occurs, DB must already be set.
        printf("Settings: %d -> OK: %s, RT: %s, DB: %s, FB: %s, CS: %s\n",
               sets, chkset(sets, OK)? "True" : "False", chkset(sets, RT)? "True" : "False",
               "True", chkset(sets, FB)? "True" : "False", chkset(sets, CS)? "True" : "False");
    
    // If OK flag is clr, abort.
    if (!chkset(sets, OK)) return 1;
    
    int e;
    /* Init */
    e = auinit();
    if (e) return e;
    e = mainit();
    if (e) return e;
    return 0;
    
    // Start audio
    e = austrt();
    if (e) return e;
    
    getchar();
    
    e = aucln();
    if (e) return e;
    e = macln();
    if (e) return e;
    
    if (chkset(sets, DB))
        puts("Gracefully resolved!");
    
    return 0;
}