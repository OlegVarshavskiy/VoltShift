# VoltShift v0.1

A minimal, read-only Windows power diagnostics CLI written in C.

## Build (Git Bash + GCC / MinGW-w64)

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -o voltshift.exe
./voltshift.exe
```

Git Bash alone does **not** install GCC. Install a Windows C compiler first (for example MSYS2 MinGW-w64), or use the Visual Studio Developer Command Prompt with `cl /W4 main.c /Fe:voltshift.exe`.

## Safety

This first version only reads power status and processor information through Windows APIs. It does not change settings, fan speeds, or drivers.
