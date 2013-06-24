casio-wmp1
==========
*Software for file management on the classic Casio WMP-1 watch*

This source code enables you to use the classic Casio WMP-1 wrist watch
on most, if not all, modern platforms. It has only been tested on Linux
and OS X (Mac), but the software should also work on Windows platform, as its only
dependency is on libusb, which has been ported to Windows (libusb32).

For a bit of history and usage, see the old <a
href="src/README">README</a>

## Main contributors
- Florian Schmidt (development)
- Carl-Erik (minor bug fixes, research, documentation and code archeology)

##Changelog:

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
