#include "audio.h"
#include "pwave.h"
#include "args.h"
#include "../math/math.h"
#include "../std.h"

HWAVEIN hWaveIn;
HWAVEOUT hWaveOut;
WAVEHDR header [NUM_BUF];
WAVEFORMATEX formatex;

int aucurbuf = 0;
int FRAMESPERSECOND = 20;
char *classname = "";

char *pbuf;
int dlock = 1;
int isauinit = 0;

void CALLBACK whndl(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void prntbr();

// Prints the bar at the bottom of the CLI
void prntbr() {
    // Short int min limit
    short int max = -32768;
    for (int i = 0; i < header[aucurbuf].dwBufferLength / BITRATE / 8; i++) {
        short int cur = ((short int *)(header[aucurbuf].lpData))[i];
        if (cur > max)
            max = cur;
    }
    
    double *dmaxa = (double *) malloc(sizeof(double) * 2);
    dmaxa[0] = (double) CLICAP; dmaxa[1] = (double) max;
    int e = amptodb(&dmaxa, 2, dmaxa[0], SCALE_FIRST);
    
    double dmax = dmaxa[1];
    free(dmaxa);
    
    // Prepare string
    char block[CLICAP + 1];
    for (int i = 0; i < CLICAP; i++) {
        if (i < (int) dmax)
            block[i] = CLICHR;
        else
            block[i] = ' ';
    }
    block[CLICAP] = '\0';
    
    printfu("%c[%s> Prediction: [ %s ]    ", chkset(sets, CS) ? '\n' : '\r', block, classname);
}

// Callback function to handle emptying the buffer when its filled
/*
Header description in Microsoft docs. loosly speaking,
hwi - handle of the audio device
uMsg - type of the call, usually WIM_INIT or WIM_DATA or WIM_CLEAN
dwInstance - unused
dwParam1 - unused
dwParam2 - unused
*/
void CALLBACK whndl(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
    /*
TODO: I was right all along. Like mentioned in the MSDN Docs,
running another wave function in the CALLBACK handler causes
dead lock. However, it happens even before it manages to
catch the WIM_CLOSE message.

What needs to be done is re-slotting the next buffer outside
the CALLBACK function, say via a thread.
https://stackoverflow.com/questions/11935486/waveinclose-reset-stop-no-msg

A bandaid fix is to implement a mutex, however this mutex 
 might still be bypassed if the buffer fills up too quickly.
*/
    if (!dlock)
        return;
    
    if (uMsg == WIM_DATA)
        prntbr();
    
    if (chkset(sets, FB))
        waveOutWrite(hWaveOut, &header[aucurbuf], sizeof(WAVEHDR));
    if (++aucurbuf == NUM_BUF) aucurbuf = 0;
    waveInAddBuffer(hWaveIn, &header[aucurbuf], sizeof(WAVEHDR));
}

// Initialize things for winapi
/*
returns 0 on success
*/
int auinit() {
    int err = 0;
    
    // Init wave format
    WAVEFORMATEX format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = SAMPLERATE;
    format.wBitsPerSample = BITRATE;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * format.wBitsPerSample / 8;
    format.cbSize = 0;
    formatex = format;
    
    // Buffer size for each buffer. Once this fills up (1 / FRAMESPERSECOND seconds)
    // the driver will trigger CALLBACK. 
    size_t bpbuff = (BITRATE / 8) * SAMPLERATE / FRAMESPERSECOND;
    printf("samples per frame: %d\n", bpbuff);
    // Init cyclical buffer
    pbuf = (char *) malloc(bpbuff * NUM_BUF);
    
    // Open devices
    err += waveInOpen(&hWaveIn, WAVE_MAPPER, &format, (DWORD_PTR) whndl, 0, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR;
    if (chkset(sets, FB))
        err += waveOutOpen(&hWaveOut, WAVE_MAPPER, &format, (DWORD_PTR) NULL, 0, WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR;
    
    // Init headers
    for (int i = 0; i < NUM_BUF; i++) {
        header[i].lpData = (LPSTR) &pbuf[i * bpbuff];
        header[i].dwBufferLength = bpbuff;
        header[i].dwBytesRecorded=0;
        header[i].dwUser = 0L;
        header[i].dwFlags = 0;
        header[i].dwLoops = 0;
        err += waveInPrepareHeader(hWaveIn, &header[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    }
    
    if (!err) isauinit = 1;
    
    if (chkset(sets, DB))
        putsc(!err, "Initialized audio successfully.", "Initialized audio unsuccessfuly.");
    
    return err;
}

// Start listening to device
/*
returns 0 on success
*/
int austrt() {
    int err;
    if (!isauinit) return 1;
    err = waveInAddBuffer(hWaveIn, &header[0], sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    err += waveInStart(hWaveIn) != MMSYSERR_NOERROR;
    if (chkset(sets, DB))
        printf("%s: Device start %s\n", __FILE__, err? "UNSUCCESSFUL" : "successful");
    
    return err;
}

// Returns the current buffer 
/* 
 returns WAVEHDR of the current buffer
*/
WAVEHDR augetb() {
    int i = aucurbuf - 1;
    if (i < 0) i = NUM_BUF - 1;
    return header[i];
}

// Cleanup winapi
/*
returns 0 on success
*/
int aucln() {
    // TODO: There seems to be a problem here. After one run, buffer data is somehow corrupt (?)
    int err = 0;
    
    // TODO: Bandaid. See whndl
    dlock = 0;
    err += waveInStop(hWaveIn) != MMSYSERR_NOERROR;
    err += waveInReset(hWaveIn) != MMSYSERR_NOERROR;
    for (int i = 0; i < NUM_BUF; i++) err += waveInUnprepareHeader(hWaveIn, &header[i], sizeof(WAVEHDR));
    err += waveInClose(hWaveIn) != MMSYSERR_NOERROR;
    
    if(chkset(sets, FB)) {
        err += waveOutReset(hWaveOut) != MMSYSERR_NOERROR;
        err += waveOutClose(hWaveOut) != MMSYSERR_NOERROR;
    }
    
    free(pbuf);
    
    if (!err) isauinit = 0;
    if (chkset(sets, DB))
        printf("%s: Cleanup %s.\n", __FILE__, err? "UNSUCCESSFUL" : "successful");
    
    return err;
}

// Play .wav file using winapi
/*
file - internal custom header for .wav files
returns 0 on success
*/
int playwav(WAVC *file, int blocking) {
    int e = 0;
    
    WAVEFORMATEX format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = file->hdr.channels;
    format.nSamplesPerSec = file->hdr.samplerate;
    format.wBitsPerSample = file->hdr.bitspsample;
    format.nBlockAlign = file->hdr.blockalign;
    format.nAvgBytesPerSec = file->hdr.byterate;
    format.cbSize = 0;
    
    WAVEHDR hdr;
    HWAVEOUT hwo;
    
    e += waveOutOpen(&hwo, WAVE_MAPPER, &format, (DWORD_PTR)NULL, 0, CALLBACK_FUNCTION);
    
    hdr.lpData = (LPSTR) file->data;
    hdr.dwBufferLength = file->samples * file->ssize;
    hdr.dwFlags = 0;
    hdr.dwLoops = 0;
    e += waveOutPrepareHeader(hwo, &hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    
    e += waveOutWrite(hwo, &hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    
    if (blocking)
        Sleep((int) 1000 * file->osize / file->hdr.byterate);
    
    e += waveOutReset(hwo) != MMSYSERR_NOERROR;
    e += waveOutClose(hwo) != MMSYSERR_NOERROR;
    
    if (chkset(sets, DB))
        printf("%s: Playback %s.\n", __FILE__, e? "UNSUCCESSFUL" : "successful");
    
    return e;
}