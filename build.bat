@echo off
gcc main.c io/args.c io/audio.c io/pwave.c dep/getopt.c math/math.c math/ml.c -lOpenCL -luser32 -lwinmm -I"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.2\include" -L"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.2\lib\x64" -o comp

echo --- Running Lateset Successful Build ---
comp.exe -f -d
