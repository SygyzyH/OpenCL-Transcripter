/* date = February 5th 2022 11:36 am */

#ifndef FPARSE_H
#define FPARSE_H

enum WAV_ERR { WNO_ERR=0, WSMALL_BUF, WOPEN_ERR, WNOT_SUPPORTED };

// Packing is nescessery when loading using fprintf
#pragma pack(1)
typedef struct {
    unsigned char fmt[4]; // "fmt\0" string
    unsigned int fsize; // size of the format data
    unsigned short int ftype; // format type.
    /* 1: PCM, 3: IEEE float, 6: 8bit A law, 7 - 8bit mu law. 
only type 1 is supported. */
    unsigned short int channels; // channel count. After parse, would always be 1.
    unsigned int samplerate; // samplerate. Expected 44100 (CD)
    unsigned int byterate; // byterate = SampleRate * NumChannels * BitsPerSample / 8
    unsigned short int blockalign; // block align. NumChannels * BitsPerSample / 8
    unsigned short int bitspsample; // bits per sample. Expected 16 bits
    unsigned char dchunkh[4]; // "DATA" or "FLLR"
    unsigned int dsize; // size of the next chunk that will be read. NumSamples * NumChannels * BitsPerSample / 8 
} WAVCH;

typedef struct {
    unsigned int osize;
    WAVCH hdr;
    
    unsigned long int samples; 
    unsigned short int ssize;
    unsigned char *data;
} WAVC;

WAVC* frmtowav(WAVEFORMATEX format, unsigned char *data, unsigned int dsize);
int pwav(char *fname, WAVC **res, int nchn);


#endif //FPARSE_H
