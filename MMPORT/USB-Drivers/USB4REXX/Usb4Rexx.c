#define INCL_NOPMAPI
#define INCL_BASE
#define INCL_DOSMODULEMGR
#include <os2.h>
#define INCL_REXXSAA
#include <rexxsaa.h>                        /* needed for RexxStart()     */

#include <global.h>
#include <globstr.h>
#include <usbcalls.h>
#include <usbtype.h>

RexxFunctionHandler USBLoadFuncs;
RexxFunctionHandler USBDropFuncs;
RexxFunctionHandler USBGetLastError;
RexxFunctionHandler USBEnumDevices;
RexxFunctionHandler USBOpen;
RexxFunctionHandler USBClose;
RexxFunctionHandler USBDeviceGetDescriptor;
RexxFunctionHandler USBConfigurationGetDescriptor;
RexxFunctionHandler USBStringGetDescriptor;
RexxFunctionHandler USBDeviceGetConfiguration;
RexxFunctionHandler USBDeviceSetConfiguration;
RexxFunctionHandler USBControlMessageIn;
RexxFunctionHandler USBControlMessageOut;
RexxFunctionHandler USBStandardMessageIn;
RexxFunctionHandler USBStandardMessageOut;
RexxFunctionHandler USBClassMessageIn;
RexxFunctionHandler USBClassMessageOut;
RexxFunctionHandler USBVendorMessageIn;
RexxFunctionHandler USBVendorMessageOut;
RexxFunctionHandler USBBulkRead;
RexxFunctionHandler USBBulkWrite;

static PSZ REXX_FunctionTable[] = {
   "USBLoadFuncs",
   "USBDropFuncs",
   "USBGetLastError",
   "USBEnumDevices",
   "USBOpen",
   "USBClose",
   "USBDeviceGetDescriptor",
   "USBConfigurationGetDescriptor",
   "USBStringGetDescriptor",
   "USBDeviceGetConfiguration",
   "USBDeviceSetConfiguration",
   "USBControlMessageIn",
   "USBControlMessageOut",
   "USBStandardMessageIn",
   "USBStandardMessageOut",
   "USBClassMessageIn",
   "USBClassMessageOut",
   "USBVendorMessageIn",
   "USBVendorMessageOut",
   "USBBulkRead",
   "USBBulkWrite"
 };

#define REXX_MyLoadFuncs                    USBLoadFuncs
#define REXX_MyDropFuncs                    USBDropFuncs
#define REXX_MyDLLName                      "USB4REXX"
#define REXX_WelcomeMessage                 "USB for Rexx 0.9b - PUBLIC BETA RELEASE\nWritten by Martin Kiewitz. Released under Aladdin Free Public License.\n"
#define INCL_REXX_STDLOADDROP
#include <globrexx.h>                       /* Include REXX stuff */

/* for USBGetLastError */
APIRET LastRC = 0;

// ============================================================================
unsigned long LibMain(HMODULE ModuleHandle, unsigned long Start_up) {
   return 1UL;
 }

VOID USBSetRexxResult (PRXSTRING retstr, APIRET rc) {
   switch (rc) {
    case NO_ERROR:
      retstr->strlength =  4; retstr->strptr = "DONE"; break;
    case 65311:
      retstr->strlength =  8; retstr->strptr = "NOTFOUND"; break;
    default:
      retstr->strlength =  5; retstr->strptr = "ERROR"; break;
    }
 }

