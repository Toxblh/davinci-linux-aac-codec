# DaVinci Resolve FFmpeg AAC Audio Encoder Plugin

This project is a DaVinci Resolve audio encoder plugin for AAC using FFmpeg. 
It builds a plugin compatible with the IOPlugins system of DaVinci Resolve Studio (Free doesn't support plugins) for Linux.

## Features
- AAC encoding using FFmpeg

## Requirements
- DaVinci Resolve Studio (FREE does NOT support plugins!)
- FFmpeg

------------

## Build Requirements
- Linux x86-64
- C++ compiler (g++ >= 9 recommended)
- FFmpeg development libraries (`libavcodec-dev`, `libavutil-dev`)
- CMake (if you want to use it)
- Make

### Install dependencies (Ubuntu/Debian example)
```
sudo apt update
sudo apt install build-essential pkg-config libavcodec-dev libavutil-dev
```

## Build

To build the plugin and package it for DaVinci Resolve:

```
./build.sh
```

The output will be in:
```
aac_encoder_plugin.dvcp.bundle/Contents/Linux-x86-64/aac_encoder_plugin.dvcp
```

## Install

To install the plugin into DaVinci Resolve:

```
sudo ./install.sh
```

This will copy the plugin bundle to `/opt/resolve/IOPlugins`.

---

## License
GPLv3
