Compiling and installing wmp_manager in 2013 (and beyond)
----------------------------------------------------------

## What you need:

  - A C++ compiler (like GCC)
  - make
  - libusb-0.1 (see below)

The first two comes with most Linux distros, and if you are running Mac,
then you can easily get hold of them by installing the package "Command
Line Tools", available from <a
href="https://developer.apple.com/downloads/index.action?=Command%20Line%20Tools%20%28OS%20X%20Mountain%20Lion%29">the Developer pages at Apple</a>.


## Compiling old libusb
This program was made using a library called libusb. Unfortunately the
version it was made with, 0.1, is now deprecated and no longer
compatible with the newer libusb. There is a compatibility layer for
programs using the old version called libusb-compat, but it would not
let me upload files. 
Thankfully, the old version is still available for download! You can get
it <a
href="http://sourceforge.net/projects/libusb/files/libusb-0.1%20%28LEGACY%29/0.1.12/">here</a>.
I also added a tar file in this repo, just in case SourceForge closes shop.

### Working around new compilers

To get the library to compile with modern compilers, you need to disable the
setting that treats warnings as errors. This is very easy; just open the file called Makefile, and delete the line
`AM_CFLAGS = -Werror $(AM_CFLAGS_EXT)`. 

### Easy: install libusb
Unpack it, make, and make install. That's it.

```
tar xzf libusb-0.1.12.tar.gz
cd libusb-0.1.12

# do the fix mentioned above, editing the file: 
# vim Makefile

./configure
make
make install # this might need `sudo` on your system
sudo ldconfig # to install the library on the system 
```

## Compiling `wmp-manager`
Now that the prerequisites are in place, all you need to compile the
program is to issue <code>make</code>, and you are done.

```
make

./wmp_manager
Casio WMP-1 Manager v0.1 by Florian Schmidt (schmidt_florian at gmx.de)
(based on Casio WMP-1 MMPORT Driver v1.0 - by Jimi)

can't find wmp-1!
```


