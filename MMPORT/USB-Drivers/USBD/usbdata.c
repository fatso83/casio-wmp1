/*DDK*************************************************************************/
/*                                                                           */
/* COPYRIGHT    Copyright (C) 1992 IBM Corporation                           */
/*                                                                           */
/*    The following IBM OS/2 source code is provided to you solely for       */
/*    the purpose of assisting you in your development of OS/2 device        */
/*    drivers. You may use this code in accordance with the IBM License      */
/*    Agreement provided in the IBM Developer Connection Device Driver       */
/*    Source Kit for OS/2. This Copyright statement may not be removed.      */
/*                                                                           */
/*****************************************************************************/
/* SCCSID = "src/dev/usb/USBD/USBDATA.C, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBDATA.C                                             */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USB device driver data segment                        */
/*                                                                            */
/*   FUNCTION: This module allocates the global data area for the             */
/*             USBD device driver.                                            */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             None                                                           */
/*                                                                            */
/*   EXTERNAL REFERENCES:                                                     */
/*             None                                                           */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark      yy/mm/dd  Programmer    Comment                                 */
/*  ------    --------  ----------    -------                                 */
/*            98/01/31  MB                                                    */
/* 31/05/1999 99/05/31  MB            Added floppy boot support variables     */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

#include "usb.h"

/*--------------------------------------------------------*/
/* Dispatch table for strategy commands (pre FIFO queue)  */
/*--------------------------------------------------------*/
USHORT (*gStratList[])() =
{                     /*--------------------------------------*/
   CmdError,          /* 0x00  initialize device driver       */
   CmdError,          /* 0x01  check the media                */
   CmdError,          /* 0x02  build BPB                      */
   CmdError,          /* 0x03  reserved                       */
   CmdError,          /* 0x04  read                           */
   CmdError,          /* 0x05  non-destructive read           */
   CmdError,          /* 0x06  input status                   */
   CmdError,          /* 0x07  input flush                    */
   CmdError,          /* 0x08  write                          */
   CmdError,          /* 0x09  write with verify              */
   CmdError,          /* 0x0A  get output status              */
   CmdError,          /* 0x0B  flush output                   */
   CmdError,          /* 0x0C  reserved                       */
   CmdError,          /* 0x0D  open                           */
   CmdError,          /* 0x0E  close                          */
   CmdError,          /* 0x0F  removable media                */
   CmdError,          /* 0x10  generic IOCTL                  */
   CmdError,          /* 0x11  reset uncertain media          */
   CmdError,          /* 0x12  get Logical Drive Map          */
   CmdError,          /* 0x13  set Logical Drive Map          */
   CmdError,          /* 0x14  de-Install this device         */
   CmdError,          /* 0x15  reserved                       */
   CmdError,          /* 0x16  get number of partitions       */
   CmdError,          /* 0x17  get unit map                   */
   CmdError,          /* 0x18  no caching read                */
   CmdError,          /* 0x19  no caching write               */
   CmdError,          /* 0x1A  no caching write/verify        */
   USBInit,           /* 0x1B  initialize device driver       */
   CmdError,          /* 0x1C  reserved for Request List code */
   CmdError,          /* 0x1D  get driver capabilities        */
   CmdError,          /* 0x1E  reserved                       */
   USBInitComplete    /* 0x1F  initialization complete        */
};                    /*--------------------------------------*/

/*----------------------------------------------*/
/* GLOBAL VARS FOR RM                           */
/*                                              */
/* RM.LIB needs these declared                  */
/*----------------------------------------------*/
PFN         Device_Help    = NULL;  /* devhelp entry point; used by dhcalls.lib */
ULONG       RMFlags        = 0L;
PFN         RM_Help0       = 0L;
PFN         RM_Help3       = 0L;

/*--------------------------------------------------------*/
/* Set the DRIVERSTRUCT and DETECTEDSTRUCT data areas     */
/*--------------------------------------------------------*/

