#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#include "io/args.h"
#include "io/audio.h"
#include "io/oclapi.h"
#include "math/math.h"
#include "math/ml.h"
#include "std.h"

#define MACHINEOUTPUTS 12
#define RUNTIME_SECONDS 20

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
    
    /* Benchmark system */
    // Benchmark system to figure out a good classification rate value.
    // Calssification rate is the target rate at which the input will be sampled 
    // (and calssified) per second.
    
    WAVC *wav;
    ldwav("Benchmark.wav", &wav, 2);
    
    // Start timing
    clock_t t = clock();
    
    double *stftinp;
    wavtod(wav, &stftinp, 1);
    
    Mat *minp;
    int framesize = (int) wav->hdr.samplerate * frameduration;
    int hopsize = (int) wav->hdr.samplerate * hopduration;
    if (chkset(sets, DB))
        printf("Standart samplerate: %d, framesize: %d, hopsize: %d\n",
               wav->hdr.samplerate, framesize, hopsize);
    
    melspec(stftinp, wav->samples, wav->hdr.samplerate, framesize, framesize, 512, hopsize, 40, 50, 7000, &minp);
    
    // Each input's length may be smaller than one second, but the machine
    // can only accept a fixed size. There needs to be padding on
    // width to ensure each segment is of the same length.
    Mat *pminp;
    ensuredims(*minp, numhops, minp->height, &pminp);
    free(minp->data);
    free(minp);
    
    // Convert to log-mel spectrogram
    log10spec(&(pminp->data), pminp->width * pminp->height);
    
    Mat *sclass;
    e = forwardpass(*machine, *pminp, &sclass);
    free(pminp);
    criticly_safe(e, "Benchmark failed.");
    
    // End timing
    t = clock() - t;
    double time = ((double) t) / CLOCKS_PER_SEC;
    int classrate = (int) (0.8 * 1 / time);
    FRAMESPERSECOND = classrate;
    if (chkset(sets, DB))
        printf("Benchmark finished, round-about time: %lf seconds, allowing for classification rate of ~%d.\n",
               time, classrate);
    
    if (chkset(sets, DB)) {
        double max = sclass->data[0];
        int maxi = 0;
        
        puts("Benchmark results:");
        for (int i = 0; i < sclass->height; i++) {
            for (int j = 0; j < sclass->width; j++) {
                if (sclass->data[j + i * sclass->width] > max) {
                    max = sclass->data[j + i * sclass->width];
                    maxi = j + i * sclass->width;
                }
                
                printfu("%lf, ", sclass->data[j + i * sclass->width]);
            } putsu();
        }
        
        printf("Chosen linear index #%d with confidance %.2lf%%. (maps to \"%s\")\n",
               maxi, max * 100, "TODO: map index to string");
    } free(sclass);
    
    /* Main loop */
    
    // Start audio
    safe(auinit());
    safe(austrt());
    
    WAVC *audioIn = NULL;
    
    char *classname;
    
    // Make a buffer that can hold up to one second of audio data
    // The buffer will be rolled to contain the last one second of recordings
    double *audiobuf = (double *) calloc(SAMPLERATE, sizeof(double));
    int *ibuf = (int *) malloc(classrate / 2 * sizeof(int));
    for (int i = 0; i < classrate / 2; i++) ibuf[i] = MACHINEOUTPUTS - 1;
    double *probbuf = (double *) calloc((size_t) MACHINEOUTPUTS * classrate / 2, sizeof(double));
    
    // Start timing
    t = clock();
    double seconds = RUNTIME_SECONDS;
    while (((double) (clock() - t)) / CLOCKS_PER_SEC < seconds) {
        WAVEHDR whdr = augetb();
        audioIn = frmtowav(formatex, (unsigned char *) whdr.lpData, (unsigned int) whdr.dwBufferLength);
        
        double *audioInnorm;
        wavtod(audioIn, &audioInnorm, 1);
        
        // Move the audio buffer back and append the audio data 
        for (int i = 0; i < SAMPLERATE; i++) {
            if (i < SAMPLERATE - audioIn->samples)
                audiobuf[i] = audiobuf[i + audioIn->samples];
            else
                audiobuf[i] = audioInnorm[i - (SAMPLERATE - audioIn->samples)];
        }
        
        Mat *machineInput = NULL;
        framesize = (int) wav->hdr.samplerate * frameduration;
        hopsize = (int) wav->hdr.samplerate * hopduration;
        safe(melspec(audiobuf, SAMPLERATE, audioIn->hdr.samplerate, framesize, framesize, 512, hopsize, 40, 50, 7000, &machineInput), "Mel-spectrogram failed");
        
        Mat *pmachineInput;
        ensuredims(*machineInput, numhops, machineInput->height, &pmachineInput);
        
        log10spec(&(pmachineInput->data), pmachineInput->width * pmachineInput->height);
        
        Mat *machineOutput;
        safe(forwardpass(*machine, *pmachineInput, &machineOutput), "Forward pass failed");
        
        double max = machineOutput->data[0];
        int maxi = 0;
        
        // Move the buffers back
        for (int i = 0; i < (classrate / 2) - 1; i++) 
            ibuf[i] = ibuf[i + 1];
        for (int i = 0; i < (int) (classrate / 2) - 1; i++) {
            for (int j = 0; j < MACHINEOUTPUTS; j++) {
                probbuf[j + i * MACHINEOUTPUTS] = probbuf[j + (i + 1) * MACHINEOUTPUTS];
            }
        }
        
        for (int i = 0; i < machineOutput->width; i++) {
            if (machineOutput->data[i] > max) {
                max = machineOutput->data[i];
                maxi = i;
            }
            
            probbuf[i + ((classrate / 2) - 1) * MACHINEOUTPUTS] = machineOutput->data[i];
        }
        
        ibuf[((int) classrate / 2) - 1] = maxi;
        
        // display the corret label
        // How many times did every label appear in label buffer
        int ibufocc[MACHINEOUTPUTS];
        for (int i = 0; i < MACHINEOUTPUTS; i++) ibufocc[i] = 0;
        // Neat O(n) algorithm to do mode()
        for (int i = 0; i < (int) classrate / 2; i++) ibufocc[ibuf[i]]++;
        // What is the label that appeared the most
        int maxibuf = MACHINEOUTPUTS - 1;
        for (int i = 0; i < MACHINEOUTPUTS; i++) if (ibufocc[i] > ibufocc[maxibuf]) maxibuf = i;
        // What is that labels highest probability recently
        double maxprob = 0;
        for (int i = 0; i < (int) classrate / 2; i++) maxprob = MAX(maxprob, probbuf[maxibuf + i * MACHINEOUTPUTS]);
        if (ibufocc[maxibuf] > ceil(classrate * 0.2) && maxprob > 0.7) {
            switch (maxibuf) {
                case 0: classname = "at"; break;
                case 1: classname = "ata"; break;
                case 2: classname = "emcha"; break;
                case 3: classname = "ken"; break;
                case 4: classname = "lechoo"; break;
                case 5: classname = "lo"; break;
                case 6: classname = "piteaach"; break;
                case 7: classname = "radood"; break;
                case 8: classname = "riteaach"; break;
                case 9: classname = "shafach"; break;
                case 10: classname = "unknown"; break;
                default: classname = ""; break;
            }
        } else classname = "";
        
        // Print to CLI
        double *raw;
        wavtod(audioIn, &raw, 0);
        double loudness = computevu(raw, audioIn->samples);
        
        char block[CLICAP + 1];
        for (int i = 0; i < CLICAP; i++) {
            if (i < loudness) 
                block[i] = CLICHR;
            else
                block[i] = ' ';
        }
        block[CLICAP] = '\0';
        printfu("%c[%s> Prediction: [ %s ]         ", chkset(sets, CS)? '\n' : '\r', block, classname);
        
        free(machineOutput->data);
        free(machineOutput);
        free(pmachineInput->data);
        free(pmachineInput);
        free(machineInput->data);
        free(machineInput);
        // DON'T free audioIn->data, since its used by audio.c
        free(audioIn);
    }
    free(audiobuf);
    free(ibuf);
    free(probbuf);
    
    putsu();
    
    /* Cleanup */
    criticly_safe(cln(), "Cleanup failure.");
    
    /*puts("Done\naudiobuf:");
    for (int i = 0; i < SAMPLERATE; i++) printfu("%lf ", audiobuf[i]);
    puts("Done");*/
    
    putsc(chkset(sets, DB), "Gracefully resolved!");
    
    return EXIT_SUCCESS;
}

int init() {
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