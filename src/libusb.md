# libusb
If you are on Ubuntu or Debian you are in luck. Installing libusb is as easy as:
```
sudo apt-get install libusb-dev
```

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

