# Hackweek WebGPU

Wanted to play around with WebGPU, here's a repository that covers some basic 3D rendering using it.

## Build

### Automatic (Windows)

**Build project & run**

```
./build.bat
```

### Manual

**Create build directories**

```
mkdir build
cd build
```

**Build project**

```
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Run**

```
.\Release\HackweekWebGPU.exe
```