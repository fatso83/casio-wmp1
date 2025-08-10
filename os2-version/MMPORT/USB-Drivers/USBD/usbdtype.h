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
/* SCCSID = "src/dev/usb/USBD/USBDTYPE.H, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBDTYPE.H                                            */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USBD internal TYPEDEF's and constant include file.    */
/*                                                                            */
/*   FUNCTION: This module defines USBD driver local constants and data       */
/*             types.                                                         */
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
/*  Mark      yy/mm/dd  Programmer    Comment                                 */
/*  -------   --------  ----------    -------                                 */
/*            98/01/31  MB                                                    */
/* 04/11/2000 00/04/11  MB            Added data toggle status variable to    */
/*                                    hub data structure                      */
/* LR0612     01/06/12  LR            Fixed portChangeBitmap length in        */
/*                                    HubInfo data structure.                 */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

//    max USBD strategy command number
#define  MAX_USB_CMD                0x1f

// static array size definition constants
#define  MAX_HOSTCONTROLLERS        4
#define  MAX_CLASS_DRIVERS          16
#define  MAX_DEVICE_ENTRIES         16
#define  MAX_HUB_ENTRIES            16

// initialization time message IDs
#define  INIT_MESSAGE_LOADED        0
#define  INIT_MESSAGE_NO_HCD        1
#define  INIT_MESSAGE_INVNUMERIC    2
#define  INIT_MESSAGE_UNKNOWNKWD    3
#define  INIT_MESSAGE_SETCTXFAILED  4  // 31/05/1999 MB - added - flp boot support

// max no of messages in message queue
#define  MAX_INIT_MESSAGE_COUNT     2

// value used to identify non-existing entry in hub table
#define  HUB_NO_HUB_INDEX           0xffff

// no 10ms intervals allowed to finish port reset
#define  HUB_RESET_RETRIES          10

// no 10ms intervals allowed to finish port reset
#define  HUB_RESET_WAITTIME         20 // 02/17/2000 MB // Dimir time was changed

#define  ROOT_HUB_RESET_WAITTIME    50 // Dimir only for root hubs

#define  SET_ADDRESS_MAX_TIME       52 // Dimir

// no 10ms intervals allowed to finish configuration
#define  HUB_RECOVERY_WAITTIME      10 // Dimir time for reset recovery

// no ProcBlock retries if sleep interrupted
#define  PROCBLOCK_RETRIES          10

// registered host controller data structure
typedef struct _HostController
{
   PUSBIDCEntry   usbIDC;        // Address of Host Controller driver IDC routine
   USHORT         usbDS;         // HCD data segment value
   UCHAR          ctrlID;        // HCD controller ID
   HDRIVER        hDriver;       // HCD driver handle
   HADAPTER       hAdapter;      // HCD adapter handle
}  HostController;

// registered class driver data structure
typedef struct _ClassDriver
{
   PUSBIDCEntry   usbIDC;        // Address of Device Class driver IDC routine
   USHORT         usbDS;         // Class Driver data segment value
}  ClassDriver;

// USB hub device data structure
typedef struct _HubInfo
{
   UCHAR              ctrlID;              // (00) hub's controller ID
   UCHAR              deviceAddress;       // (01) hub's device address
   USHORT             deviceIndex;         // (02) index to device descriptor in device table
   UCHAR              statusPipeID;        // (04) "status changed" pipe address
   UCHAR              portNum;             // (05) port number to be processed
   USHORT             resetRetries;        // (06) current retry count to get port reset
   BOOL               listeningStatusPipe; // (08) "status changed" pipe listening in progress
   BOOL               defAddressInUse;     // (10) true if reading default device descriptor or setting address
   BOOL               defAddressUseDelayed;// (12) true if queued for default address use
   USHORT             poweredOnPorts;      // (14) last powered on port ID
   SetupPacket        getPortStatus;       // (16) setup packet buffer
   PortStatus         portStatus;          // (24) portNum port status words as retrived form device
   PortStatus         hubStatus;           // (28) hub status words
   HubDescriptor      hubDescriptor;       // (32) hub descriptor
   UCHAR              portChangeBitmap[16];//LR0612 [64] "status changed" data buffer
   BOOL               statusPipeToggle;    // interrupt pipe toggle status // 04/11/2000 MB
}  HubInfo;

// IRQ switch constant definitions for USBD local I/O request processing
// Kiewitz - 16.12.2002
#define  USBD_IRQ_STATUS_GETDESCINIT   1 // KIEWITZ: New
#define  USBD_IRQ_STATUS_SETADDR       2
#define  USBD_IRQ_STATUS_GETDESC       3
#define  USBD_IRQ_STATUS_GETCONFLEN    4
#define  USBD_IRQ_STATUS_GETCONF       5
#define  USBD_IRQ_STATUS_SETCONF       6
#define  USBD_IRQ_STATUS_GTHCONFL      7
#define  USBD_IRQ_STATUS_GTHCONF       8
#define  USBD_IRQ_STATUS_CHANGED       9
#define  USBD_IRQ_STATUS_CHGRCVD       10
#define  USBD_IRQ_STATUS_GETPRTST      11
#define  USBD_IRQ_STATUS_PORTCHG       12
// Kiewitz-End
#define  USBD_IRQ_STATUS_SETDEVCONF    32
#define  USBD_IRQ_STATUS_SETDEVINTF    33
