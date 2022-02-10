@echo off
:: if only you could compile using gcc...
:: gcc main.c dep/getopt.c io/audio.c io/args.c io/pwave.c math/math.c math/mat.c hware/mathhw.c hware/mathw.c -luser32 -lwinmm

echo Building
:: build
nvcc -t 0 -x cu main.c dep/getopt.c io/audio.c io/args.c io/pwave.c math/math.c math/mat.c hware/mathhw.c hware/mathw.c -luser32 -lwinmm -o comp -Xcudafe --diag_suppress=2464 -run -run-args -df

echo Build done, cleanup
:: cleanup
del comp.exp
del comp.lib