// ============================================================================
ULONG APIENTRY USBGetLastError (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   if (numargs!=0)
      return REXX_INVALIDCALL;

   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBEnumDevices (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   ULONG TotalDevices = 0;

   if (numargs!=0)
      return REXX_INVALIDCALL;

   // No arguments
   if (UsbQueryNumberDevices (&TotalDevices))
      TotalDevices = 0;                     // Reply 0 devices on error

   retstr->strlength = sprintf (retstr->strptr, "%d", TotalDevices);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBOpen (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    VendorID, DeviceID;
   USHORT    EnumDevice;
   USHORT    BCDVersion;
   CHAR      TempBuffer[10];

   if ((numargs!=4) && (numargs!=5))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) ||
         ((numargs==5) && (!RXVALIDSTRING(args[4]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle-Variablename
   // args[1] - VendorID, args[2] - DeviceID
   // args[3] - EnumDevice (count, which device to open)
   // args[4] - BCDVersion of device (optional!)
   VendorID   = strtoul(args[1].strptr, NULL, 0);
   DeviceID   = strtoul(args[2].strptr, NULL, 0);
   EnumDevice = strtoul(args[3].strptr, NULL, 10);
   if (numargs==5)
      BCDVersion = strtoul(args[4].strptr, NULL, 0);
     else
      BCDVersion = USB_ANY_PRODUCTVERSION;

   LastRC = UsbOpen (&Handle, VendorID, DeviceID, BCDVersion, EnumDevice);
   if (!LastRC)
      REXX_SetVariableViaULONGHex (args[0].strptr, Handle);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

ULONG APIENTRY USBClose (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;

   if ((numargs!=1) || (!RXVALIDSTRING(args[0])))
      return REXX_INVALIDCALL;

   // args[0] - Handle
   Handle = strtoul(args[0].strptr, NULL, 16);

   LastRC = UsbClose (Handle);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBDeviceGetDescriptor (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    LanguageID = 0;
   CHAR      DescriptorSpace[18];

   if ((numargs!=1) && (numargs!=2))
      if ((!RXVALIDSTRING(args[0])) ||
         ((numargs==2) && (!RXVALIDSTRING(args[1]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle
   // args[1] - Language ID (optional)
   Handle = strtoul(args[0].strptr, NULL, 16);
   if (numargs==2)
      LanguageID = strtoul(args[1].strptr, NULL, 0);

   memset (retstr->strptr, 0, 18); // Index 0, fixed length 18
   if (!(LastRC = UsbDeviceGetDescriptor(Handle, 0, LanguageID, 18, retstr->strptr))) {
       retstr->strlength = 18;
    } else {
      retstr->strlength = 0;
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBConfigurationGetDescriptor (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE           Handle;
   USHORT              ConfigNo;
   USHORT              LanguageID = 0;
   DeviceConfiguration TempConfigDescriptor;
   PCHAR               TempBufferPtr;

   if ((numargs!=2) && (numargs!=3))
      if ((!RXVALIDSTRING(args[0])) && (!RXVALIDSTRING(args[1])) ||
         ((numargs==3) && (!RXVALIDSTRING(args[3]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle
   // args[1] - Configuration-Number
   // args[2] - Language ID (optional)
   Handle = strtoul(args[0].strptr, NULL, 16);
   ConfigNo = strtoul(args[1].strptr, NULL, 0);
   if ((ConfigNo==0) || (ConfigNo>255))
      return REXX_INVALIDCALL;
   if (numargs==3)
      LanguageID = strtoul(args[2].strptr, NULL, 0);

   retstr->strlength = 0;

   memset (retstr->strptr, 0, sizeof(DeviceConfiguration));
   if (!(LastRC = UsbConfigurationGetDescriptor(Handle, ConfigNo, LanguageID, sizeof(DeviceConfiguration), (PUCHAR)&TempConfigDescriptor))) {
      TempBufferPtr = malloc(TempConfigDescriptor.wTotalLength);
      if (TempBufferPtr) {
         // Now get the whole Configuration Descriptor
         memset (TempBufferPtr, 0, TempConfigDescriptor.wTotalLength);
         if (!(LastRC = UsbConfigurationGetDescriptor(Handle, ConfigNo, LanguageID, TempConfigDescriptor.wTotalLength, TempBufferPtr))) {
            // Got it, set result...
            retstr->strlength = TempConfigDescriptor.wTotalLength;
            retstr->strptr    = TempBufferPtr;
          } else {
            free (TempBufferPtr);
          }
       }
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBStringGetDescriptor (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    StringNo;
   USHORT    LanguageID = 0;

   if ((numargs!=2) && (numargs!=3))
      if ((!RXVALIDSTRING(args[0])) && (!RXVALIDSTRING(args[1])) ||
         ((numargs==3) && (!RXVALIDSTRING(args[3]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle
   // args[1] - String-Number
   // args[2] - Language ID (optional)
   Handle   = strtoul(args[0].strptr, NULL, 16);
   StringNo = strtoul(args[1].strptr, NULL, 0);
   if (StringNo>255)
      return REXX_INVALIDCALL;
   if (numargs==3)
      LanguageID = strtoul(args[2].strptr, NULL, 0);

   retstr->strlength = 0;

   memset (retstr->strptr, 0, 255);
   if (!(LastRC = UsbStringGetDescriptor(Handle, StringNo, LanguageID, 255, retstr->strptr))) {
      retstr->strlength = *(PUCHAR)retstr->strptr;
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBDeviceGetConfiguration (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle      = 0;
   USHORT    ConfigNo    = 0;
   USHORT    CurConfigNo = 0;
 
   if ((numargs!=1) || (!RXVALIDSTRING(args[0])))
      return REXX_INVALIDCALL;

   // args[0] - USB-Handle

   Handle = strtoul(args[0].strptr, NULL, 16);
   LastRC = UsbDeviceGetConfiguration (Handle, (PUCHAR)&CurConfigNo);

   if (!LastRC) {
      itoa(CurConfigNo, retstr->strptr, 8);
      retstr->strlength = strlen(retstr->strptr);
    } else {
      retstr->strlength = 0;
    }

   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBDeviceSetConfiguration (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle   = 0;
   USHORT    ConfigNo = 0;
 
   if ((numargs!=2) || (!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])))
      return REXX_INVALIDCALL;

   // args[0] - USB-Handle, args[1] - Configuration-Number

   Handle = strtoul(args[0].strptr, NULL, 16);
   ConfigNo = strtoul(args[1].strptr, NULL, 10);
   if ((ConfigNo==0) || (ConfigNo>255))
      return REXX_INVALIDCALL;
   LastRC = UsbDeviceSetConfiguration (Handle, ConfigNo);

   // If we succeeded, we will delay for a short time, so device is able to
   //  settle and Rexx programmer doesnt need to take care of it
   if (!LastRC)
      DosSleep(200);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBControlMessageIn (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   UCHAR     RequestType, RequestNo;
   USHORT    Value, Index, InBufferLength, TimeOut;
   PCHAR     TempBufferPtr;

   if ((numargs!=6) && (numargs!=7))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) || (!RXVALIDSTRING(args[5])) ||
         ((numargs==7) && (!RXVALIDSTRING(args[6]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle
   // args[1] - RequestType (raw)
   // args[2] - RequestNo
   // args[3] - Value
   // args[4] - Index
   // args[5] - InBufferLength
   // args[6] - Timeout (optional)
   Handle           = strtoul(args[0].strptr, NULL, 16);
   RequestType      = strtoul(args[1].strptr, NULL, 0) & 0x7F;
   RequestNo        = strtoul(args[2].strptr, NULL, 0);
   Value            = strtoul(args[3].strptr, NULL, 0);
   Index            = strtoul(args[4].strptr, NULL, 0);
   InBufferLength   = strtoul(args[5].strptr, NULL, 0);
   if (numargs==8)
      TimeOut    = strtoul(args[6].strptr, NULL, 0);
     else
      TimeOut    = 0;

   // Allocate buffer, if we are supposed to receive something
   if (InBufferLength) {
      TempBufferPtr = malloc(InBufferLength);
      memset (TempBufferPtr, 0, InBufferLength); // NUL buffer out
    }

   if (!(LastRC = UsbCtrlMessage(Handle, 0x80|RequestType, RequestNo,
        Value, Index, InBufferLength, TempBufferPtr, TimeOut))) {
       retstr->strptr    = TempBufferPtr;
       retstr->strlength = InBufferLength;
    } else {
      free (TempBufferPtr);
      retstr->strlength = 0;
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBControlMessageOut (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   UCHAR     RequestType, RequestNo;
   USHORT    Value, Index, TimeOut;

   if ((numargs!=5) && (numargs!=7))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==7) && (!RXVALIDSTRING(args[5])) && (!RXVALIDSTRING(args[6]))))
         return REXX_INVALIDCALL;

   // args[0] - Handle
   // args[1] - RequestType (raw)
   // args[2] - Request
   // args[3] - Value
   // args[4] - Index
   // args[5] - OutBuffer (optional)
   // args[6] - Timeout (optional)
   Handle           = strtoul(args[0].strptr, NULL, 16);
   RequestType      = strtoul(args[1].strptr, NULL, 0) & 0x7F;
   RequestNo        = strtoul(args[2].strptr, NULL, 0);
   Value            = strtoul(args[3].strptr, NULL, 0);
   Index            = strtoul(args[4].strptr, NULL, 0);
   if (numargs==7)
      TimeOut    = strtoul(args[6].strptr, NULL, 0);
     else
      TimeOut    = 0;

   LastRC = UsbCtrlMessage(Handle, RequestType, RequestNo,
        Value, Index, args[6].strlength, args[6].strptr, TimeOut);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBStandardMessageIn (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, Length, TimeOut;
   PCHAR     TempBufferPtr;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer length
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   Length        = strtoul(args[4].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   retstr->strlength = 0;
   TempBufferPtr     = malloc(Length);
   if (TempBufferPtr) {
      LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_STANDARD|Receipent, RequestNo, RequestValue, 0, Length, TempBufferPtr, TimeOut);
      if (!LastRC) {
         retstr->strptr = TempBufferPtr; retstr->strlength = Length;
       } else {
         free (TempBufferPtr);
       }
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBStandardMessageOut (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, TimeOut;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer (is directly used)
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_HOSTTODEV|REQTYPE_TYPE_STANDARD|Receipent, RequestNo, RequestValue, 0, args[4].strlength, args[4].strptr, TimeOut);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBClassMessageIn (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, Length, TimeOut;
   PCHAR     TempBufferPtr;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer length
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   Length        = strtoul(args[4].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   retstr->strlength = 0;
   TempBufferPtr     = malloc(Length);
   if (TempBufferPtr) {
      LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_CLASS|Receipent, RequestNo, RequestValue, 0, Length, TempBufferPtr, TimeOut);
      if (!LastRC) {
         retstr->strptr = TempBufferPtr; retstr->strlength = Length;
       } else {
         free (TempBufferPtr);
       }
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBClassMessageOut (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, TimeOut;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer (is directly used)
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_HOSTTODEV|REQTYPE_TYPE_CLASS|Receipent, RequestNo, RequestValue, 0, args[4].strlength, args[4].strptr, TimeOut);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBVendorMessageIn (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, Length, TimeOut;
   PCHAR     TempBufferPtr;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer length
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   Length        = strtoul(args[4].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   retstr->strlength = 0;
   TempBufferPtr     = malloc(Length);
   if (TempBufferPtr) {
      LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_VENDOR|Receipent, RequestNo, RequestValue, 0, Length, TempBufferPtr, TimeOut);
      if (!LastRC) {
         retstr->strptr = TempBufferPtr; retstr->strlength = Length;
       } else {
         free (TempBufferPtr);
       }
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBVendorMessageOut (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Receipent, RequestNo, RequestValue, TimeOut;

   if ((numargs!=5) && (numargs!=6))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) || (!RXVALIDSTRING(args[4])) ||
         ((numargs==6) && (!RXVALIDSTRING(args[5]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Receipent
   // args[2] - RequestNo
   // args[3] - RequestValue
   // args[4] - Buffer (is directly used)
   // args[5] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Receipent     = strtoul(args[1].strptr, NULL, 0) & 0x1F;
   RequestNo     = strtoul(args[2].strptr, NULL, 0);
   RequestValue  = strtoul(args[3].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[5].strptr, NULL, 0);
     else
      TimeOut    = 0;

   LastRC = UsbCtrlMessage (Handle, REQTYPE_XFERDIR_HOSTTODEV|REQTYPE_TYPE_VENDOR|Receipent, RequestNo, RequestValue, 0, args[4].strlength, args[4].strptr, TimeOut);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBBulkRead (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Interface, EndPoint, TimeOut;
   ULONG     Length;
   PCHAR     TempBufferPtr;

   if ((numargs!=4) && (numargs!=5))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) ||
         ((numargs==5) && (!RXVALIDSTRING(args[4]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Interface
   // args[2] - EndPoint
   // args[3] - Length
   // args[4] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Interface     = strtoul(args[1].strptr, NULL, 0);
   EndPoint      = strtoul(args[2].strptr, NULL, 0);
   Length        = strtoul(args[3].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[4].strptr, NULL, 0);
     else
      TimeOut    = 0;

   retstr->strlength = 0;
   TempBufferPtr     = malloc(Length);
   if (TempBufferPtr) {
      LastRC = UsbBulkRead (Handle, EndPoint, Interface, &Length, TempBufferPtr, TimeOut);
      if (!LastRC) {
         retstr->strptr = TempBufferPtr; retstr->strlength = Length;
       } else {
         free (TempBufferPtr);
       }
    }
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY USBBulkWrite (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   USBHANDLE Handle;
   USHORT    Interface, EndPoint, TimeOut;

   if ((numargs!=4) && (numargs!=5))
      if ((!RXVALIDSTRING(args[0])) || (!RXVALIDSTRING(args[1])) || (!RXVALIDSTRING(args[2])) || (!RXVALIDSTRING(args[3])) ||
         ((numargs==5) && (!RXVALIDSTRING(args[4]))))
         return REXX_INVALIDCALL;

   // args[0] - USB-Handle
   // args[1] - Interface
   // args[2] - EndPoint
   // args[3] - Buffer (is directly used)
   // args[4] - Timeout (optional)
   Handle        = strtoul(args[0].strptr, NULL, 16);
   Interface     = strtoul(args[1].strptr, NULL, 0);
   EndPoint      = strtoul(args[2].strptr, NULL, 0);
   if (numargs==5)
      TimeOut    = strtoul(args[4].strptr, NULL, 0);
     else
      TimeOut    = 0;

   LastRC = UsbBulkWrite (Handle, EndPoint, Interface, args[3].strlength, args[3].strptr, TimeOut);
   USBSetRexxResult (retstr, LastRC);
   return REXX_VALIDCALL;
 }