char  near  gDDName[];                  /* device driver name */
char  near  gDDDesc[];                  /* device driver description */
char  near  gVendorID[];                /* vendor identification */
char  near  gAdapterName[];             /* adapter name */
char  near  gHubDeviceName[];           /* device name template used for all hub devices   */
char  near  gUSBDeviceName[];           /* device name template used for all non-hub devices   */

/*----------------------------------------------*/
/* Driver Description   (rmbase.h)              */
/*----------------------------------------------*/
DRIVERSTRUCT gDriverStruct =
{
   gDDName,                            /* DrvrName                */
   gDDDesc,                            /* DrvrDescript            */
   gVendorID,                          /* VendorName              */
   CMVERSION_MAJOR,                    /* MajorVer                */
   CMVERSION_MINOR,                    /* MinorVer                */
   2001,6,12,                          /* Date             LR0612 */
   DRF_STATIC,                         /* DrvrFlags               */
   DRT_OS2,                            /* DrvrType                */
   DRS_CHAR,                           /* DrvrSubType             */
   NULL                                /* DrvrCallback            */
};

/*----------------------------------------------*/
/* Adapter Description                          */
/*----------------------------------------------*/
ADAPTERSTRUCT gAdapterStruct =
{
   gAdapterName,                       /* AdaptDescriptName; */
   AS_NO16MB_ADDRESS_LIMIT,            /* AdaptFlags;        */
   AS_BASE_INPUT,                      /* BaseType;          */
   AS_SUB_OTHER,                       /* SubType;           */
   AS_BASE_COMM,                       /* InterfaceType;     */
   NULL,                               /* HostBusType;       */
   AS_BUSWIDTH_32BIT,                  /* HostBusWidth;      */
   NULL,                               /* pAdjunctList;      */
   NULL                                /* reserved           */
};

/*----------------------------------------------*/
/* Device Description                           */
/*----------------------------------------------*/
DEVICESTRUCT gDeviceStruct[MAX_HOSTCONTROLLERS] =
{
   gHubDeviceName,                    /* DevDescriptName;   */
   0,                                 /* DevFlags;          */
   DS_TYPE_IO,                        /* DevType;           */
   NULL                               /* pAdjunctList;      */
};

/*----------------------------------------------*/
/* GLOBAL HANDLE VARIABLES                      */
/*                                              */
/* These variables are assigned the handles for */
/* drivers, detected hardware and resources.    */
/*----------------------------------------------*/

HDRIVER           ghDriver     = NULL;                   // global handle to driver
HADAPTER          ghAdapter    = NULL;                   // global handle to RM adapter

IDCTABLE          gDDTable =  { { 0, 0, 0}, 0, 0};       //	structure used by DevHelp AttachDD

UCHAR             gMaxControllers=MAX_HOSTCONTROLLERS;   // maximal number of Host Controller Drivers
USHORT            gMaxClassDrivers=MAX_CLASS_DRIVERS;    // maximal number of Class Device Drivers
USHORT            gMaxDevices=MAX_DEVICE_ENTRIES;        // maximal number of servicable USB devices
USHORT            gMaxHubDevices=MAX_HUB_ENTRIES;        // maximal number of servicable USB Hub devices

UCHAR             gNoOfRegisteredHC=0;                   //	no of registered HCD layer drivers
HostController    gHostControllers[MAX_HOSTCONTROLLERS]; //	HCD layer registration data
USHORT            gNoOfRegisteredClassDrvs=0;            //	no of registered class drivers
ClassDriver       gClassDrivers[MAX_CLASS_DRIVERS];      //	class driver registration data
USHORT            gNoOfDeviceDataEntries=0;              //	no of active devices
DeviceInfo        gDevices[MAX_DEVICE_ENTRIES];          //	active device data

