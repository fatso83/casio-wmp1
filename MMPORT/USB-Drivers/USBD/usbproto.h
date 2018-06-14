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
/* SCCSID = "src/dev/usb/USBD/USBPROTO.H, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBPROTO.H                                            */
/*                                                                            */
/*   DESCRIPTIVE NAME:  Function prototypes provided by the USBD              */
/*                      device driver.                                        */
/*                                                                            */
/*   FUNCTION: This module is the USB port device driver                      */
/*             function prototype include file.                               */
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
/*  Mark       yy/mm/dd  Programmer   Comment                                 */
/*  -------    --------  ----------   -------                                 */
/*             96/03/01  MB                                                   */
/*  31/05/1999 99/05/31  MB           Added floppy boot support definitions   */
/*  07/12/1999 99/12/07  MB           Added DetachHubPortDevices definition   */
/*  01/20/2000 00/01/20  MB           Added USBAPMRegister definition         */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

/* USBSTRAT.C */
void FAR USBStrategy();
void CmdError( RPH FAR *pRP);
void USBInitComplete( RPH FAR *pRP);

/* USBDATA.C */
USHORT (*gStratList[])( RPH FAR *pRP );

/* USBIDC.C */
void FAR USBDidc( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBRegisterHCD( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBRegisterClass( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBSetConfiguration( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBSetInterface( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBAcceptIO( RP_GENIOCTL FAR *pRP_GENIOCTL );
void USBCancelIO( RP_GENIOCTL FAR *pRP_GENIOCTL ); // 07/11/98 MB
void ClearStalledPipe( RP_GENIOCTL FAR *pRP_GENIOCTL );
VOID FAR PASCAL StartHostCtxHookRtn( VOID ); // 31/05/1999 MB - added - floppy boot support

/* USBIRQ.C */
void USBProcessIRQ( RP_GENIOCTL FAR *pRP_GENIOCTL );
void ClearUSBDStalledPipe( RP_GENIOCTL FAR *pRP_GENIOCTL );
// Kiewitz - 16.12.2002
void StartEnumDevice (RP_GENIOCTL FAR *pRP_GENIOCTL, UCHAR controllerId, BOOL lowSpeedDevice, USHORT parentHubIndex, UCHAR portNum);
void FailedEnumDevice (RP_GENIOCTL FAR *pRP_GENIOCTL, USHORT deviceIndex);
void EnumGetDescInitCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void EnumSetAddrCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void EnumGetDescCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void EnumGetConfLenCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void EnumGetConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
// Kiewitz-End
void SetConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void GetHConfLenCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void GetHConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL );
void ListenStatusChangedPipe( RP_GENIOCTL FAR *pRP_GENIOCTL );
void HubStatusChanged( RP_GENIOCTL FAR *pRP_GENIOCTL );
void GetPortStatus( RP_GENIOCTL FAR *pRP_GENIOCTL );
void HubPortStatusRetrieved( RP_GENIOCTL FAR *pRP_GENIOCTL );
void DetachHubPortDevices( UCHAR controllerId, USHORT parentHubIndex, UCHAR portNum );  // 07/12/1999 MB - added
void HubAckStatusChanges( RP_GENIOCTL FAR *pRP_GENIOCTL );
void HubSetPortFeature( RP_GENIOCTL FAR *pRP_GENIOCTL, USHORT featureSelector, ULONG irqSwitch );
void PortStatusChanged( RP_GENIOCTL FAR *pRP_GENIOCTL );
void ExtConfSet( RP_GENIOCTL FAR *pRP_GENIOCTL );
void ExtIntfaceSet( RP_GENIOCTL FAR *pRP_GENIOCTL );

/* USBINIT.C */
void USBInit( RPH FAR *pRP );

/* USBAPM.C */
void USBAPMRegister(void);  // 01/20/2000 MB - added


