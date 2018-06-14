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
/* SCCSID = "src/dev/usb/USBD/USBSTRAT.C, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBAPM.C                                              */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USB APM support routines.                             */
/*                                                                            */
/*   FUNCTION: These routines registers APM callback routine and processes    */
/*             APM events.                                                    */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             USBAPMRegister                                                 */
/*             CmdError                                                       */
/*             USBInitComplete                                                */
/*                                                                            */
/*   EXTERNAL REFERENCES:                                                     */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark    yy/mm/dd  Programmer      Comment                                 */
/*  ----    --------  ----------      -------                                 */
/*          00/01/20  MB              Original developer.                     */
/*                                                                            */
/* VC082401 01/08/24  VC              StandBy/Resume Support for Desktop      */
/*                                    PCs added                               */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/
#include        "usb.h"
#include       <apmioctl.h>

// APM structure/data definitions not found in any standard header file
typedef struct _apmRegister
{  // structure used to register APM event handler
   USHORT               Function;
   APMIDC_REGISTER_PKT  Register;
} APMREGISTER, far *PAPMREGISTER, near * NPAMPREGISTER;

#define  APMDDName "APM$    " // APM driver's name


// APM support routine definitions
USHORT NEAR    USBAPMResume( void );         // resume event  handler
USHORT NEAR    USBAPMSuspend(void);          // suspend event handler
// event handler registration routine
USHORT APMRegister(PFN Handler, ULONG NotifyMask, USHORT DeviceID);
// APM event handler routine
USHORT cdecl _loadds far     USBAPMEventHandler (PAPMIDC_NOTIFY_PKT pAPMEvent);


