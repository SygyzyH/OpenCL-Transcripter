#include <stdio.h>
#include <stdlib.h>

#include "io/args.h"
#include "io/audio.h"
#include "math/math.h"
#include "math/ml.h"

int init();

int main(int argc, char *argv[]) {
    sets = hndl_set(argc, argv);
    if (chkset(sets, DB))
        // If this occurs, DB must already be set.
        printf("Settings: %d -> OK: %s, RT: %s, DB: %s, FB: %s, CS: %s\n",
               sets, chkset(sets, OK)? "True" : "False", chkset(sets, RT)? "True" : "False",
               "True", chkset(sets, FB)? "True" : "False", chkset(sets, CS)? "True" : "False");
    
    // If OK flag is clr, abort.
    if (!chkset(sets, OK)) return 1;
    
    // Init
    int e;
    if ((e = init())) return e;
    
    // Testing math lib
    Mat a, b, res;
    a.width = a.height = b.width = b.height = 3;
    double d[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    a.data = b.data = (double *) d;
    
    matadd(unpkmat(a), unpkmat(b), &res.data);
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            printf("%lf, ", res.data[j + i * 3]);
        puts("");
    }
    
    Layer *machine = (Layer *) malloc(sizeof(Layer));
    machine->inw = 3;
    machine->inh = 3;
    
    machine->params.width = 0;
    machine->params.height = 0;
    machine->params.data = NULL;
    
    machine->transform = relu;
    machine->prev = NULL;
    machine->next = NULL;
    
    Mat input, *output;
    input.width = 3;
    input.height = 3;
    double da[] = { 1, 2, -3, 1, -2, 3, -1, 2, 3 };
    input.data = da;
    
    e = forwardpass(*machine, input, &output);
    
    if (!e) {
        puts("MAIN_C: [TESTING]: Foraward pass success!");
        printf("MAIN_C [TESTING]: e: %d, w: %d, h: %d\n", e, output->width, output->height);
        
        for (int i = 0; i < output->height; i++) {
            for (int j = 0; j < output->width; j++) {
                printf("%lf, ", output->data[j + i * output->width]);
            }
            puts("");
        }
    } else puts("MAIN_C: [TESTING]: Forward pass FAILURE");
    
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

int init() {
    int e;
    
    e = auinit();
    if (e) return e;
    e = mainit();
    if (e) return e;
    
    return 0;
}