## What is it ?
System primitives and architectural Modules for RT apps.

1. Linear Allocators


# Build

Install these first:
```bash
sudo apt update
sudo apt install build-essential git cmake ninja-build \
    libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
    libxinerama-dev libwayland-dev libxkbcommon-dev
```

## Configure

```bash
cmake -S . -B build -G Ninja

cmake --build build -j
```

## Run

```bash
./build/application/linear_allocator_app/linear_allocator_app
```