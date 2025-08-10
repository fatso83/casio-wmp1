/* Test Rexx - by Kiewitz */
call RxFuncAdd "USBLoadFuncs","Usb4Rexx","USBLoadFuncs"
call USBLoadFuncs

say "Attached USB devices: "USBEnumDevices()
say USBGetLastError()