// APM support status flags
BOOL           gAPMRegistered=FALSE;   // TRUE if registration routine was called
BOOL           gSuspendPassed=FALSE;   // TRUE if suspend call passed

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBAPMRegister                                   */
/*                                                                    */
/* DESCRIPTIVE NAME:  USB APM Register routine.                       */
/*                                                                    */
/* FUNCTION:  The function of this routine is to register APM event   */
/*            handler.                                                */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Initialization time                                       */
/*                                                                    */
/* ENTRY POINT:  USBAPMRegister                                       */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  N/A                                                        */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS: USB APM event handler is registered within APM device     */
/*          driver                                                    */
/*                                                                    */
/* INTERNAL REFERENCES:  APMRegister                                  */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
void USBAPMRegister(void)
{
   USHORT   rc;

   if (gAPMRegistered)
      return;

   gAPMRegistered=TRUE;

   rc=APMRegister( (PFN)&USBAPMEventHandler,                              
                   APMMASK_SetPowerState | APMMASK_NormalResume |      
                   APMMASK_CriticalResume 
                   | APMMASK_StandbyResume, // VC082401  Register for StandByResume on Destop PCs                       
                   (USHORT) 0 );                                  
#ifdef DEBUG
   dsPrint1( DBG_HLVLFLOW, "USBD: USBAPMRegister rc=%x\r\n",rc );
#endif
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBAPMEventHandler                               */
/*                                                                    */
/* DESCRIPTIVE NAME:  USB APM Event Handler routine.                  */
/*                                                                    */
/* FUNCTION:  The function of this routine is to process APM events.  */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBAPMEventHandler                                   */
/*    LINKAGE:  CALL FAR                                              */
/*                                                                    */
/* INPUT:  PAPMEVENT pAPMEvent - far pointer to event parameter block */
/*                                                                    */
/* EXIT-NORMAL: 0                                                     */
/*                                                                    */
/* EXIT-ERROR:  non-zero error code                                   */
/*                                                                    */
/* EFFECTS:                                                           */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAPMSuspend                                */
/*                       USBAPMResume                                 */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
USHORT cdecl _loadds far   USBAPMEventHandler (PAPMIDC_NOTIFY_PKT pAPMEvent)
{
   USHORT      rc=0;

#ifdef DEBUG
   dsPrint4( DBG_HLVLFLOW, "USBD: USBAPMEventHandler func %d, ulParm1=%lx, ulParm2=%lx, rc=%d\r\n",
             pAPMEvent->Function, pAPMEvent->SubId, pAPMEvent->PwrState, rc );
#endif
   switch (pAPMEvent->SubId)
   {
   case APMEVENT_SetPowerState:
        if (pAPMEvent->PwrState != APMSTATE_Ready)
           USBAPMSuspend();
      break;

   case APMEVENT_NormalResume:
   case APMEVENT_CriticalResume:
   case APMEVENT_StandbyResume: // VC082401 Resume support for desktop PCs
      rc=USBAPMResume();
      break;
   
   default:
      break;
   }

#ifdef DEBUG
   dsPrint4( DBG_HLVLFLOW, "USBD: USBAPMEventHandler func %d, ulParm1=%lx, ulParm2=%lx, rc=%d\r\n",
             pAPMEvent->Function, pAPMEvent->SubId, pAPMEvent->PwrState, rc );
#endif
   return (rc);
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBAPMSuspend                                    */
/*                                                                    */
/* DESCRIPTIVE NAME:  USB APM Suspend routine.                        */
/*                                                                    */
/* FUNCTION:  The function of this routine is to process suspend      */
/*            events.                                                 */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBAPMSuspend                                        */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  N/A                                                        */
/*                                                                    */
/* EXIT-NORMAL: 0                                                     */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS:                                                           */
/*                                                                    */
/* INTERNAL REFERENCES:  N/A                                          */
/*                                                                    */
/* EXTERNAL REFERENCES:  DetachHubPortDevices                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
USHORT NEAR USBAPMSuspend(void)
{
   UCHAR        hcIndex;

   if (!gDelayHostStart)
   {  // detach all the devices
      for (hcIndex=0; hcIndex<gNoOfRegisteredHC; hcIndex++)
      {
         DetachHubPortDevices( hcIndex, HUB_NO_HUB_INDEX, 0 );
      }
   }

   gSuspendPassed=TRUE;

   return (0);
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBAPMResume                                     */
/*                                                                    */
/* DESCRIPTIVE NAME:  USB APM Resume routine.                         */
/*                                                                    */
/* FUNCTION:  The function of this routine is to process resume       */
/*            events.                                                 */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBAPMResume                                         */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  N/A                                                        */
/*                                                                    */
/* EXIT-NORMAL: 0                                                     */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS:                                                           */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAPMSuspend                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*                       USBCallIDC                                   */
/*                       StartEnumDevice                              */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
USHORT NEAR USBAPMResume(void)
{
   UCHAR        hcIndex;
   RP_GENIOCTL  rp_USBReq;

   if(!gSuspendPassed)  // ensure that all the devices has been detached
      USBAPMSuspend();

   if (!gDelayHostStart)
   {
      //  reset all passed to registration host controllers
      for (hcIndex=0; hcIndex<gNoOfRegisteredHC; hcIndex++)
      {
         setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
         rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
         rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
         rp_USBReq.Function=USB_IDC_FUNCTION_RSTHOST;
         rp_USBReq.ParmPacket=(PVOID)&rp_USBReq; //  not used, but must be non-zero to pass param checking
         USBCallIDC( gHostControllers[hcIndex].usbIDC,
                     gHostControllers[hcIndex].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );
      }

      //  start description reading for device on default address
      for (hcIndex=0; hcIndex<gNoOfRegisteredHC; hcIndex++)
      {
         StartEnumDevice( (RP_GENIOCTL FAR *)&rp_USBReq, (UCHAR)hcIndex, FALSE, HUB_NO_HUB_INDEX, 0 );
      }
   }

   gSuspendPassed=FALSE;

   return (0);
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  APMRegister                                      */
/*                                                                    */
/* DESCRIPTIVE NAME:  USB APM Event Handler registration routine.     */
/*                                                                    */
/* FUNCTION:  The function of this routine is to register             */
/*            specified APM event habdler routine.                    */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBAPMResume                                         */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  FNAPMEVENT FAR *Handler - event handler routine address    */
/*         ULONG NotifyMask - event notification mask                 */
/*         USHORT DeviceID - device ID                                */
/*                                                                    */
/* EXIT-NORMAL: 0 - handler registered                                */
/*                                                                    */
/* EXIT-ERROR:  non-zero - handler is not registered                  */
/*                                                                    */
/* EFFECTS:                                                           */
/*                                                                    */
/* INTERNAL REFERENCES:                                               */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*                       DevHelp_AttachDD                             */
/*                       StartEnumDevice                              */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
#pragma optimize("eglt", off)
USHORT APMRegister(PFN Handler, ULONG NotifyMask, USHORT DeviceID)
{
   APMREGISTER  parm;
   PAPMREGISTER pParm=&parm;
   USHORT   rc=ERROR_PROC_NOT_FOUND;
   USHORT   x;
   USHORT   (cdecl far *  APM_Help)(void);
   
   // fill in registration structure
   setmem((PSZ)&parm,0,sizeof(parm));
   parm.Function=APMIDC_Register;
   parm.Register.EventHandler=Handler;
   parm.Register.NotifyMask=NotifyMask;
   parm.Register.DeviceId=DeviceID;
   parm.Register.ClientDS=GetDS();

   if (!DevHelp_AttachDD( (NPSZ)APMDDName, (NPBYTE)&gDDTable))
   {  // get appropriate APM IDC rtne address
      _asm
      {
           mov x,cs
      }

      if(x & 3)
      {
         SELECTOROF(APM_Help) = gDDTable.Reserved[2];
         OFFSETOF(APM_Help) = 0;
      }
      else
      {
         SELECTOROF(APM_Help) = SELECTOROF(gDDTable.ProtIDCEntry);
         OFFSETOF(APM_Help) = OFFSETOF(gDDTable.ProtIDCEntry);
      }

      if(APM_Help)
      {  // call IDC routine to register event handler
         _asm
         {
            mov      ax,cs
            les      bx,pParm
         }
         rc=(*APM_Help)();
      }
   }

   return (rc);
}
#pragma optimize("", on)
