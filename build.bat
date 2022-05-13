@echo off
gcc main.c io/args.c io/audio.c io/oclapi.c io/pwave.c dep/getopt.c math/math.c math/ml.c -ggdb -lOpenCL -luser32 -lwinmm -I"%CUDA_PATH%\include" -L"%CUDA_PATH%\lib\x64" -o comp

echo --- Running Lateset Successful Build ---
comp.exe -d
