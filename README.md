# OpenCL-Transcripter
Transcribes input audio to word bins.

Using a machine trained in MATLAB, the code uses WINMM (WIN32 API) to take user input from microphone, and converts it to a log-mel spectrogram.
The spectrogram is than passed to the machine algorithm which is computed using OpenCL, at runtime. The result is than displayed in the CLI.

## Requirements
Compiling and running the program **only requires an OpenCL implementation**. Most modern GPUs will come pre-equiped with an implementation, or at least a .dll,
however, to compile the program either the OpenCL path is in the default linker path or specified in the build command. The `build.bat` file is
expecting NVIDIA implementation.

The compiled result does not staticly include the machine data, it expects to find it in `./machine/...`. This means that after compiling, a
folder named `machine` with all the included binary files must be in the same directory as the executble.

Currently the audio interface `audio.h` is written for Windows only, therefore, using Windows (or some kind of emulation) is also required.

## Usage
Running `build.bat` will build the project from source and produce `comp.exe` (on Windows).
By default, `build.bat` will also run the program with the debug `-d` flag.
`comp.exe` can be run with the following flags:
- `-h`: prints help message.
- `-d`: enables debug printing.
- `-c`: enables cascading output.
- ~~`-o`-: writes output to file~~ obselete, simply piping the result using bash is sufficiant.
- ~~`-f`: enables live auditory feedback~~ feedback is always displayed visually in the CLI.

## TODO: Improvments
- Using CMake will both increase compatability and allow for a clearer way of staticly including the `.cl` kernel code. Perhaps this will also allow for
staticly including the machine binary in an elagent way.
- Using a cross-platform audio interface, like libsoundio (pretty much requires CMake). This will make the project mostly platform independent.
- Remove obselete flags
- Cleaning up and refractoring, especially in `main.c`.
