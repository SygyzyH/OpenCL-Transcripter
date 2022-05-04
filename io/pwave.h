/* date = February 5th 2022 11:36 am */

#ifndef FPARSE_H
#define FPARSE_H

#include <stdint.h>

enum WAV_ERR { WNO_ERR=0, WSMALL_BUF, WOPEN_ERR, WNOT_SUPPORTED };

// Packing is nescessery when loading using fprintf
#pragma pack(push, 1)
typedef struct {
    unsigned char fmt[4]; // "fmt\0" string
    uint32_t fsize; // size of the format data
    uint16_t ftype; // format type.
    /* 1: PCM, 3: IEEE float, 6: 8bit A law, 7 - 8bit mu law. 
only type 1 is supported. */
    uint16_t channels; // channel count. After parse, would always be 1.
    uint32_t samplerate; // samplerate. Expected 44100 (CD)
    uint32_t byterate; // byterate = SampleRate * NumChannels * BitsPerSample / 8
    uint16_t blockalign; // block align. NumChannels * BitsPerSample / 8
    uint16_t bitspsample; // bits per sample. Expected 16 bits
    unsigned char dchunkh[4]; // "DATA" or "FLLR"
    uint32_t dsize; // size of the next chunk that will be read. NumSamples * NumChannels * BitsPerSample / 8 
} WAVCH;
#pragma pack(pop)

typedef struct {
    uint32_t osize;
    WAVCH hdr;
    
    unsigned long int samples; 
    unsigned short int ssize;
    unsigned char *data;
} WAVC;

WAVC* frmtowav(WAVEFORMATEX format, unsigned char *data, unsigned int dsize);
int pwav(char *fname, WAVC **res, int nchn);
int wavtod(WAVC *src, double **res, int norm);

#endif //FPARSE_H
