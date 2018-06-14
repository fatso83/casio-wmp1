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
/* SCCSID = "src/dev/usb/USBD/USBIDC.C, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBIDC.C                                              */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USBD device driver inter-device driver                */
/*                      communication routines.                               */
/*                                                                            */
/*   FUNCTION: These routines handle the PDD-PDD IDC for the                  */
/*             USBD device driver.                                            */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             USBDidc       PDD - PDD IDC Entry Point                        */
/*             USBRegisterHCD                                                 */
/*             USBProcessIRQ                                                  */
/*             USBAcceptIO                                                    */
/*             USBCancelIO                                                    */
/*             USBRegisterClass                                               */
/*             USBSetConfiguration                                            */
/*             USBSetInterface                                                */
/*             ClearStalledPipe                                               */
/*             StartHostCtxHookRtn                                            */         
/*                                                                            */
/*   EXTERNAL REFERENCES:													               */
/*				USBCallIDC                                                        */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark        yy/mm/dd  Programmer  Comment                                 */
/*  ----------  --------  ----------  -------                                 */
/*              98/01/16  MB          Original developer.                     */
/*              98/08/21  MB          Modified USBAcceptIO to accept delay    */
/*                                    time in serviceTime field               */
/*              98/11/07  MB          Added USBCancelIO worker routine        */
/*  07/04/1999  99/04/07  MB          Modified to accept non-default maxpkt   */
/*                                    values for 0 address devices (used to   */
/*                                    fix babble problem during enumeration)  */
/*  31/05/1999  99/05/31  MB          Added floppy boot support routines,     */
/*                                    fixed ClearStalledPipe request flag     */
/*                                    settings.                               */
/*  26/11/1999  99/11/26  MB          Reworked device release procedure on    */
/*                                    repeated registration calls             */
/*  D180501     01/05/18  Dimir       Added time for host controllers reset   */
/*                                    and recovery                            */
/*  LR0612      01/06/12  LR          Fixed previous change (D180501).        */
/*  LR0831      01/08/31  LR          Fixed delayed host start.               */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

