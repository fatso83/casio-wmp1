Casio WMP 1 WebUSB Manager 🚀
================================

> Modern web-based file management for the classic Casio WMP-1 watch using WebUSB

<img src="./assets/b1b72ibsxkfugfeuxcpl.jpg" alt="white background casio wmp 1" width="400px"/>

**NEW (2025)**: This project has been ported to WebUSB, thanks to @simenf! 🎉 You can now manage your Casio WMP-1 watch directly from your web browser without installing any software or drivers.

The Casio WMP 1 was the world's first MP3 watch when it came out in 2000. You can read more
about it [on the 50 year anniversery page for Casio](https://web.archive.org/web/20250410093512/https://www.casio.com/europe/watches/50th/Heritage/2000s/).

## 🌐 WebUSB Web Application

The easiest way to use this software is through the new WebUSB-powered web application. This modern interface runs directly in your browser and provides:

- ✅ **No installation required** - works directly in modern browsers
- ✅ **Cross-platform compatibility** - works on Windows, macOS, and Linux
- ✅ **User-friendly interface** - drag & drop file management
- ✅ **Real-time file operations** - upload, download, and delete files
- ✅ **MP3 validation** - ensures files are compatible with the watch

### How to Use the WebUSB App

1. **Open the web application**: Spin up a local web server (see below) and access `/casio-wmp-manager.html` in a modern web browser (Chrome, Edge, or Opera - WebUSB is not supported in Firefox or Safari)

2. **Connect your watch**: 
   - Connect your Casio WMP-1 to your computer via USB
   - Click the "Connect to Watch" button in the web app
   - Select your watch from the device list when prompted

3. **Manage files**:
   - **Upload MP3s**: Drag and drop MP3 files onto the upload area or click "Choose MP3 Files"
   - **Download files**: Click the download button next to any file on the watch
   - **Delete files**: Click the delete button next to files you want to remove
   - **View file info**: See file names, sizes, and details in the file list

### Requirements

- **Browser**: Chrome 61+, Edge 79+, or Opera 48+ (WebUSB support required)
- **USB**: Your Casio WMP-1 watch connected via USB cable
- **Files**: MP3 files at 96-128kbps, 44.1kHz (the app will validate compatibility)

### Running the Web App

Webusb only works over https or from localhost, which means your options are spinning it up yourself or try the hosted version

# Option 1: Serve locally (recommended for security)
```
python -m http.server 8000
```
Then open http://localhost:8000/casio-wmp-manager.html

# Option 2: external hosting
Access it at [wmp1.simenf.com](https://wmp1.simenf.com) hosted with cloudflare pages.

# 🖥️ Original Command-Line Software

For those who prefer the original command-line interface or need to use older systems, the original C++ implementation is still available below. Something in the legacy libusb code seems to be broken on newer Linux distros (see [#3](https://github.com/fatso83/casio-wmp1/issues/3)), so this has not been working for some time. Which is why the port to WebUSB was really welcome!

### Legacy Command-Line Interface

This source code _should_ enable you to use the classic Casio WMP-1 wrist watch
on all modern platforms, as its only dependency is libusb. 
Unfortunately it only works successfully in Linux at the moment:
- [Unable to compile libusb32](https://github.com/fatso83/casio-wmp1/issues/2) on Windows. 
- libusb has a bug in OS X [that aborts transfers](https://github.com/fatso83/casio-wmp1/issues/1)

The interface is a clunky text based interface, but it works reasonably well - 
some would say better than the original Windows software, although that wouldn't say much :)

For a bit of history and usage, see the old <a
href="original_documentation/README">README</a>

### Installing the Legacy Software
See [./libusb-version/README.md](./libusb-version/README.md).

## Additional Info (Legacy Software)
I have kept an old copy of the original software for Windows 98 <a
href="./assets/casio_wap.zip">Casio WAP.</a> Still worked in Windows XP the last time I tried it (using Win98 emulation mode). Do not know about Windows 7 though ... But if you are on the lookout, then this is your lucky day :)

I also have a copy of the original contents of the MMC (including firmware) in
case you should somehow manage to fuck it up. Can be found <a
href="./assets/wmp1_org_firmware.dat">here</a>.

## Todo
- ✅ **COMPLETED**: Cross-platform compatibility achieved with WebUSB implementation
- Update to use the [newer lib usb](https://sourceforge.net/p/libusb/mailman/libusb-devel/thread/CAGjSPUCSPkCjKvrAVH7uzG2aFv1fG07gtko-wQGSBPwi2v3k5Q%40mail.gmail.com/) for legacy CLI version. Also see issue #1.

## Main contributors
- Florian Schmidt (development)
- Carl-Erik (minor bug fixes, research, documentation and code archeology)
- Martin Kiewitz (wrote the [original REXX code for OS/2](os2-version/MMPORT/))
- SimenF (WebUSB port)

## Changelog:

### August 2025 - WebUSB Port 🚀
Complete rewrite using WebUSB technology:
- **Cross-platform web application** that works in modern browsers (Chrome, Edge, Opera)
- **No installation required** - no drivers, no compilation, no dependencies
- **Modern drag & drop interface** for easy file management
- **Real-time file operations** with progress indicators
- **MP3 validation** to ensure file compatibility
- **Solves all legacy platform compatibility issues** (Windows libusb32 compilation, macOS transfer bugs)

### June 14 2018 Added original REXX code
Thought it would be fun to find and add the original code for OS/2 that I sent Florian originally.
I found it on the [OS/2 archive "Hobbes"](http://hobbes.nmsu.edu/h-viewer.php?dir=/pub/os2/apps/mmedia/music&file=mmportv1.zip).
The original download site is [non-functional](http://en.ecomstation.ru/personal/kiewitzsoft/app-mmport.php) for this download.

### June 24 2013 Added INSTALL note
Added a small howto on how to build on current systems (OS X and Linux)

### May 6-9 2013 Published on GitHub
Made the source and history available on GitHub

### Version 0.2 (2005)
The latest version copies ID3v2 files without causing trouble. Should work
fine with all files with bitrate <= 32 kbit <= 128kbit.

### Version 0.1 (2005)
Initial release based Martin Kiewit's REXX code for OS/2
Up-/download working, id3, general file upload, playlist support, ++
