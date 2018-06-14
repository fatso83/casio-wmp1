@echo off
set INCLUDE=%INCLUDE%;%INCLUDE_TOOLKIT%;%INCLUDE_WATCOM%
set LIB=%LIB%;%LIB_TOOLKIT%;%LIB_WATCOM%
\IbmC\bin\icc /W2 -Ge- /C /Ms Usb4Rexx.c
if errorlevel 1 goto End
ilink Usb4Rexx.obj ..\..\JimiHelp\stdcode\globstr.obj ..\..\JimiHelp\stdcode\globrexx.obj rexx.lib Usb4Rexx.def
:End
