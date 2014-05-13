casio-wmp1
==========
*Software for file management on the classic Casio WMP-1 watch*

This source code enables you to use the classic Casio WMP-1 wrist watch
on most, if not all, modern platforms. It has only been tested on Linux
and OS X (Mac), but the software should also work on Windows platform, as its only
dependency is on libusb, which has been ported to Windows (libusb32).

For a bit of history and usage, see the old <a
href="original_documentation/README">README</a>

## Additional info
I have kept an old copy of the original software for Windows 98 <a
href="https://dl.dropboxusercontent.com/u/514315/binaries/casio_wmp1/casio_wap.zip">Casio WAP.</a> Still worked in Windows XP the last time I tried it (using Win98 emulation mode). Do not know about Windows 7 though ... But if you are on the lookout, then this is your lucky day :)

I also have a copy of the original contents of the MMC (including firmware) in
case you should somehow manage to fuck it up. Can be found <a
href="https://dl.dropboxusercontent.com/u/514315/binaries/casio_wmp1/wmp1_org_firmware.dat">here</a>.

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
