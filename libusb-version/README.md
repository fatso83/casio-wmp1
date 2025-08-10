Compiling and installing wmp_manager in 2013 (and beyond)
----------------------------------------------------------

## What you need:

  - A C++ compiler (like GCC)
  - make
  - libusb-0.1 for development (see below)

The first two comes with most Linux distros, and if you are running Mac,
then you can easily get hold of them by installing the package "Command
Line Tools", available from <a
href="https://developer.apple.com/downloads/index.action?=Command%20Line%20Tools%20%28OS%20X%20Mountain%20Lion%29">the Developer pages at Apple</a>.

Installing libusb shouldn't be more than
```
sudo apt-get install libusb-dev
```
Should you for some reason need to build it from scratch, see [this file](./libusb.md)).

## Compiling `wmp-manager`
Now that the prerequisites are in place, all you need to compile the
program is to issue <code>make</code>, and you are done.

```
make
```

That's it. You should now have a file that you can call:
```
./wmp_manager
Casio WMP-1 Manager v0.1 by Florian Schmidt (schmidt_florian at gmx.de)
(based on Casio WMP-1 MMPORT Driver v1.0 - by Jimi)

can't find wmp-1!
```

If you get errors like not finding wmp-1, then try running it using `sudo`.
