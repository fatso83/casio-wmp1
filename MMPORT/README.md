
                     ======================================
                      Martin Kiewitz's MMPORT - Version v1
                     ======================================

                             Released on 25.08.2004

                        Written and (c) by Martin Kiewitz
                            Dedicated to Gerd Kiewitz
-------------------------------------------------------------------------------

        This software is 'e-mail ware'. This means, if you use it, please send
         me an e-mail or a greeting card. I just want to see, how many people
         are using it.

  ============
  | FOREWORD |
  ============

        It all started, when I bought that great Casio WMP-1 watch. It was able
         to play MP3s and had 32MB embedded RAM on it. Nowadays this doesn't
         seem much, but it was at that time. Anyway the main problem with that
         watch is that it only has Windoze support. Please let me be more
         precise.

         It only has Windoze 98 (!) support. Yes, this means that this watch is
          now almost worthless and is sold on eBay for peanuts.

         Well, I'm using OS/2 / eComStation at home, so I was pissed off. I
          tried to contact Casio for getting technical information. I didn't
          get it of course, so I had to reverse-engineer it. The protocol is
          kinda easy, so this was not much work. I wrote myself a simple Rexx
          upload script first - later I included a download (!) option. Note
          that DOWNLOADING from the watch is NOT possible under Windoze. It was
          a hidden feature :D

         Later, I thought of some generic and simple overlay layer, so that one
          is able to upload/download multiple files, change the order of the
          files and delete them without waiting for previous completion. That's
          why I did MMPORT. Nearly anyone with little Rexx knowledge should be
          able to port other drivers over (like for the old Rio players that
          worked via parallel port) or develop new drivers for USB devices.

  ================
  | INSTALLATION |
  ================

         Just unpack all files from the archive to a directory of your choice.
         If you don't have XtraRexx installed to your system or the installed
          XtraRexx version is below 0.40, copy XtraRexx.DLL to \OS2\DLL.

         Afterwards change to that directory and enter 'mmport createicons' in
          a commandline. This will create 2 WPS-objects. One is for drag-drop
          to upload files to devices and one is for accessing the command-line
          version, where one is able to do all sorts of things.

         Currently MMPORT only supports MP3 files. Anyway, it's easily
          enhanceable.

         Type "HELP" in MMPORT command line mode to get a listing of all
          possible commands including parameters.

  =======================
  | CASIO WMP-1 SUPPORT |
  =======================

        My Casio WMP-1 is kinda tested out, because I used those scripts for
         about 2-3 years now. I found out one problem that bothered me some
         days ago. If you leave the dongle (the load station of the watch)
         alone for a long time and then connect your watch again AND the driver
         says "no compatible hardware" after about 10-15 seconds, simply press
         in the slot-in mechanism. The station and the watch are unable to
         connect to each other, strangely power loading works anyway.
         After I did this for about 10 times while checking, the whole thing
         worked again w/o any manual interaction. I tried the original Casio
         software and it had the same problem. I checked with Casio, but they
         were kinda "strange" and told me that their software would only work
         on Windoze 98 (which I already told them in my question mail).

         Setting of title and artist names is possible using the DSET command.
          Use DSET [track] title "Smack My Bush Up"
            to set the title to >Smack My Bush Up<.
          Use DSET [track] artist "The Prodigy"
            to set the artist to >The Prodigy<.

         Putting animations on the watch is not possible currently. I know how
          to, but I never use that feature and I think it sucks and is kinda
          useless.

  ================
  | ENHANCEMENTS |
  ================

         If you want to include support for another portable multimedia device,
          please send me an e-mail about it including the script. I will put it
          into further releases. If new features are required, I will implement
          them into MMPORT as well.

         For USB4REXX usage check USB-DRIVERS\USB4REXX\USB4REXX.txt!

         Currently MMPORT will ALWAYS use my Casio-WMP1 driver. This will
          change, if someone is willing to do another driver for another device.

  =====================
  | POSSIBLE PROBLEMS |
  =====================

         Currently MMPORT plays back beep.wav after finishing all queued
          operations *and* it will also send the same thing to the internal
          speaker. If both are active, you will hear both. If you don't like
          this, simply remove the MCI function in MMPORT.CMD
          The same is true, if you have problems with MCI on your system.

         Something doesn't work or doesn't work like I want it to work?!
          Hey, that's why you got the full Rexx source-code.

         I want to do a Windoze/Linux/Unix/BeOS-whatever Casio WMP1 driver!
          Great, but I don't. You got the sources, so there should be no
          problem for you. You won't even have to reverse-engineer anything.
          Please don't ask me any questions, if you haven't tried to find them
          out by yourself looking at the sources.

  ===============
  | USB-DRIVERS |
  ===============

         To get the Casio WMP-1 working under OS/2, I had to rewrite the
          enumeration process (in USBD.SYS). The one by IBM was perfectly
          following the standard, but because Windoze does it different,
          strangely the enum doesn't work on the watch. IBM got informed 2
          years ago and I never received real feedback. Probably they fixed it,
          probably not. I included the full source-code changes.

         USBD.SYS prior 19/09/2002 (e.g. eComStation 1.1 release) does NOT
          work. Earlier definitely not as well.

         Also my own USB4REXX.DLL (&source) is included in this package.
          For USB4REXX usage check USB-DRIVERS\USB4REXX\USB4REXX.txt!

         USB4REXX.DLL is using USBCALLS.DLL, but the last version (2 years ago)
          had a huge bug that CRASHED the whole OS/2 (kernel mode exception),
          when doing bulk transfers, b/c of mishandling of a pointer. I also
          included the code to fix it by yourself in the latest sources, if
          that was not already being done.

  ===========
  | CONTACT |
  ===========

        There were several ways to contact me.
        Just try 'jimi@klinikum-konstanz.de'.
         Please DO NOT POST THIS E-MAIL TO USENET NOR THE WEB!!!

        My official homepage is at: http://www.ecomstation.ru/kiewitzsoft/

        I'm dedicating MMPORT to my father.