#include "usb.h"

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBDidc                                          */
/*                                                                    */
/* DESCRIPTIVE NAME:  PDD-PDD IDC entry point and request router      */
/*                                                                    */
/* FUNCTION:  This routine is the PDD-PDD IDC entry point and         */
/*            request router..  IDC function requests are routed      */
/*            to the appropriate worker routine.  The address of      */
/*            this routine is returned to other device drivers via    */
/*            the DevHelp AttachDD call.                              */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBDidc                                             */
/*    LINKAGE  :  CALL FAR                                            */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS: sets return code in pRP_GENIOCTL->rph>Status              */
/*          USB_IDC_RC_PARMERR - 1) request command is not CMDGenIOCTL*/
/*                               2) parameter packet address is NULL; */
/*          USB_IDC_RC_WRONGFUNC -       Function value specified     */
/*          USB_IDC_RC_WRONGCAT -       Category value specified      */
/*                                                                    */
/* INTERNAL REFERENCES:                                               */
/*    ROUTINES:          USBRegisterHCD                               */
/*                       USBProcessIRQ                                */
/*                       USBAcceptIO                                  */
/*                       USBCancelIO                                  */
/*                       USBRegisterClass                             */
/*                       USBSetConfiguration                          */
/*                       USBSetInterface                              */
/*                       ClearStalledPipe                             */         
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*    ROUTINES:          DevHelp_ArmCtxHook                           */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void FAR USBDidc( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USHORT      status;

   if (!pRP_GENIOCTL)
      return;

#ifdef   DEBUG
   dsPrint3(DBG_HLVLFLOW, "USBD: USBDidc entered: Category=%x, Function=%x, Status=%x\r\n",
            pRP_GENIOCTL->Category,pRP_GENIOCTL->Function,pRP_GENIOCTL->rph.Status);
#endif
   status=pRP_GENIOCTL->rph.Status;
   pRP_GENIOCTL->rph.Status=USB_IDC_RC_OK;
   if (pRP_GENIOCTL->rph.Cmd!=CMDGenIOCTL || !pRP_GENIOCTL->ParmPacket)
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_PARMERR;

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
      switch (pRP_GENIOCTL->Category)
      {
      case USB_IDC_CATEGORY_HOST:
         switch (pRP_GENIOCTL->Function)
         {
         case USB_IDC_FUNCTION_REGISTER:
            USBRegisterHCD( pRP_GENIOCTL );         
            break;
         default:
            pRP_GENIOCTL->rph.Status=USB_IDC_RC_WRONGFUNC;
            break;
         }
         break;
      case USB_IDC_CATEGORY_USBD:
         switch (pRP_GENIOCTL->Function)
         {
         case USB_IDC_FUNCTION_REGISTER:
            USBRegisterClass( pRP_GENIOCTL );
            break;
         case USB_IDC_FUNCTION_SETCONF:
            USBSetConfiguration( pRP_GENIOCTL );
            break;
         case USB_IDC_FUNCTION_SETINTF:
            USBSetInterface( pRP_GENIOCTL );
            break;
         case USB_IDC_FUNCTION_PRCIRQ:
            pRP_GENIOCTL->rph.Status=status;
            USBProcessIRQ( pRP_GENIOCTL );
            break;
         case USB_IDC_FUNCTION_ACCIO:
            USBAcceptIO( pRP_GENIOCTL );         
            break;
         case USB_IDC_FUNCTION_CANCEL:
            USBCancelIO( pRP_GENIOCTL );
            break;
         case USB_IDC_FUNCTION_CLRSTALL:
            ClearStalledPipe( pRP_GENIOCTL );         
            break;
         case USB_IDC_FUNCTION_CMPL_INI:  // 31/05/1999 MB - added delayed host start feature
            if(gDelayHostStart)
            {
//LR0831               gDelayHostStart=FALSE;
               DevHelp_ArmCtxHook( 0, gStartHostHookHandle );
            }
            break;
         default:
            pRP_GENIOCTL->rph.Status=USB_IDC_RC_WRONGFUNC;
            break;
         }
         break;
      default:
         pRP_GENIOCTL->rph.Status=USB_IDC_RC_WRONGCAT;
         break;
      }
   pRP_GENIOCTL->rph.Status|=STATUS_DONE;

#ifdef   DEBUG
   dsPrint3(DBG_HLVLFLOW, "USBD: USBDidc finished: Category=%x, Function=%x, Status=%x\r\n",
            pRP_GENIOCTL->Category,pRP_GENIOCTL->Function,pRP_GENIOCTL->rph.Status);
#endif
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBAcceptIO                                      */
/*                                                                    */
/* DESCRIPTIVE NAME:  Accept I/O PDD-PDD IDC worker routine           */
/*                                                                    */
/* FUNCTION:  This routine is the PDD-PDD IDC worker routine.         */
/*            Request router calls this function to pass I/O          */
/*            request to HCD layer driver. USBAcceptIO verifies       */
/*            device address, locates HCD driver IDC routine and      */
/*            passes request HCD layer driver                         */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBAcceptIO                                         */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_ADDRINV - 1) invalid controller address;      */
/*                                2) invalid (unknown) device address;*/
/*                                3) invalid (unknown) endpoint;      */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:          GetEndpointDPtr                              */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBAcceptIO( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR            *reqData;
   UCHAR                hcIndex;
   UCHAR                savedCategory;
   USHORT               deviceIndex;
   USHORT               maxPacketSize;
   USHORT               retryCount;
   ULONG                eventID=(ULONG)ghDriver;
   USHORT               serviceTime; // 21/08/98 MB
   UCHAR                endPointId;

   reqData=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   hcIndex=reqData->controllerId;
   if (hcIndex>=gNoOfRegisteredHC)
   {
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV; // invalid controller ID
#ifdef   DEBUG
      dsPrint1(DBG_CRITICAL, "USBD: USBAcceptIO: wrong controllerId=%d\r\n", hcIndex);
#endif
   }
   else
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_OK;

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {   //   check address
      if (reqData->deviceAddress!=USB_DEFAULT_DEV_ADDR)
      {
         for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
         {
            if (gDevices[deviceIndex].ctrlID!=hcIndex)
               continue;
            if (gDevices[deviceIndex].deviceAddress==reqData->deviceAddress)
            {
               eventID=(ULONG)(DeviceInfo FAR *)&gDevices[deviceIndex];
               if (gDevices[deviceIndex].lowSpeedDevice)
                  reqData->flags|=USRB_FLAGS_DET_LOWSPEED;
               else
                  reqData->flags&=~USRB_FLAGS_DET_LOWSPEED;
               break;
            }
         }
         if (deviceIndex>=gNoOfDeviceDataEntries)
            pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV; // invalid device address
      }
   }

#ifdef   DEBUG
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
      dsPrint1(DBG_CRITICAL, "USBD: USBAcceptIO: wrong dev address=%d\r\n", reqData->deviceAddress);
#endif
   serviceTime=reqData->serviceTime; // 21/08/98 MB
   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {   //   check endpoint address and set maxpacket and service time
      if (reqData->endPointId!=USB_DEFAULT_CTRL_ENDPT && reqData->deviceAddress!=USB_DEFAULT_DEV_ADDR)
      {
         DeviceEndpoint FAR   *endPointDesc;
         UCHAR                altInterface;  // 27/08/98 MB

         if (reqData->flags&USRB_FLAGS_ALT_INTF)  // 27/08/98 MB
            altInterface=reqData->altInterface;
         else
            altInterface=0;
         endPointId=reqData->endPointId;
         if ((reqData->flags&~USRB_FLAGS_TTYPE_MASK)==USRB_FLAGS_TTYPE_IN)
            endPointId|=DEV_ENDPT_DIRIN;
         endPointDesc=GetEndpointDPtr(gDevices[deviceIndex].configurationData,
                                      gDevices[deviceIndex].descriptor.bNumConfigurations,
                                      gDevices[deviceIndex].bConfigurationValue,
                                      altInterface, endPointId);
         if (endPointDesc)
         {
            reqData->serviceTime=endPointDesc->bInterval;    // Required service frequency in ms
            if (reqData->maxPacketSize==USB_DEFAULT_PKT_SIZE || reqData->maxPacketSize>endPointDesc->wMaxPacketSize)
               reqData->maxPacketSize=endPointDesc->wMaxPacketSize;  // max packet size to be sent at once
         }
         else
         {
            pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV; // invalid endpoint address
         }
      }
      else
      {
         reqData->serviceTime=USB_DEFAULT_SRV_INTV;    // Required service frequency in ms.
         if (reqData->deviceAddress!=USB_DEFAULT_DEV_ADDR)
            maxPacketSize=gDevices[deviceIndex].descriptor.bMaxPacketSize0;
         else
         {  // 07/04/1999 MB - allows to use specific maxpkt size to fix babble
            if(reqData->maxPacketSize==USB_DEFAULT_PKT_SIZE)
               maxPacketSize=USB_STANDARD_PKT_SIZE;
            else
               maxPacketSize=reqData->maxPacketSize;
         }
         if (reqData->maxPacketSize==USB_DEFAULT_PKT_SIZE || reqData->maxPacketSize>maxPacketSize)
            reqData->maxPacketSize=maxPacketSize;  // max packet size to be sent at once
      }
#ifdef   DEBUG
      if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
         dsPrint3(DBG_CRITICAL, "USBD: USBAcceptIO: wrong endpoint address=%d(%x), devAddr=%d\r\n",
                  reqData->endPointId, endPointId, reqData->deviceAddress);
#endif
   }

   if (reqData->maxPacketSize==USB_DEFAULT_PKT_SIZE)
      reqData->maxPacketSize=USB_STANDARD_PKT_SIZE;

   if (reqData->flags&USRB_FLAGS_DET_ISOHR) // 21/08/98 MB
      reqData->serviceTime=serviceTime;

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {
      savedCategory=pRP_GENIOCTL->Category;
      pRP_GENIOCTL->Category=USB_IDC_CATEGORY_HOST;
      // repeat request if host out of bandwidth
      for (retryCount=0; retryCount<HUB_RESET_RETRIES; retryCount++)
      {
         USBCallIDC( gHostControllers[hcIndex].usbIDC,
                     gHostControllers[hcIndex].usbDS, pRP_GENIOCTL );
         if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_NOBANDWIDTH)
         {
            CLISave(); DevHelp_ProcBlock(eventID,HUB_RESET_WAITTIME,(USHORT)0);
         } else
            break;
      }
      pRP_GENIOCTL->Category=savedCategory;
   }
