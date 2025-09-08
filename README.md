# VirtualCamera 

A simple Qt application for Linux that streams an image or video to a virtual webcam device, using **v4l2loopback** and **ffmpeg**.  
This allows you to use static images or looping videos as your camera in apps like Discord.

---

## Features
- Select an image or video file as camera output
- Automatically detects if `v4l2loopback` is loaded
- Simple Qt-based UI

---

## Requirements
- Linux (tested on Arch Linux)
- [Qt 6](https://www.qt.io/)
- [ffmpeg](https://ffmpeg.org/)
- [v4l2loopback](https://github.com/umlaeute/v4l2loopback)

## Dependencies (for arch based)
```bash
sudo pacman -S v4l2loopback-dkms v4l-utils ffmpeg
```

# Installation
- Download file from Releases
- Make it executable
- Download dependencies
- and run it

---

Build on Arch Linux:
```bash
sudo pacman -S v4l2loopback-dkms v4l-utils ffmpeg
git clone https://github.com/muzammilshafique/VirtualCamera.git
cd VirtualCamera/
cmake CMakeLists.txt
make
```

## Screenshot

<img width="452" height="399" alt="Screenshot_20250908_064019" src="https://github.com/user-attachments/assets/fec0ba5f-c7a2-4c94-94a9-07d4430971c4" />
