#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "pwave.h"
#include "audio.h"
#include "args.h"
#include "../math/math.h"

// Read .wav file, prepare it, and return its data to user. 
// (Will also play it if debug flag is set)
/*
fname - file name of the .wav file to be played
res - result of the parsing. Automaticly allocated
nchn - number of channels requested, either 1 or 2. If channel size of the file doesn't match, the data will be modified to match
returns 0 on success
*/
int pwav(char *fname, WAVC **res, int nchn) {
    FILE *file = fopen(fname, "rb");
    if (file == NULL) return WOPEN_ERR;
    
    uint32_t osize;
    fseek(file, 4, SEEK_CUR);
    fread(&osize, sizeof(int), 1, file);
    fseek(file, 4, SEEK_CUR);
    
    WAVC *cur = (WAVC *) malloc(sizeof(WAVC));
    *res = cur;
    
    fread(&cur->hdr, sizeof(WAVCH), 1, file);
    
    // Find DATA or FLLR header
    char *tdchnkh = (char *) malloc(5);
    memcpy(tdchnkh, cur->hdr.dchunkh, 4);
    tdchnkh[4] = 0;
    // TODO: Not 100% this is on spec, since permutations are also included (such as "LRfl")
    // Should be good enough though
    while (strlen(tdchnkh) == 0 || !strstr("DATAdataFLLRfllr", tdchnkh)) {
        fseek(file, -3, SEEK_CUR);
        
        // If reached EOF or otherwise an error has occured, return error 
        if (fread(&cur->hdr.dchunkh, sizeof(char), 4, file) != 4) return WOPEN_ERR;
        memcpy(tdchnkh, cur->hdr.dchunkh, 4);
    } free(tdchnkh);
    
    fread(&cur->hdr.dsize, sizeof(cur->hdr.dsize), 1, file);
    
    // Non-PCM files are not supported
    if (cur->hdr.ftype != 1) return WNOT_SUPPORTED;
    // Too many channels
    if (cur->hdr.channels > 2) return WNOT_SUPPORTED;
    
    cur->osize = osize;
    cur->samples = 8 * cur->hdr.dsize / (cur->hdr.channels * cur->hdr.bitspsample);
    cur->ssize = cur->hdr.channels * cur->hdr.bitspsample / 8;
    cur->data =  (unsigned char *) malloc(cur->samples * cur->ssize);
    // Since data aligment is irrelevent, its more efficient
    // to take bigger slices of a smallar array rather than
    // taking 1 Byte slices out of a bigger array
    // fread(cur->data, 1, cur-ssize * cur->samples, file);
    fread(cur->data, cur->ssize, cur->samples, file);
    
    if (cur->hdr.channels == 2 && nchn) {
        if (chkset(sets, DB))
            printf("%s: Removing channel from \"%s\".\n", __FILE__, fname);
        
        // Remove a channel
        unsigned char buf[4];
        unsigned char *ndata = (unsigned char *) malloc(cur->samples * cur->ssize / 2);
        for (int i = 0; i < cur->samples; i++) {
            if (i % 2 == 1) {
                for(int j = 0; j < cur->ssize / 2; j++) {
                    buf[j] = cur->data[j + i * cur->ssize];
                }
            } else {
                for (int j = 0; j < cur->ssize / 2; j++) {
                    ndata[j + i / 2] = buf[j];
                }
            }
        }
        
        // Recompute
        cur->hdr.channels = 1;
        cur->hdr.byterate = cur->hdr.samplerate * cur->hdr.bitspsample / 8;
        cur->hdr.blockalign = cur->hdr.bitspsample / 8;
        cur->hdr.dsize /= 2;
        // Sample count remains the same, only the sample size is decreased
        cur->ssize /= 2;
        
        free(cur->data);
        
        cur->data = ndata;
        
        cur->osize = (sizeof(WAVCH) + cur->ssize * cur->samples) * 8;
    }
    
    fclose(file);
    
    if (chkset(sets, FB)) {
        if (chkset(sets, DB))
            printf("%s: Playing \"%s\"\n", __FILE__, fname);
        
        playwav(cur);
    }
    
    return WNO_ERR;
}

// Convert WAVEFORMATEXX (winapi) to WAVC (custom) so that a .wav file can be parsed
/*
format - winapi formmated header
data - raw PCM data
dsize - size of data
*/
WAVC* frmtowav(WAVEFORMATEX format, unsigned char *data, unsigned int dsize) {
    WAVC *res = (WAVC *) malloc(sizeof(WAVC));
    
    res->osize = 36 + dsize;
    
    for (int i = 0; i < 4; i++)
        res->hdr.fmt[i] = "fmt"[i];
    res->hdr.fsize = dsize + 36;
    if (format.wFormatTag != WAVE_FORMAT_PCM) return NULL;
    res->hdr.ftype = 1;
    res->hdr.channels = format.nChannels;
    res->hdr.samplerate = format.nSamplesPerSec;
    res->hdr.byterate = format.nAvgBytesPerSec;
    res->hdr.blockalign = format.nBlockAlign;
    res->hdr.bitspsample = format.wBitsPerSample;
    for (int i = 0; i < 4; i++)
        res->hdr.dchunkh[i] = "DATA"[i];
    res->hdr.dsize = dsize;
    
    res->samples = 8 * dsize / res->hdr.channels * res->hdr.bitspsample;
    res->ssize = res->hdr.channels * res->hdr.bitspsample / 8;
    res->data = data;
    
    return res;
}

// Converts native wav file data to array of doubles
/*
src - source wav file
res - result (allocated automaticly, sized src->samples)
returns 0 on success
*/
int wavtod(WAVC *src, double **res, int norm) {
    int bps = (int) (src->hdr.bitspsample / 8);
    
    // Check if size is "operateable" on this arch
    if (bps != sizeof(short int) && bps != sizeof(int) && bps != sizeof(long int) && bps != sizeof(long long int)) return WNOT_SUPPORTED;
    
    double *r = (double *) malloc(sizeof(double) * src->samples);
    
    // long long int will always fit, since its unsigned and im checking for
    // signed.
    int maxsig = (int) (1LL << bps * 8) / 2;
    printf("max signed int of size %d is %d?\n", bps * 8, maxsig);
    
    for (int i = 0; i < src->samples; i++) {
        if (bps == sizeof(short int))
            r[i] = (double) ((short int *) src->data)[i];
        else if (bps == sizeof(int))
            r[i] = (double) ((int *) src->data)[i];
        else if (bps == sizeof(long int))
            r[i] = (double) ((long int *) src->data)[i];
        else if (bps == sizeof(long long int))
            r[i] = (double) ((long long int *) src->data)[i];
        
        // Matlab normalizes inputs unless specified otherwise,
        // and so should we.
        if (norm) {
            int offset = (r[i] > 0) ? 1 : 0;
            
            // Due to how signed integers have less positive numbers
            // than negative ones, theres a need to account for the offset.
            r[i] = r[i] / (maxsig - offset);
        }
    }
    
    *res = r;
    
    return WNO_ERR;
}