#include <stdio.h>
#include <stdlib.h>

#include "io/args.h"
#include "io/audio.h"
#include "io/oclapi.h"
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
    
    // Testing OpenCL API
    const char *kernel_code = "__kernel void test(int input, int *output) { *output = input; }";
    register_from_src(&kernel_code, 1, "test");
    int *oclres;
    size_t gsz[3] = { 1, 0, 0};
    run_kernel("test", gsz, 69, oclres, 1, OCLREAD | OCLWRITE);
    
    // Testing ml lib
    
    Layer *machine = (Layer *) malloc(sizeof(Layer));
    machine->inw = 4;
    machine->inh = 4;
    
    machine->params.width = 7 + 16 + 2;
    machine->params.height = 1;
    double da[] = { 1, 1, 2, 2, 2, SAME, 2,
        // Weights
        // c = 1      // c = 2
        1, 1, 0, 0,   1, 1, 0, 0, // filter = 1
        
        1, 1, 0, 0,   0, 0, 0, 0, // filter = 2
        // Bias
        0, 1 };
    machine->params.data = da;
    
    machine->transform = conv2d;
    machine->prev = NULL;
    machine->next = NULL;
    
    Mat input, *output;
    input.width = 4;
    input.height = 4;
    double dat[] = { 12, 20, 30, 0, 8, 12, 2, 0, 34, 70, 37, 4, 112, 100, 25, 12 };
    input.data = dat;
    
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
    } else printf("MAIN_C: [TESTING]: Forward pass FAILURE: %d\n", e);
    
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
    e = ocinit();
    if (e) return e;
    
    return 0;
}