#ifdef   DEBUG
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
      dsPrint1(DBG_DBGSPCL, "USBD: USBAcceptIO: failed =%x\r\n", pRP_GENIOCTL->rph.Status);
#endif
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBCancelIO                                      */
/*                                                                    */
/* DESCRIPTIVE NAME:  Cancel I/O PDD-PDD IDC worker routine           */
/*                                                                    */
/* FUNCTION:  This routine is the PDD-PDD IDC worker routine.         */
/*            Request router calls this function to pass cancel I/O   */
/*            request to HCD layer driver.                            */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBCancelIO                                         */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_ADDRINV -  invalid controller address         */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBCancelIO( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   UCHAR          savedCategory;
   UCHAR          hcIndex;
   USBCancel FAR  *cancelData;

   cancelData=(USBCancel FAR *)pRP_GENIOCTL->ParmPacket;
   hcIndex=cancelData->controllerId;
   if (hcIndex>=gNoOfRegisteredHC)
   {
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV; // invalid controller ID
#ifdef   DEBUG
      dsPrint1(DBG_CRITICAL, "USBD: USBCancelIO: wrong controllerId=%d\r\n", hcIndex);
#endif
   }
   else
   {
      savedCategory=pRP_GENIOCTL->Category;           // save category value
      pRP_GENIOCTL->Category=USB_IDC_CATEGORY_HOST;   // set Host Controller Driver as IDC call target
      USBCallIDC( gHostControllers[hcIndex].usbIDC,
                  gHostControllers[hcIndex].usbDS, pRP_GENIOCTL );
      pRP_GENIOCTL->Category=savedCategory;           // restore category value
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBRegisterHCD                                   */
/*                                                                    */
/* DESCRIPTIVE NAME:  HCD driver registration routine                 */
/*                                                                    */
/* FUNCTION:  This routine registers HCD driver: saves its IDC        */
/*            routine address, DS value and RM adapter handle,        */
/*            assigns controller ID. In case of already registered    */
/*            controller all registered Class Drivers are informed on */
/*            device detach event and all its device and hub entries  */
/*            are released. Routine opens control pipe to read        */
/*            in device description data for device 0 starting        */
/*            USB device enumeration process.                         */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBRegisterHCD                                      */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_EXCEEDSMAX - no of registered host            */
/*                                   controllers exceeds maximum      */
/*                                                                    */
/* INTERNAL REFERENCES:  StartEnumDevice                              */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:          USBCallIDC                                   */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBRegisterHCD( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBHCD FAR   *regData;
   UCHAR        hcIndex, lastIndex;
   RP_GENIOCTL  rp_USBReq;

   regData=(USBHCD FAR *)pRP_GENIOCTL->ParmPacket;


   for (lastIndex=0; lastIndex<gNoOfRegisteredHC; lastIndex++)
   {
      if (gHostControllers[lastIndex].usbIDC==regData->usbIDC)
         break; // HCD driver already registered
   }

   if (lastIndex>=gNoOfRegisteredHC)   // driver not yet registered
   {
      if (gNoOfRegisteredHC+regData->hcdCount>gMaxControllers)
      {
         pRP_GENIOCTL->rph.Status=USB_IDC_RC_EXCEEDSMAX;
         return;
      }
      *(regData->hcdID)=gNoOfRegisteredHC;
      lastIndex=gNoOfRegisteredHC;
      for (hcIndex=0; hcIndex<regData->hcdCount; hcIndex++)
      {
         gHostControllers[gNoOfRegisteredHC].usbIDC=regData->usbIDC;    // Address of Host Controller driver IDC routine
         gHostControllers[gNoOfRegisteredHC].usbDS=regData->usbDS;      // HCD data segment value
         gHostControllers[gNoOfRegisteredHC].ctrlID=gNoOfRegisteredHC;
         gHostControllers[gNoOfRegisteredHC].hDriver=regData->rmData[hcIndex].hDriver;
         gHostControllers[gNoOfRegisteredHC].hAdapter=regData->rmData[hcIndex].hAdapter;
         gNoOfRegisteredHC++;
      }
   }
   else   // release all devices & hubs for already registered HCD
   {  // 07/12/1999 MB - reworked to use existing routines
      for (hcIndex=0; hcIndex<regData->hcdCount; hcIndex++)
      {
         DetachHubPortDevices( (UCHAR)(hcIndex+lastIndex), HUB_NO_HUB_INDEX, 0 );
      }
   }

   if(!gDelayHostStart) // 31/05/1999 - MB - start host if delay not required for boot support
   {
      //  reset all passed to registration host controllers
      for (hcIndex=0; hcIndex<regData->hcdCount; hcIndex++)
      {
         setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
         rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
         rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
         rp_USBReq.Function=USB_IDC_FUNCTION_RSTHOST;
         rp_USBReq.ParmPacket=(PVOID)&rp_USBReq; //  not used, but must be non-zero to pass param checking
         USBCallIDC( gHostControllers[hcIndex+lastIndex].usbIDC,
                     gHostControllers[hcIndex+lastIndex].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );
      }
   
      //  start description reading for device on default address
      for (hcIndex=0; hcIndex<regData->hcdCount; hcIndex++)
      {
         StartEnumDevice( pRP_GENIOCTL, (UCHAR)(hcIndex+lastIndex), FALSE, HUB_NO_HUB_INDEX, 0 );
      }
   }
}


/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBRegisterClass                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Class driver registration routine               */
/*                                                                    */
/* FUNCTION:  This routine is registers Device Class driver - saves   */
/*            driver's IDC routine address and DS value and calls     */
/*            class driver to check all known devices for service.    */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBRegisterClass                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_EXCEEDSMAX - no of registered class drivers   */
/*                                   exceds maximum allowed           */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:          setmem                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBRegisterClass( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBDClass FAR   *regData;
   USBCDServe      classDriverInfo;
   RP_GENIOCTL     rp_USBReq;
   USHORT          deviceIndex, classIndex;

   if (gNoOfRegisteredClassDrvs>=gMaxClassDrivers)
   {  // no space in Class driver registration array
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_EXCEEDSMAX;
      return;
   }
   regData=(USBDClass FAR *)pRP_GENIOCTL->ParmPacket;
   gClassDrivers[gNoOfRegisteredClassDrvs].usbIDC=regData->usbIDC;
   gClassDrivers[gNoOfRegisteredClassDrvs].usbDS=regData->usbDS;
   classIndex=gNoOfRegisteredClassDrvs;
   gNoOfRegisteredClassDrvs++;

   // Call Class Device driver to check service for all known devices
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_CLASS;
   rp_USBReq.Function=USB_IDC_FUNCTION_CHKSERV;
   rp_USBReq.ParmPacket=(PVOID)&classDriverInfo;
   for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
   {
      if (!gDevices[deviceIndex].deviceAddress)  // ignore unused entry
         continue;
      if (gDevices[deviceIndex].descriptor.bDeviceClass==DEV_CLASS_HUB)  // ignore HUB entries
         continue;
      classDriverInfo.pDeviceInfo=(DeviceInfo FAR *)(gDevices+deviceIndex);

      USBCallIDC( gClassDrivers[classIndex].usbIDC,
                  gClassDrivers[classIndex].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );
   }
}


/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBSetConfiguration                              */
/*                                                                    */
/* DESCRIPTIVE NAME:  Set USB configuration routine                   */
/*                                                                    */
/* FUNCTION:  This routine issues "set configuration" I/O request     */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBSetConfiguration                                 */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_ADDRINV - 1)       controller address;        */
/*                                2)       device address.            */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBSetConfiguration( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBSetConf FAR    *setData;
   USBRB             hcdReqBlock;
   RP_GENIOCTL       rp_USBReq;
   USHORT            deviceIndex;

   setData=(USBSetConf FAR *)pRP_GENIOCTL->ParmPacket;

   if (setData->controllerId>=gNoOfRegisteredHC)
   {  // invalid controller ID
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV;
      return;
   }

   for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
   {
      if (gDevices[deviceIndex].ctrlID!=setData->controllerId)
         continue;
      if (gDevices[deviceIndex].deviceAddress==setData->deviceAddress)
         break;
   }
   if (deviceIndex>=gNoOfDeviceDataEntries)
   {  // unknown device address
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV;
      return;
   }

   // fill in request block to set device configuration
   hcdReqBlock.controllerId=gDevices[deviceIndex].ctrlID;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   setData->setConfiguration->bmRequestType=REQTYPE_TYPE_STANDARD;    // standard request
   setData->setConfiguration->bRequest=REQ_SET_CONFIGURATION;         // Set configuration Request
   setData->setConfiguration->wValue=setData->configurationValue;     // configuration value
   setData->setConfiguration->wIndex=0;           // not used
   setData->setConfiguration->wLength=0;          // not used
   hcdReqBlock.buffer1=(PUCHAR)setData->setConfiguration;         // pointer to set configuration setup packet
   hcdReqBlock.buffer1Length=sizeof(*setData->setConfiguration);  // setup packet size
#ifdef   DEBUG
dsPrint1(0, "len=%lx\r\n", (ULONG)hcdReqBlock.buffer1Length);
#endif
   hcdReqBlock.buffer2=NULL;        // not used for this setup request
   hcdReqBlock.buffer2Length=0;     // not used for this setup request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service interval
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // maximum error count
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // USBD IDC routine will be called to process finished requests
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;     // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=MAKEULONG(USBD_IRQ_STATUS_SETDEVCONF,setData->category);  // USBD I/O call type ID - set device configuration
   hcdReqBlock.requestData2=(ULONG)deviceIndex;    // index in device table to current device
   hcdReqBlock.requestData3=MAKEULONG(setData->classDriverDS,setData->irqSwitchValue);// callback information - callers DS and IRQ switch value
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );
   pRP_GENIOCTL->rph.Status=rp_USBReq.rph.Status;
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:   USBSetInterface                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Set device interface routine                    */
/*                                                                    */
/* FUNCTION:  This routine issues "set interface" I/O request         */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBSetInterface                                     */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/a                                                   */
/*                                                                    */
/* EFFECTS:  sets return code in pRP_GENIOCTL->rph.Status             */
/*           USB_IDC_RC_ADDRINV - 1)       controller address;        */
/*                                2)       device address.            */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBSetInterface( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBSetConf FAR    *setData;
   USBRB             hcdReqBlock;
   RP_GENIOCTL       rp_USBReq;
   USHORT            deviceIndex;  

   setData=(USBSetConf FAR *)pRP_GENIOCTL->ParmPacket;

   if (setData->controllerId>=gNoOfRegisteredHC)
   {  // invalid controller ID
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV;
      return;
   }

   for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
   {
      if (gDevices[deviceIndex].ctrlID!=setData->controllerId)
         continue;
      if (gDevices[deviceIndex].deviceAddress==setData->deviceAddress)
         break;
   }
   if (deviceIndex>=gNoOfDeviceDataEntries)
   {  // unknown device address
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV;
      return;
   }

   // fill in request block to set device configuration interface
   hcdReqBlock.controllerId=gDevices[deviceIndex].ctrlID;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   setData->setConfiguration->bmRequestType=REQTYPE_TYPE_STANDARD;   // standard request
   setData->setConfiguration->bRequest=REQ_SET_INTERFACE;            // Set interface Request
   setData->setConfiguration->wValue=0;                              // not used
   setData->setConfiguration->wIndex=setData->configurationValue;    // required configuration index
   setData->setConfiguration->wLength=0;                             // not used
   hcdReqBlock.buffer1=(PUCHAR)setData->setConfiguration;            // pointer to set interface setup packet
   hcdReqBlock.buffer1Length=sizeof(*setData->setConfiguration);     // setup packet size
   hcdReqBlock.buffer2=NULL;        // not used for this request
   hcdReqBlock.buffer2Length=0;     // not used for this request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service interval
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use defualt packet size
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // maximum error retries
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // USBD IDC routine will be called to process finished requests
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=MAKEULONG(USBD_IRQ_STATUS_SETDEVINTF,setData->category);  // USBD I/O call type ID - set interface
   hcdReqBlock.requestData2=(ULONG)deviceIndex;        // index in device table to current device
   hcdReqBlock.requestData3=MAKEULONG(setData->classDriverDS,setData->irqSwitchValue);// callback information - callers DS and IRQ switch value
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );
   pRP_GENIOCTL->rph.Status=rp_USBReq.rph.Status;
}


