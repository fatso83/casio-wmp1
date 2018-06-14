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
/* SCCSID = "src/dev/usb/USBD/USBEXTRN.H, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBEXTRN.H                                            */
/*                                                                            */
/*   DESCRIPTIVE NAME:  External data declarations for the USBD               */
/*                      device driver.                                        */
/*                                                                            */
/*   FUNCTION: This module is the USBD device driver external                 */
/*             data declarations include file. See usbddata.c for the data    */
/*             items being externalized.                                      */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:  None                                                      */
/*                                                                            */
/*   EXTERNAL REFERENCES:  None                                               */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark       yy/mm/dd  Programmer    Comment                                */
/*  ------     --------  ----------    -------                                */
/*             98/01/31  MB                                                   */
/*  31/05/1999 99/05/31  MB            Added USB floppy boot support varibles */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

extern USHORT        (*gStrat1[])();

extern PFN           Device_Help;
extern ULONG         RMFlags;
extern PFN           RM_Help0;
extern PFN           RM_Help3;

extern DRIVERSTRUCT  gDriverStruct;
extern ADAPTERSTRUCT gAdapterStruct;
extern DEVICESTRUCT  gDeviceStruct[MAX_HOSTCONTROLLERS];

extern HDRIVER       ghDriver;
extern HADAPTER      ghAdapter;

extern CHAR          gOEMDriverName[];
extern IDCTABLE      gDDTable;
extern char          gRootHubDeviceName[];       /* name used for all root hub devices   */
extern char          gHubDeviceName[];           /* name used for all non-root hub devices   */
extern char          gUSBDeviceName[];           /* name used for all non-hub devices   */

extern UCHAR             gMaxControllers;    // maximal number of Host Controller Drivers
extern USHORT            gMaxClassDrivers;   // maximal number of Class Device Drivers
extern USHORT            gMaxDevices;        // maximal number of servicable USB devices
extern USHORT            gMaxHubDevices;     // maximal number of servicable USB Hub devices

extern UCHAR             gNoOfRegisteredHC;
extern HostController    gHostControllers[MAX_HOSTCONTROLLERS];
extern USHORT            gNoOfRegisteredClassDrvs;
extern ClassDriver       gClassDrivers[MAX_CLASS_DRIVERS];
extern USHORT            gNoOfDeviceDataEntries;
extern DeviceInfo        gDevices[MAX_DEVICE_ENTRIES];

// USB floppy boot support
extern ULONG             gStartHostHookHandle; // start host ctx hook handle // 31/05/1999 MB - added to support flp boot
extern BOOL              gDelayHostStart;  // delay host start flag   // 31/05/1999 MB - added to support flp boot

// InitReset - Kiewitz
extern USHORT            gInitResetedDeviceHubIndex[MAX_HOSTCONTROLLERS];
extern UCHAR             gInitResetedDevicePortNum[MAX_HOSTCONTROLLERS];
extern UCHAR             gInitResetedDeviceMaxPacketSize[MAX_HOSTCONTROLLERS];

extern USHORT            gNoOfHubDataEntries;					   //	no of active HUB devices
extern HubInfo           gHubs[MAX_HUB_ENTRIES];               // active hub device data
extern SetupPacket       gGetDescriptor;
extern SetupPacket       gGetDescriptorShort;   //	get device descriptor request block up to maxpacket0 field
extern SetupPacket       gGetConfiguration[MAX_HOSTCONTROLLERS];
extern SetupPacket       gSetConfiguration[MAX_HOSTCONTROLLERS];
extern SetupPacket       gSetAddress[MAX_HOSTCONTROLLERS];
extern SetupPacket       gGetHubConfigurationLength[MAX_HOSTCONTROLLERS];

#ifdef   DEBUG
extern USHORT            gUSBDMsgLevel;   // debug message level
#endif

extern BYTE              gInitDataStart;   /* Marks the end of the data segment. */
extern USHORT            gVerbose;
extern MSGTABLE          gInitMsg;
extern CHAR              gHCDDriverName[9];
extern USHORT            gMessageIDs[MAX_INIT_MESSAGE_COUNT];
extern PSZ               gVMessages[];
extern USHORT            gMessageCount;