USHORT            gNoOfHubDataEntries=0;                 //	no of active HUB devices
HubInfo           gHubs[MAX_HUB_ENTRIES];                // active hub device data

// USB floppy boot support
ULONG             gStartHostHookHandle=0; // start host ctx hook handle // 31/05/1999 MB - added to support flp boot
BOOL              gDelayHostStart=FALSE;  // delay host start flag   // 31/05/1999 MB - added to support flp boot

// Kiewitz - 16.12.2002
// InitReset (used in StartEnumDevice/GetDescInitCompleted in USBIRQ.c) - Kiewitz
USHORT            gInitResetedDeviceHubIndex[MAX_HOSTCONTROLLERS] = {0};
UCHAR             gInitResetedDevicePortNum[MAX_HOSTCONTROLLERS] = {0};
UCHAR             gInitResetedDeviceMaxPacketSize[MAX_HOSTCONTROLLERS] = {0};
// PortNum is the port-number of the current device that got re-resetted
// MaxPacketSize is the packetsize that this device needs to receive

// request packects used internally
SetupPacket       gGetDescriptor={REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_STANDARD, REQ_GET_DESCRIPTOR,
   MAKEUSHORT(0,DESC_DEVICE), 0, sizeof(DeviceDescriptor)};   //	get device descriptor request block
SetupPacket       gGetDescriptorShort={REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_STANDARD, REQ_GET_DESCRIPTOR, // 31/05/1999 MB
   MAKEUSHORT(0,DESC_DEVICE), 0, DEV_DESC_SIZE_SHORT};   //	get device descriptor request block up to maxpacket0 field
// Kiewitz-End

SetupPacket       gGetConfiguration[MAX_HOSTCONTROLLERS]={REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_STANDARD, REQ_GET_DESCRIPTOR,
   MAKEUSHORT(0,DESC_CONFIGURATION), 0, MAX_CONFIG_LENGTH};   //	get device configuration descriptors request block
SetupPacket       gSetConfiguration[MAX_HOSTCONTROLLERS]={REQTYPE_TYPE_STANDARD, REQ_SET_CONFIGURATION,
   0, 0, 0};  //	get device descriptor request block
SetupPacket       gSetAddress[MAX_HOSTCONTROLLERS]={REQTYPE_TYPE_STANDARD, REQ_SET_ADDRESS,
   0, 0, 0};  //	set address decriptor for each HCD driver
SetupPacket       gGetHubConfigurationLength[MAX_HOSTCONTROLLERS]={REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_STANDARD|REQTYPE_TYPE_CLASS, REQ_GET_DESCRIPTOR,
   MAKEUSHORT(0, DESC_HUB), 0, 1}; //	get device descriptor request block

#ifdef DEBUG
// debug message level
USHORT            gUSBDMsgLevel = DBG_CRITICAL|DBG_HLVLFLOW|DBG_IRQFLOW|DBG_DETAILED|DBG_SPECIFIC;
#endif

BYTE              gInitDataStart = 0;   // Marks the end of the data segment.

USHORT            gVerbose       = 0;   //   initialization time message level

CHAR              gHCDDriverName[9]={0};  

USHORT            gMessageIDs[MAX_INIT_MESSAGE_COUNT]={0};
USHORT            gMessageCount=0;
PSZ               gVMessages[]={ "IUSBD.SYS: USBD driver v.%dd.%dd loaded",
   "EUSBD.SYS: Required HCD layer driver(s) not loaded",
   "EUSBD.SYS: Invalid numeric value in CONFIG.SYS line at column %dddd",
   "EUSBD.SYS: Invalid key value in CONFIG.SYS line at column %dddd",
   "EUSBD.SYS: Failed to allocate CTX hook routine"}; // 31/05/1999 MB

#define MSG_REPLACEMENT_STRING  1178

MSGTABLE  gInitMsg = { MSG_REPLACEMENT_STRING, 1, 0};   //	structure used to write out message during initialization

