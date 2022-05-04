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
    
    mklayer(&machine, -1, -1, dropout, NULL, "./machine/dropout.bin");
    mklayer(&machine, -1, -1, fullyc, NULL, "./machine/fullyc.bin");
    mklayer(&machine, -1, -1, softmax, NULL, NULL);
    
    /* Testing */
    
    WAVC *wav;
    pwav("./testing/dataset/Hebrew Build Converted/lo/temp1p3.wav", &wav, 2);
    double *stftinp;
    wavtod(wav, &stftinp, 1);
    printf("%d Samples\n", wav->samples);
    for (int i = 0; i < wav->samples / 100; i++) printfu("%.4e, ", stftinp[i]); putsu();
    
    Mat *minp;
    int framesize = (int) wav->hdr.samplerate * 0.025;
    int hopsize = (int) wav->hdr.samplerate * 0.01;
    printf("samplerate: %d, framesize: %d, hopsize: %d\n", wav->hdr.samplerate, framesize, hopsize);
    
    // TODO: window normalization, fftlen
    stft(stftinp, wav->samples, framesize, framesize, 512, hopsize, TWO_SIDED, &minp);
    //melspec(stftinp, wav->samples, framesize, framesize, hopsize, 40, 50, 7000, &minp);
    //printf("stft height should be at %lf once fft length is implemented\n", 
    //stfth(wav->samples, 512, hopsize));
    //double stfttest[128]; for (int i = 0; i < 128; i++) stfttest[i] = 1;
    //double stfttest[6] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    //stft(stfttest, 6, 3, 3, 3, 3, TWO_SIDED, &minp);
    //printf("e: %d\n", melspec(stftinp, wav->samples, framesize, framesize, hopsize, 40, 50, 7000, &minp));
    
    printf("w: %d, h: %d\n", minp->width, minp->height);
    for (int i = 0; i < minp->height; i++) {
        for (int j = 0; j < minp->width; j++) {
            printfu("%.4e ", minp->data[j + i * minp->width]);
        } putsu();
    } putsu();
    
    // TODO: Each input's length may be smaller than one second, but the machine
    // can only accept a fixed size. for this reason, there needs to be padding on
    // width to ensure each segment is of the same length. To get the desired consistant
    // dimensions, Matlab sets width to be consistently
    // ceil((segmentDuration - frameDuration)/hopDuration)
    // which ends up being ceil((1 - 0.025) / 0.01) = 98.
    // I need to pad both sides as well (where the left side gets the reminder) to fit 98.
    
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