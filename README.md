# DaVinci Resolve FFmpeg AAC Audio Encoder Plugin

This project is a DaVinci Resolve audio encoder plugin for AAC using FFmpeg. 
It builds a plugin compatible with the IOPlugins system of DaVinci Resolve Studio (Free doesn't support plugins) for Linux.

<p align="center">  
  <img style="width:400px;" src="./image.png" />
</p>

<p align="center">
  <a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WUAAG2HH58WE4" title="Donate via Paypal"><img height="36px" src="https://github.com/Toxblh/MTMR/raw/master/Resources/support_paypal.svg" alt="PayPal donate button" /></a>
  <a href="https://www.buymeacoffee.com/toxblh" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" height="36px" ></a>
  <a href="https://www.patreon.com/bePatron?u=9900748"><img height="36px"  src="https://c5.patreon.com/external/logo/become_a_patron_button.png" srcset="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png 2x"></a>
</p>
<p align="center">
  <a href="https://www.donationalerts.com/r/toxblh" target="_blank"><img src="https://github.com/user-attachments/assets/84b50c9c-f135-4f88-97b4-3a9efe53e48f" alt="Donation Alrets" height="36px" ></a>
  <a href="https://boosty.to/toxblh" target="_blank"><img src="https://github.com/user-attachments/assets/4a6113a6-fd33-4f69-a104-5a61bd230e3e" alt="Boosty" height="36px" ></a>
</p>

## Features
- AAC encoding using FFmpeg

## Requirements
- DaVinci Resolve Studio (FREE does NOT support plugins!)
- FFmpeg

## Install
- Download aac_encoder_plugin-linux-bundle.tar.gz from [Releases](https://github.com/Toxblh/davinci-linux-aac-codec/releases)
- Unpack
- Exec `./install.sh` (or check Readme inside)

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
