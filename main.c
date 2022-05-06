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
    machine->inw = 98;
    machine->inh = 40;
    machine->transform = zcenter;
    machine->next = NULL;
    
    Mat mAvgImg;
    mAvgImg.height = 1;
    criticly_safe(ldbind("./machine/avgimg.bin", &mAvgImg.data, &mAvgImg.width), "Failed to initialize machine.");
    
    machine->params = mAvgImg;
    
    mklayer(&machine, -1, -1, conv2d, NULL, "./machine/conv1.bin");
    mklayer(&machine, -1, -1, bnorm, NULL, "./machine/bnorm1.bin");
    mklayer(&machine, -1, -1, relu, NULL, NULL);
    mklayer(&machine, -1, -1, maxpool, NULL, "./machine/maxpool1.bin");
    
    mklayer(&machine, -1, -1, conv2d, NULL, "./machine/conv2.bin");
    mklayer(&machine, -1, -1, bnorm, NULL, "./machine/bnorm2.bin");
    mklayer(&machine, -1, -1, relu, NULL, NULL);
    mklayer(&machine, -1, -1, maxpool, NULL, "./machine/maxpool2.bin");
    
    mklayer(&machine, -1, -1, conv2d, NULL, "./machine/conv3.bin");
    mklayer(&machine, -1, -1, bnorm, NULL, "./machine/bnorm3.bin");
    mklayer(&machine, -1, -1, relu, NULL, NULL);
    mklayer(&machine, -1, -1, maxpool, NULL, "./machine/maxpool3.bin");
    
    mklayer(&machine, -1, -1, conv2d, NULL, "./machine/conv4.bin");
    mklayer(&machine, -1, -1, bnorm, NULL, "./machine/bnorm4.bin");
    mklayer(&machine, -1, -1, relu, NULL, NULL);
    
    mklayer(&machine, -1, -1, conv2d, NULL, "./machine/conv5.bin");
    mklayer(&machine, -1, -1, bnorm, NULL, "./machine/bnorm5.bin");
    mklayer(&machine, -1, -1, relu, NULL, NULL);
    mklayer(&machine, -1, -1, maxpool, NULL, "./machine/maxpool4.bin");
    
    // Only used during training
    //mklayer(&machine, -1, -1, dropout, NULL, "./machine/dropout.bin");
    mklayer(&machine, -1, -1, fullyc, NULL, "./machine/fullyc.bin");
    mklayer(&machine, -1, -1, softmax, NULL, NULL);
    
    /* Designed parameters */
    
    double frameduration = 0.025;
    double hopduration = 0.01;
    double dfileduaration = 1;
    int numhops = ceil((dfileduaration - frameduration) / hopduration);
    
    /* Testing */
    
    WAVC *wav;
    pwav("./testing/dataset/Hebrew Build Converted/lo/temp1p3.wav", &wav, 2);
    double *stftinp;
    wavtod(wav, &stftinp, 1);
    printf("%d Samples\n", wav->samples);
    for (int i = 0; i < wav->samples / 100; i++) printfu("%.4e, ", stftinp[i]); putsu();
    
    Mat *minp;
    int framesize = (int) wav->hdr.samplerate * frameduration;
    int hopsize = (int) wav->hdr.samplerate * hopduration;
    printf("samplerate: %d, framesize: %d, hopsize: %d\n", wav->hdr.samplerate, framesize, hopsize);
    
    melspec(stftinp, wav->samples, wav->hdr.samplerate, framesize, framesize, 512, hopsize, 40, 50, 7000, &minp);
    
    // Each input's length may be smaller than one second, but the machine
    // can only accept a fixed size. for this reason, there needs to be padding on
    // width to ensure each segment is of the same length.
    Mat *res;
    ensuredims(*minp, numhops, minp->height, &res);
    
    Mat *sclass;
    e = forwardpass(*machine, *res, &sclass);
    printf("e: %d\n", e);
    
    if (!e) {
        printf("Forward pass succesful.\nMachine result: %d x %d\n", 
               sclass->width, sclass->height);
        for (int i = 0; i < sclass->height; i++) {
            for (int j = 0; j < sclass->width; j++) {
                printfu("%lf, ", sclass->data[j + i * sclass->width]);
            } putsu();
        } putsu();
    }
    
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
    safe(mlinit());
    
    return 0;
}

int cln() {
    safe(aucln());
    safe(occln());
    
    return 0;
}