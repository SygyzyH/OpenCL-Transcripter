#include <stdio.h>
#include <stdlib.h>

#include "io/args.h"
#include "io/audio.h"
#include "io/oclapi.h"
#include "math/math.h"
#include "math/ml.h"
#include "std.h"

int init();
int cln();

int main(int argc, char *argv[]) {
    int e;
    
    /* Get running parameters from CLI */
    sets = hndl_set(argc, argv);
    if (chkset(sets, DB))
        // If this occurs, DB must already be set.
        printf("Settings: %d -> OK: %s, RT: %s, DB: %s, FB: %s, CS: %s\n",
               sets, chkset(sets, OK)? "True" : "False", chkset(sets, RT)? "True" : "False",
               "True", chkset(sets, FB)? "True" : "False", chkset(sets, CS)? "True" : "False");
    
    // If OK flag is clr, abort.
    criticly_safe(!chkset(sets, OK), "Invalid program arguments."); 
    
    /* Init */
    criticly_safe(init(), "Initialiazation failure.");
    
    /* Make machine */
    // TODO: Automate this, maybe by reading from a file or making a clearer interface
    
    /*
Imageinp

x4:
conv
batch
relu
maxpool

drpout
fullycon
softmax

*/
    
    Layer *machine = (Layer *) malloc(sizeof(Layer));
    machine->inw = 40;
    machine->inh = 98;
    machine->transform = zcenter;
    machine->next = NULL;
    
    Mat mAvgImg;
    mAvgImg.height = 1;
    safe(ldbind("./machine/avgimg.bin", &mAvgImg.data, &mAvgImg.width));
    
    machine->params = mAvgImg;
    
    // First rep
    Layer *newl = (Layer *) malloc(sizeof(Layer));
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = conv2d;
    newl->next = NULL;
    
    Mat newlParam1;
    newlParam1.height = 1;
    safe(ldbind("./machine/conv1.bin", &newlParam1.data, &newlParam1.width));
    newl->params = newlParam1;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = bnorm;
    newl->next = NULL;
    
    Mat newlParam2;
    newlParam2.height = 1;
    safe(ldbind("./machine/bnorm1.bin", &newlParam2.data, &newlParam2.width));
    newl->params = newlParam2;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = relu;
    newl->next = NULL;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = maxpool;
    newl->next = NULL;
    
    Mat newlParam4;
    newlParam4.height = 1;
    safe(ldbind("./machine/maxpool1.bin", &newlParam4.data, &newlParam4.width));
    newl->params = newlParam2;
    
    addlayer(&machine, &newl);
    
    // Second rep
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = conv2d;
    newl->next = NULL;
    
    Mat newlParam5;
    newlParam5.height = 1;
    safe(ldbind("./machine/conv2.bin", &newlParam5.data, &newlParam5.width));
    newl->params = newlParam5;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = bnorm;
    newl->next = NULL;
    
    Mat newlParam6;
    newlParam6.height = 1;
    safe(ldbind("./machine/bnorm2.bin", &newlParam6.data, &newlParam6.width));
    newl->params = newlParam6;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = relu;
    newl->next = NULL;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = maxpool;
    newl->next = NULL;
    
    Mat newlParam7;
    newlParam7.height = 1;
    safe(ldbind("./machine/maxpool2.bin", &newlParam7.data, &newlParam7.width));
    newl->params = newlParam7;
    
    addlayer(&machine, &newl);
    
    // Third rep
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = conv2d;
    newl->next = NULL;
    
    Mat newlParam8;
    newlParam8.height = 1;
    safe(ldbind("./machine/conv3.bin", &newlParam8.data, &newlParam8.width));
    newl->params = newlParam8;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = bnorm;
    newl->next = NULL;
    
    Mat newlParam9;
    newlParam9.height = 1;
    safe(ldbind("./machine/bnorm3.bin", &newlParam9.data, &newlParam9.width));
    newl->params = newlParam9;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = relu;
    newl->next = NULL;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = maxpool;
    newl->next = NULL;
    
    Mat newlParam10;
    newlParam10.height = 1;
    safe(ldbind("./machine/maxpool3.bin", &newlParam10.data, &newlParam10.width));
    newl->params = newlParam10;
    
    addlayer(&machine, &newl);
    
    // Fourth rep
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = conv2d;
    newl->next = NULL;
    
    Mat newlParam11;
    newlParam11.height = 1;
    safe(ldbind("./machine/conv4.bin", &newlParam11.data, &newlParam11.width));
    newl->params = newlParam11;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = bnorm;
    newl->next = NULL;
    
    Mat newlParam12;
    newlParam12.height = 1;
    safe(ldbind("./machine/bnorm4.bin", &newlParam12.data, &newlParam12.width));
    newl->params = newlParam12;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = relu;
    newl->next = NULL;
    
    addlayer(&machine, &newl);
    
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = maxpool;
    newl->next = NULL;
    
    Mat newlParam13;
    newlParam13.height = 1;
    safe(ldbind("./machine/maxpool4.bin", &newlParam13.data, &newlParam13.width));
    newl->params = newlParam13;
    
    addlayer(&machine, &newl);
    
    // dropout
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = dropout;
    newl->next = NULL;
    
    Mat newlParam14;
    newlParam14.height = 1;
    safe(ldbind("./machine/dropout.bin", &newlParam14.data, &newlParam14.width));
    
    newl->params = newlParam14;
    
    addlayer(&machine, &newl);
    
    // fullycon
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = fullyc;
    newl->next = NULL;
    
    Mat newlParam15;
    newlParam15.height = 1;
    safe(ldbind("./machine/fullycon.bin", &newlParam15.data, &newlParam15.width));
    
    newl->params = newlParam15;
    
    addlayer(&machine, &newl);
    
    // softmax
    newl = (Layer *) malloc(sizeof(Layer));;
    newl->inw = -1;
    newl->inh = -1;
    newl->transform = softmax;
    newl->next = NULL;
    
    addlayer(&machine, &newl);
    
    /* Main loop */
    
    // Start audio
    safe(austrt());
    
    getchar();
    
    /* Cleanup */
    criticly_safe(cln(), "Cleanup failure.");
    
    putsc(chkset(sets, DB), "Gracefully resolved!");
    
    return EXIT_SUCCESS;
}

int init() {
    safe(auinit());
    safe(ocinit());
    safe(mainit());
    
    return 0;
}

int cln() {
    safe(aucln());
    safe(occln());
    
    return 0;
}