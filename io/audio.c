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

char *pbuf;
int blocked = 0;
int isauinit = 0;

void CALLBACK whndl(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void prntbr();

// Computes the integrated loundness of the signal and returns the RMS in dB
double computevu(double *signal, int sz) {
    double sum = 0;
    for (int i = 0; i < sz; i++) {
        double t = signal[i];//20 * log10(signal[i] / max);
        sum += t * t;//* t;
    }
    
    double rms = sqrt(sum / sz);
    return 10 * log10(rms);
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
    if (uMsg == WIM_DATA && header[aucurbuf].dwBytesRecorded == header[aucurbuf].dwBufferLength)
        blocked = 0;
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
    size_t bpbuff = (size_t) (BITRATE / 8) * SAMPLERATE / FRAMESPERSECOND;
    // Ensure bpbuff is devisable by byterate, otherwise a few bytes may be read
    // out of the audio buffer
    if (bpbuff % (BITRATE / 8) != 0) bpbuff += (BITRATE / 8) - bpbuff % (BITRATE / 8);
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
        header[i].dwBytesRecorded = 0;
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
    blocked = 1;
    
    return err;
}

// Returns the current buffer
// Blocks until the buffer is shuffled
/* 
 returns WAVEHDR of the current buffer
*/
WAVEHDR augetb() {
    WAVEHDR non;
    if (!isauinit) return non;
    while (blocked);
    int returnindex = aucurbuf;
    if (++aucurbuf == NUM_BUF) aucurbuf = 0;
    waveInAddBuffer(hWaveIn, &header[aucurbuf], sizeof(WAVEHDR));
    blocked = 1;
    return header[returnindex];
}

// Cleanup winapi
/*
returns 0 on success
*/
int aucln() {
    int err = 0;
    while(blocked);
    
    for (int i = 0; i < NUM_BUF; i++) err = waveInUnprepareHeader(hWaveIn, &header[i], sizeof(WAVEHDR));
    err += waveInStop(hWaveIn) != MMSYSERR_NOERROR;
    err += waveInReset(hWaveIn) != MMSYSERR_NOERROR;
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