/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  ClearStalledPipe                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Clears stalled communication pipe               */
/*                                                                    */
/* FUNCTION:  This routine creates I/O request to clear pipe stalled  */
/*            condition.                                              */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  ClearStalledPipe                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  sets error code in pRP_GENIOCTL->rph.Status              */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:                                               */
/*    ROUTINES:          setmem                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void ClearStalledPipe( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB                hcdReqBlock;
   RP_GENIOCTL          rp_USBReq;
   USBClearStalled FAR  *clearRB;

   clearRB=(USBClearStalled FAR *)pRP_GENIOCTL->ParmPacket;

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: ClearStalledPipe: clearing ctrlid=%d,address=%d,endpoint=%d\r\n",
            clearRB->controllerId, clearRB->deviceAddress, clearRB->endPointId);
#endif

   if(clearRB->deviceAddress==USB_DEFAULT_DEV_ADDR)   // 02/17/2000 MB - don't support clear stalled requests
      return;                                         // for default device address (port reset is used instead)

   //  fill in request block to clear stalled pipe
   hcdReqBlock.controllerId=clearRB->controllerId;    // use failed request controller ID
   hcdReqBlock.deviceAddress=clearRB->deviceAddress;  // use failed request device address
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;     // default control pipe is used to clear stalled pipe
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   clearRB->clearStalled->bmRequestType=REQTYPE_XFERDIR_HOSTTODEV|
         REQTYPE_TYPE_STANDARD|REQTYPE_RECIPIENT_ENDPOINT;    // standard request   // 31/05/1999 MB
   clearRB->clearStalled->bRequest=REQ_CLEAR_FEATURE; // "clear stalled" is clear feature request
   clearRB->clearStalled->wValue=ENDPOINT_STALL;      // feature index - 'endpoint stall'
   clearRB->clearStalled->wIndex=clearRB->endPointId; // endpoint to be reset
   clearRB->clearStalled->wLength=0;   // no extra bytes to be transferred for "clear stalled pipe"
   hcdReqBlock.buffer1=(PUCHAR)clearRB->clearStalled; // "clear stalled pipe" packet address
   hcdReqBlock.buffer1Length=sizeof(*clearRB->clearStalled);   // setup packet length
   hcdReqBlock.buffer2=NULL;     // not used for clear stalled pipe request
   hcdReqBlock.buffer2Length=0;  // not used for clear stalled pipe request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // standard service interval
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // and mximum error retry count
   hcdReqBlock.usbIDC=clearRB->clientIDCAddr;      // use stalled request IDC address to process finished requests
   hcdReqBlock.usbDS=clearRB->clientDS;            // stalled request routine DS address
   hcdReqBlock.category=clearRB->category;         // set IRQ processor layer
   hcdReqBlock.requestData1=clearRB->irqSwitchValue;  // set
   hcdReqBlock.requestData2=clearRB->requestData2;    // required
   hcdReqBlock.requestData3=clearRB->requestData3;    // values for request data

   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );
}


/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  StartHostCtxHookRtn                              */
/*                                                                    */
/* DESCRIPTIVE NAME:  Start Host Context Thread Handler               */
/*                                                                    */
/* FUNCTION:  This routine is called from context thread and meets    */
/*            all the kernel requirements on register saving. Starts  */
/*            all the registered USB host controllers and submits     */
/*            get device descriptor requests to the hosts             */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  StartHostCtxHookRtn                                 */
/*    LINKAGE  :  CALL FAR                                            */
/*                                                                    */
/* INPUT:  none                                                       */
/*                                                                    */
/* EXIT-NORMAL:  n/a                                                  */
/*                                                                    */
/* EXIT-ERROR:  n/s                                                   */
/*                                                                    */
/* EFFECTS:  None                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:                                               */
/*    ROUTINES:          USBCallIDC                                   */
/*                       StartEnumDevice                              */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
#pragma optimize("eglt", off)
VOID FAR PASCAL StartHostCtxHookRtn( VOID )
{
   UCHAR        hcIndex;
   RP_GENIOCTL  rp_USBReq;
   USHORT       blkRc = 0;  // D180501
   
   //   save the registers
   _asm
   {
      _emit  66h
      push  si

      _emit  66h
      push  di

      _emit  66h
      push  bx
   }
   if (gDelayHostStart == TRUE)  //LR0831
   {
      gDelayHostStart = FALSE;   //LR0831
      //  reset all registered host controllers
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
      // D180501 start of block
      // D180501 time for reset and for recovery host controllers
      //LR0612   CLI();
      while (blkRc != WAIT_TIMED_OUT) {
         blkRc = DevHelp_ProcBlock((ULONG)ghDriver, 500, WAIT_IS_INTERRUPTABLE);
      //LR0612      CLI();
      }
      // D180501 end of block
   
      // start description reading for device on default address for all registered host controllers
       for (hcIndex=0; hcIndex<gNoOfRegisteredHC; hcIndex++)
      {
         StartEnumDevice( &rp_USBReq, (UCHAR)(hcIndex), FALSE, HUB_NO_HUB_INDEX, 0 );
      }
   }  //LR0831
   // restore the registers
   _asm
   {
      _emit  66h
      pop   bx

      _emit  66h
      pop   di

      _emit  66h
      pop   si
   }
}
#pragma optimize("", on)
