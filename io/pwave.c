#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "pwave.h"
#include "audio.h"
#include "args.h"
#include "../math/math.h"

int pwav(char *fname, WAVC **res, int nchn) {
    FILE *file = fopen(fname, "rb");
    if (file == NULL) return WOPEN_ERR;
    
    int osize;
    fseek(file, 4, SEEK_CUR);
    fread(&osize, sizeof(int), 1, file);
    fseek(file, 4, SEEK_CUR);
    
    WAVC *cur = (WAVC *) malloc(sizeof(WAVC));
    *res = cur;
    
    fread(&cur->hdr, sizeof(WAVCH), 1, file);
    
    // Non-PCM files are not supported
    if (cur->hdr.ftype != 1) return WNOT_SUPPORTED;
    // Too many channels
    if (cur->hdr.channels > 2) return WNOT_SUPPORTED;
    
    cur->osize = osize;
    cur->samples = 8 * cur->hdr.dsize / cur->hdr.channels * cur->hdr.bitspsample;
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