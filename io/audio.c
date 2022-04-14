#include "audio.h"
#include "pwave.h"
#include "args.h"
#include "../math/math.h"

HWAVEIN hWaveIn;
HWAVEOUT hWaveOut;
WAVEHDR header [NUM_BUF];

int ibuf = 0;
short int *pbuf;
int dlock = 1;
int isauinit = 0;

void CALLBACK whndl(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void prntbr();

void prntbr() {
    // Short int minimal limit
    short int max = -32768;
    for (int i = 0; i < header[ibuf].dwBufferLength / BITRATE / 8; i++) {
        short int cur = ((short int *)(header[ibuf].lpData))[i];
        if (cur > max)
            max = cur;
    }
    
    double *dmaxa = (double *) malloc(sizeof(double) * 2);
    dmaxa[0] = (double) CLICAP; dmaxa[1] = (double) max;
    int e = amptodb(&dmaxa, 2, dmaxa[0], SCALE_FIRST);
    
    double dmax = dmaxa[1];
    free(dmaxa);
    
    char block[CLICAP + 1];
    for (int i = 0; i < CLICAP; i++) {
        if (i < (int) dmax)
            block[i] = CLICHR;
        else
            block[i] = ' ';
    }
    block[CLICAP] = '\0';
    
    printf("%c[%s> Prediction: [ %s ]", chkset(sets, CS) ? '\n' : '\r', block, "Unknown");
    fflush(stdout);
}

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
        waveOutWrite(hWaveOut, &header[ibuf], sizeof(WAVEHDR));
    if (++ibuf == NUM_BUF) ibuf = 0;
    waveInAddBuffer(hWaveIn, &header[ibuf], sizeof(WAVEHDR));
}

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
    
    // Buffer size for each buffer. Once this fills up (1 / SAMPLEFRAME seconds)
    // the buffer should fill up and the driver will trigger CALLBACK. 
    size_t bpbuff = SAMPLERATE * BITRATE / 8 / FRAMESPERSECOND;
    // Init cyclical buffer
    pbuf = (short int *) malloc(bpbuff * NUM_BUF * BITRATE / 8);
    
    // Open devices
    err += waveInOpen(&hWaveIn, WAVE_MAPPER, &format, (DWORD_PTR)whndl, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR;
    if (chkset(sets, FB))
        err += waveOutOpen(&hWaveOut, WAVE_MAPPER, &format, (DWORD_PTR)NULL, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR;
    
    // Init headers
    for (int i = 0; i < NUM_BUF; i++) {
        header[i].lpData = (LPSTR) &pbuf[i * bpbuff];
        header[i].dwBufferLength = bpbuff * sizeof(*pbuf);
        header[i].dwFlags = 0;
        header[i].dwLoops = 0;
        err += waveInPrepareHeader(hWaveIn, &header[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    }
    err += waveInAddBuffer(hWaveIn, &header[0], sizeof(WAVEHDR)) != MMSYSERR_NOERROR;
    
    if (!err) isauinit = 1;
    if (chkset(sets, DB))
        printf("%s: Initialized audio %s.\n", __FILE__, err? "UNSUCCESSFULY" : "successfuly");
    
    return err;
}

int austrt() {
    int err;
    if (!isauinit) return 1;
    err = waveInStart(hWaveIn) != MMSYSERR_NOERROR;
    if (chkset(sets, DB))
        printf("%s: Device start %s\n", __FILE__, err? "UNSUCCESSFUL" : "successful");
    
    return err;
}

int aucln() {
    int err = 0;
    
    // TODO: Bandaid. See whndl
    dlock = 0;
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

int playwav(WAVC *file) {
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
    
    Sleep((int) 1000 * file->osize / file->hdr.byterate);
    
    e += waveOutReset(hwo) != MMSYSERR_NOERROR;
    e += waveOutClose(hwo) != MMSYSERR_NOERROR;
    
    if (chkset(sets, DB))
        printf("%s: Playback %s.\n", __FILE__, e? "UNSUCCESSFUL" : "successful");
    
    return e;
}