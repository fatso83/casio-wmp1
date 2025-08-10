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
/* SCCSID = "src/dev/usb/USBD/USBIRQ.C, usb, c.basedd, currbld 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBIRQ.C                                              */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USB stack IRQ processing extension routines.          */
/*                                                                            */
/*   FUNCTION: These routines handle IRQ processing for locally initiated     */
/*			   I/O requests (to process device attachments/detachments and    */
/*             HUB Class service requests). Processing starts when host       */
/*             controller is reset consists of the following steps:           */
/*             1) USB device descriptor retrieving from device at address 0;  */
/*             2) device address assignment;                                  */
/*             3) reading device configuration length;                        */
/*             4) reading complete configuration data;                        */
/*             5) all non-hub device configuration data are passed to class   */
/*                drivers to continue initialization;                         */
/*             6) hub device configuration data reading;                      */
/*             7) hub is configured to its 1st configuration given in         */
/*                descriptor;                                                 */
/*             8) all hub ports are switched on if necessary;                 */
/*             9) 'status changed' pipe is opened;                            */
/*            10) port status information reading for all ports reported      */
/*                status changed;                                             */
/*            11) detached devices are deleted from USB stack;                */
/*            12) ports with devices attached are reset and enabled;          */
/*                                                                            */
/*   NOTES:                                                                   */
/*             HUB services ensures that only single I/O request  for         */
/*             default device address is active.                              */
/*                                                                            */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             USBProcessIRQ       IRQ processing routine switch              */
/*             ClearUSBDStalledPipe                                           */
/*             GetDescCompleted                                               */
/*             SetAddrCompleted                                               */
/*             GetConfCompleted                                               */
/*             SetConfCompleted                                               */
/*             GetConfLenCompleted                                            */
/*             GetHConfLenCompleted                                           */
/*             GetHConfCompleted                                              */
/*             HubStatusChanged                                               */
/*             HubPortStatusRetrieved                                         */
/*             PortStatusChanged                                              */
/*             ExtConfSet                                                     */
/*             ExtIntfaceSet                                                  */
/*                                                                            */
/*   EXTERNAL REFERENCES:                                                     */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark       yy/mm/dd  Programmer   Comment                                 */
/*  ---------- --------  ----------   -------                                 */
/*             98/01/16  MB           Original developer.                     */
/*             98/08/14  MB           Modified to read in all the             */
/*                                    configuration data                      */
/*  07/04/1999 99/04/07  MB           Fixed device descriptor retrieval       */
/*                                    problem if control endpoint maxpkt size */
/*                                    exceeds 8 bytes (Epson 740).            */
/*  31/05/1999 99/05/31  MB           More fixes for descriptor retrieval     */
/*                                    (Newer Technology floppy), device       */
/*                                    search fixes in ClearUSBDStalledPipe    */
/*  07/12/1999 99/12/07  MB           DetachHubPortDevices made external      */
/*  02/17/2000 00/02/17  MB           Fixed delays before/after port reset,   */
/*                                    added configuration validity check,     */
/*                                    fixed multiple configuration processing */
/*  03/30/2000 00/03/30  MB           Fixed port change bitmap byte size      */
/*                                    calculation.                            */
/*  04/11/2000 00/03/30  MB           Added data toggle for hub interrupt pipe*/
/*                                    operations, fixed Entrega hub attach    */
/*                                    problem.                                */
/*  05/23/2000 00/05/23  MB           Fixed I/O error proceesing in           */
/*                                    StartEnumDevice routine                   */
/*  01/17/2001 01/17/01  MB           Fixed hub attach process fro hubs with  */
/*                                    permanent power on                      */
/*  LR0323     01/03/23  LR           Added USB 1.1 hub descriptor type       */
/*  D032601    01/03/26  Dimir        Added time for reset recovery           */
/*  D032701    01/03/27  Dimir        If all ports are powered at the         */
/*                                    same time power is switched on in HCD   */
/*  D052001    01/05/20  Dimir        Added time for enable recovery          */
/*  D052101    01/05/21  Dimir        Resets from root ports should be        */
/*                                    nominaly 50ms                           */
/*  D052201    01/05/22  Dimir        The device must be able to complete     */
/*                                    the Status stage of the request whithin */
/*                                    50 ms                                   */
/*  LR0612     01/06/12  LR           Fixed Status Change Bitmap setting in   */
/*                                    GetHConfCompleted function.             */
/*  LR0830     01/08/30  LR           Fixed Device descriptor length in       */
/*                                    GetDescCompleted function.              */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

#include "usb.h"

static void SuppressProcessing( ULONG eventId, ULONG waitTime );
static void CreateDeviceName ( PSZ template, PSZ name, UCHAR ctrlId, UCHAR parentHubAddr, UCHAR deviceAddress );
static void FreeDeviceEntry( USHORT deviceIndex );
static void FreeHubEntry( USHORT hubIndex );
static void DetachDevice( UCHAR controllerId, USHORT parentHubIndex, UCHAR portNum );
static void DetachSingle(USHORT deviceIndex, BOOL dontCallClassDrv);

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  USBProcessIRQ                                    */
/*                                                                    */
/* DESCRIPTIVE NAME:  local I/O request IRQ processor                 */
/*                                                                    */
/* FUNCTION:  This routine is processes all completed USBD local      */
/*            device enumeration I/O requests - getting device        */
/*            descriptor, setting device address, reading             */
/*            configuration data. Particularly for HUB devices        */
/*            it reads in hub configuration data, selects hub         */
/*            configuration, reads status changed pipe and processes  */
/*            hub port status changes, starts device attachment/      */
/*            detachment process. Local requests ended with stalled   */
/*            flag set are cleared up by reseting corresponding pipe. */
/*            USBProcessIRQ stops processing for all internal USBD    */
/*            cancelled requests.                                     */
/*                                                                    */
/* NOTES:     USBProcessIRQ is called from HCD layer to finish IRQ    */
/*            processing                                              */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  USBProcessIRQ                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  ClearUSBDStalledPipe                         */
/*    ROUTINES:          GetDescCompleted                             */
/*                       SetAddrCompleted                             */
/*                       GetConfCompleted                             */
/*                       SetConfCompleted                             */
/*                       GetConfLenCompleted                          */
/*                       GetHConfLenCompleted                         */
/*                       GetHConfCompleted                            */
/*                       HubStatusChanged                             */
/*                       HubPortStatusRetrieved                       */
/*                       PortStatusChanged                            */
/*                       ExtConfSet                                   */
/*                       ExtIntfaceSet                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void USBProcessIRQ( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       irqSwitch;
   USHORT       deviceIndex, hubIndex;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   irqSwitch=(USHORT)processedRB->requestData1;

#ifdef   DEBUG
   dsPrint4(DBG_HLVLFLOW, "USBD: USBProcessIRQ Switch %lx, r1=%lx, r2=%lx. Status=%x\r\n",
            processedRB->requestData1,processedRB->requestData2,processedRB->requestData3,pRP_GENIOCTL->rph.Status);
#endif

   if (processedRB->status&USRB_STATUS_STALLED)
      ClearUSBDStalledPipe(pRP_GENIOCTL);

   // discontinue processing for all canceled internal USBD requests
   switch (irqSwitch)
   {
   case USBD_IRQ_STATUS_SETDEVCONF: // external call - 'canceled' status will be processed by worker rtne
   case USBD_IRQ_STATUS_SETDEVINTF: // external call - 'canceled' status will be processed by worker rtne
      break;
   default:
      if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_CANCELED)
         return;  // stop processing for canceled requests
      break;
   }

   // check device & hub indexes
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);
   if (deviceIndex>=gNoOfDeviceDataEntries || (hubIndex>=gNoOfHubDataEntries && hubIndex!=HUB_NO_HUB_INDEX))
      return;

   // call required worker routine
   switch (irqSwitch) {
// Kiewitz - 16.12.2002
   case USBD_IRQ_STATUS_GETDESCINIT:
      EnumGetDescInitCompleted (pRP_GENIOCTL);
      break;
   case USBD_IRQ_STATUS_SETADDR:
      EnumSetAddrCompleted (pRP_GENIOCTL);
      break;
   case USBD_IRQ_STATUS_GETDESC:
      EnumGetDescCompleted (pRP_GENIOCTL);
      break;
   case USBD_IRQ_STATUS_GETCONFLEN:
      EnumGetConfLenCompleted (pRP_GENIOCTL);
      break;
   case USBD_IRQ_STATUS_GETCONF:
      EnumGetConfCompleted( pRP_GENIOCTL );
      break;
// Kiewitz-End
   case USBD_IRQ_STATUS_SETCONF:
      SetConfCompleted( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_GTHCONFL:
      GetHConfLenCompleted( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_GTHCONF:
      GetHConfCompleted( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_CHANGED:
      HubStatusChanged( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_GETPRTST:
      HubPortStatusRetrieved( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_PORTCHG:
      PortStatusChanged( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_SETDEVCONF:
      ExtConfSet( pRP_GENIOCTL );
      break;
   case USBD_IRQ_STATUS_SETDEVINTF:
      ExtIntfaceSet( pRP_GENIOCTL );
      break;
   default:
      break;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  ClearUSBDStalledPipe                             */
/*                                                                    */
/* DESCRIPTIVE NAME:  Clears USBD owned stalled communication pipe    */
/*                                                                    */
/* FUNCTION:  This routine creates I/O request to clear pipe stalled  */
/*            condition.                                              */
/*                                                                    */
/* NOTES:   This routine does not clear stalled condition for device  */
/*          on default address (0).                                   */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  ClearUSBDStalledPipe                                */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  sets error code in pRP_GENIOCTL->rph.Status              */
/*           USB_IDC_RC_ADDRINV - unknown device address (default     */
/*                                address also is treated as unkknown)*/
/*                                                                    */
/* INTERNAL REFERENCES:  ClearStalledPipe                             */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:                                               */
/*    ROUTINES:          setmem                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void ClearUSBDStalledPipe( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR            *processedRB;
   RP_GENIOCTL          rp_USBReq;
   USBClearStalled      clearRB;
   USHORT               deviceIndex;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: ClearUSBDStalledPipe: clearing ctrlid=%d,address=%d,endpoint=%d\r\n",
            processedRB->controllerId, processedRB->deviceAddress, processedRB->endPointId);
#endif

   clearRB.controllerId=processedRB->controllerId;    // controller ID
   clearRB.deviceAddress=processedRB->deviceAddress;  // stalled USB device address
   clearRB.endPointId=processedRB->endPointId;        // stalled endpoint ID
   if(clearRB.endPointId)  // 04/11/2000 MB - non zero endpoints are used only as hub interrupt in endpoints 
      clearRB.endPointId |= DEV_ENDPT_DIRIN;    // in USBD internal data processing
   clearRB.clientIDCAddr=NULL;         // no irq notification for this request
   clearRB.clientDS=0;                 // no irq notification for this request
   clearRB.irqSwitchValue=0;           // no irq notification for this request
   clearRB.requestData2=0;             // no irq notification for this request
   clearRB.requestData3=0;             // no irq notification for this request
   clearRB.category=USB_IDC_CATEGORY_USBD;   // IRQ processor category
   clearRB.clearStalled=NULL;          // clear stalled pipe request packet buffer

   // find out device entry for stalled device and use its reservation for setup packet buffer
   if(processedRB->deviceAddress)   // 31/05/1999 MB - fixed device search problem for default address
   {
      // search by controller ID and device address
      for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
      {
         if (gDevices[deviceIndex].ctrlID!=clearRB.controllerId)
            continue;
         if (gDevices[deviceIndex].deviceAddress==clearRB.deviceAddress)
         {  // set clear  packet buffer address
            clearRB.clearStalled=(SetupPacket FAR *)&gDevices[deviceIndex].clearStalled;
            break;
         }
      }
   }
   else
   {
      // search by default address in use flag in hub table
      for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
      {
         if (gDevices[deviceIndex].ctrlID!=clearRB.controllerId)
            continue;
         if (gDevices[deviceIndex].parentHubIndex<gNoOfHubDataEntries &&
             gHubs[gDevices[deviceIndex].parentHubIndex].defAddressInUse)
         {  // set clear  packet buffer address
            clearRB.clearStalled=(SetupPacket FAR *)&gDevices[deviceIndex].clearStalled;
            break;
         }
      }
   }

   if (!clearRB.clearStalled)  // buffer address not set -> device address unknown
   {
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ADDRINV;
      return;
   }

   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&clearRB;

   ClearStalledPipe( (RP_GENIOCTL FAR *)&rp_USBReq );

   if (rp_USBReq.rph.Status==USB_IDC_RC_OK)
      processedRB->status&=~USRB_STATUS_STALLED;
}

// Kiewitz - 16.12.2002
// Huge changes
// KIEWITZ - KIEWITZ - KIEWITZ
word EnumHelperAcceptIO (USBRB FAR *URB) {
   RP_GENIOCTL URB_IOCTL;

   setmem((PSZ)&URB_IOCTL, 0, sizeof(URB_IOCTL));
   URB_IOCTL.rph.Cmd    = CMDGenIOCTL;
   URB_IOCTL.Category   = USB_IDC_CATEGORY_HOST;
   URB_IOCTL.Function   = USB_IDC_FUNCTION_ACCIO;
   URB_IOCTL.ParmPacket = (PVOID)URB;

   USBAcceptIO ((RP_GENIOCTL FAR *)&URB_IOCTL);
   return URB_IOCTL.rph.Status;
 }

word EnumHelperCancelIO (UCHAR ControllerID, UCHAR DeviceAddress) {
   RP_GENIOCTL rp_USBReq;
   USBCancel   cancelIO;

   // cancel all pending I/O request to device with default  address
   cancelIO.controllerId  = ControllerID;
   cancelIO.deviceAddress = DeviceAddress;
   cancelIO.endPointId    = USBCANCEL_CANCEL_ALL;

   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd    = CMDGenIOCTL;      // IOCTL
   rp_USBReq.Category   = USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function   = USB_IDC_FUNCTION_CANCEL;
   rp_USBReq.ParmPacket = (PVOID)&cancelIO;
   return USBCallIDC (gHostControllers[ControllerID].usbIDC,
               gHostControllers[ControllerID].usbDS,
               (RP_GENIOCTL FAR *)&rp_USBReq);
 }

void EnumHelperInitURB (USBRB FAR *URB, ULONG IOCallType, USHORT DeviceIndex,
                        PUCHAR OutBufferPtr, USHORT OutBufferSize,
                        PUCHAR InBufferPtr, USHORT InBufferSize) {
   USHORT HubIndex = gDevices[DeviceIndex].parentHubIndex;

   URB->controllerId  = gDevices[DeviceIndex].ctrlID;
   URB->deviceAddress = gDevices[DeviceIndex].deviceAddress;
   URB->endPointId    = USB_DEFAULT_CTRL_ENDPT;     // 0, default control endpoint
   URB->status        = 0;        // not used
   URB->flags         = USRB_FLAGS_TTYPE_SETUP;
   if (gDevices[DeviceIndex].lowSpeedDevice)
      URB->flags     |= USRB_FLAGS_DET_LOWSPEED;
   URB->buffer1       = OutBufferPtr;
   URB->buffer1Length = OutBufferSize;
   URB->buffer2       = InBufferPtr;
   URB->buffer2Length = InBufferSize;
   // Required service frequency in ms.
   URB->serviceTime   = USB_DEFAULT_SRV_INTV;
   // Now set maximum packet size from device descriptor. If the descriptor
   //  is not already filled (bMaxPacketSize0==0), we use STANDARD_PKT_SIZE
   if (gDevices[DeviceIndex].descriptor.bMaxPacketSize0) {
      URB->maxPacketSize = gDevices[DeviceIndex].descriptor.bMaxPacketSize0;
    } else {
      URB->maxPacketSize = USB_STANDARD_PKT_SIZE;
    }
   // maximum error count allowed to complete request
   URB->maxErrorCount = USB_MAX_ERROR_COUNT;
   // Set ourselves to receive Callback
   URB->usbIDC        = (PUSBIDCEntry)USBDidc;
   URB->usbDS         = GetDS();
   URB->category      = USB_IDC_CATEGORY_USBD;
   // Save Type of call for IRQ Callback
   URB->requestData1  = IOCallType;
   // Save Device&HubIndex for IRQ Callback
   URB->requestData2  = MAKEULONG(DeviceIndex,HubIndex);
   URB->requestData3  = 0;
 }

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  StartEnumDevice                                  */
/*                                                                    */
/* DESCRIPTIVE NAME:  Device Descriptor retrieving routine            */
/*                                                                    */
/* FUNCTION:  This routine allocates new address and fills in new     */
/*            device entry, cancels all requests to device with       */
/*            default device address, issues I/O request to read in   */
/*            device descriptor data using default device address     */
/*            (address 0).                                            */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  StartEnumDevice                                     */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*         UCHAR controllerId                                         */
/*         BOOL lowSpeedDevice                                        */
/*         USHORT parentHubIndex                                      */
/*         UCHAR portNum                                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  sets error code in pRP_GENIOCTL->rph.Status              */
/*           USB_IDC_RC_ALLOCERR - 1) no space for new device entry;  */
/*                                 2) no new device address available;*/
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:          DefAddrFailed                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:          USBCallIDC                                   */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void StartEnumDevice (RP_GENIOCTL FAR *pRP_GENIOCTL, UCHAR controllerID, BOOL lowSpeedDevice, USHORT parentHubIndex, UCHAR portNum) {
   USBRB       FreshURB;
   UCHAR       devAddress;
   USHORT      deviceIndex;

   #ifdef   DEBUG
      dsPrint2 (DBG_DETAILED, "USBD: [Enum] Started enumeration on device %d:%d\r\n", parentHubIndex, portNum);
   #endif

   // detach all we have linked to this hub port
   DetachHubPortDevices (controllerID, parentHubIndex, portNum);

   // allocate new device address - get smallest unused address for current controller
   for (devAddress=1; devAddress<=USB_MAX_DEVICE_ADDRESS; devAddress++) {
      for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++) {
         if (gDevices[deviceIndex].ctrlID!=controllerID)
            continue;
         if (gDevices[deviceIndex].deviceAddress==devAddress)
            break;
       }
      if (deviceIndex>=gNoOfDeviceDataEntries)
         break;
    }
   if (devAddress>USB_MAX_DEVICE_ADDRESS) {
      // set error code if no more available device addresses
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ALLOCERR;
      #ifdef   DEBUG
         dsPrint (DBG_CRITICAL, "USBD: [Enum] can't allocate new device address\r\n");
      #endif
      return;
    }

   // get free entry in device table
   for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++) {
      if (!gDevices[deviceIndex].deviceAddress)
         break;
    }
   if (deviceIndex+1>=gMaxDevices) {
      // set error code if no space in device table for a new device
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_ALLOCERR;
      #ifdef   DEBUG
         dsPrint (DBG_CRITICAL, "USBD: [Enum] can't allocate new device entry\r\n");
      #endif
      return;
    }
   if (deviceIndex>=gNoOfDeviceDataEntries)
      gNoOfDeviceDataEntries=deviceIndex+1;

   // fill in device table entry
   gDevices[deviceIndex].ctrlID              = controllerID;          // controller ID
   gDevices[deviceIndex].deviceAddress       = devAddress;            // device address
   gDevices[deviceIndex].bConfigurationValue = 0;                  // 0 - not configured
   gDevices[deviceIndex].bInterfaceNumber    = 0;                     // default interface
   gDevices[deviceIndex].lowSpeedDevice      = (UCHAR)lowSpeedDevice; // low speed device flag
   gDevices[deviceIndex].parentHubIndex      = parentHubIndex;        // index to parent hub in hub table
   gDevices[deviceIndex].portNum             = portNum;               // port number device is attached to
   gDevices[deviceIndex].rmDevHandle         = NULL;                  // Resource Manager device handle
   // clear device descriptor
   setmem((PSZ)&gDevices[deviceIndex].descriptor,0, sizeof(gDevices[deviceIndex].descriptor));

   EnumHelperCancelIO (controllerID, USB_DEFAULT_DEV_ADDR);

   #ifdef   DEBUG
      dsPrint2 (DBG_DETAILED, "USBD: [Enum] new device receives address %d, index %d\r\n", devAddress, deviceIndex);
   #endif

   // Check, if we reseted that device during previous Enumeration...
   if ((gInitResetedDeviceMaxPacketSize[controllerID]!=0)
       && (gInitResetedDeviceHubIndex[controllerID]==parentHubIndex)
       && (gInitResetedDevicePortNum[controllerID]==portNum)) {
      // Okay, we are here on 2nd try, directly set MaxPacketSize in our
      //  descriptor and set assigned address now
      gDevices[deviceIndex].descriptor.bMaxPacketSize0 = gInitResetedDeviceMaxPacketSize[controllerID];

      #ifdef   DEBUG
         dsPrint (DBG_DETAILED, "USBD: [Enum] device failed init before, so skip short-descriptor\r\n");
      #endif

      EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_SETADDR, deviceIndex,
                         (PUCHAR)&gSetAddress[controllerID],
                         sizeof(gSetAddress[controllerID]),
                         (PUCHAR)NULL, 0);

      // use default address for unconfigured device
      FreshURB.deviceAddress = USB_DEFAULT_DEV_ADDR;

      // set desired address in setup packet 
      gSetAddress[controllerID].wValue = gDevices[deviceIndex].deviceAddress;

      // clear configuration index and configuration data buffer
      gGetConfiguration[controllerID].wValue = MAKEUSHORT(0,DESC_CONFIGURATION);
      setmem((PSZ)&gDevices[deviceIndex].configurationData,0,
             sizeof(gDevices[deviceIndex].configurationData));

      pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
      if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK) {
         // release device entry and stop processing if failed to send URB
         #ifdef   DEBUG
            dsPrint (DBG_CRITICAL, "USBD: [Enum] Setting device address failed\r\n");
         #endif
         FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
       }
      return;
    }

   // We are first-time in here, so get short descriptor first...
   EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETDESCINIT,
                      deviceIndex,
                      (PUCHAR)&gGetDescriptorShort, sizeof(gGetDescriptorShort),
                      (PUCHAR)&gDevices[deviceIndex].descriptor, DEV_DESC_SIZE_SHORT);

   // use default address for unconfigured device
   FreshURB.deviceAddress = USB_DEFAULT_DEV_ADDR;

   pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK) {
      // release device entry and stop processing if failed to send URB
      #ifdef   DEBUG
         dsPrint (DBG_CRITICAL, "USBD: [Enum] Getting initial device descriptor failed\r\n");
      #endif
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
    }
 }

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  DefAddrFailed                                    */
/*                                                                    */
/* DESCRIPTIVE NAME:  Default Address operation failed                */
/*                                                                    */
/* FUNCTION:  This routine is called to process error situations for  */
/*            operations on default device address to:                */
/*            1) disable port device is attached to;                  */
/*            2) free entry in device table;                          */
/*            3) continue parent hub processing                       */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  DefAddrFailed                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void FailedEnumDevice (RP_GENIOCTL FAR *pRP_GENIOCTL, USHORT deviceIndex) {
   USBRB FAR    *ProcessedURB;
   UCHAR        controllerID;
//   USHORT       deviceIndex, hubIndex;
   USHORT       hubIndex, hubDeviceIndex, status;

   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   hubIndex     = gDevices[deviceIndex].parentHubIndex;
   controllerID = gDevices[deviceIndex].ctrlID;

   status                   = pRP_GENIOCTL->rph.Status;
   pRP_GENIOCTL->rph.Status = USB_IDC_RC_OK;

   // Set gInitReseted variables for next Enum-Process to use correct
   //  DeviceMaxPacketSize
   gInitResetedDeviceHubIndex[controllerID]      = hubIndex;
   gInitResetedDevicePortNum[controllerID]       = gDevices[deviceIndex].portNum;
   gInitResetedDeviceMaxPacketSize[controllerID] = gDevices[deviceIndex].descriptor.bMaxPacketSize0;

   SuppressProcessing ((ULONG)ghDriver, SET_ADDRESS_MAX_TIME);

   // free device entry
   FreeDeviceEntry (deviceIndex);

   // disable hub port
   if (hubIndex!=HUB_NO_HUB_INDEX) {
      hubDeviceIndex             = gHubs[hubIndex].deviceIndex;
      ProcessedURB->requestData2 = MAKEULONG(hubDeviceIndex,hubIndex);
      gHubs[hubIndex].portNum    = gDevices[deviceIndex].portNum;

      // Free default address
      gHubs[hubIndex].defAddressInUse = FALSE;
      #ifdef   DEBUG
         dsPrint2 (DBG_DETAILED, "USBD: Resetting hub %d, port %d\r\n", hubIndex, gDevices[deviceIndex].portNum);
      #endif
      if (!gHubs[hubIndex].deviceAddress)  // hub device detached
         return;

      // reset current port to start enumeration once more
      HubSetPortFeature (pRP_GENIOCTL, PORT_RESET, 0);

      // continue with parent hub status change processing
      pRP_GENIOCTL->rph.Status = USB_IDC_RC_OK;
      // continue hub status change processing
      HubStatusChanged (pRP_GENIOCTL);
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  GetDescInitCompleted                             */
/*                                                                    */
/* DESCRIPTIVE NAME:  Init-Device-Descriptor retrieved Callback       */
/*                                                                    */
/* FUNCTION:  This routine continues device enumeration process:      */
/*            will check, if init device descriptor (first 8 bytes!)  */
/*            was successful and set device address                   */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetDescInitCompleted                                */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:          DefAddrFailed                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/

void EnumGetDescInitCompleted (RP_GENIOCTL FAR *pRP_GENIOCTL) {
   USBRB FAR    *ProcessedURB;
   USBRB        FreshURB;
   UCHAR        controllerID;
   USHORT       deviceIndex, hubIndex;

   // Set some standard variables...
   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   deviceIndex  = LOUSHORT(ProcessedURB->requestData2);
   hubIndex     = HIUSHORT(ProcessedURB->requestData2);
   controllerID = gDevices[deviceIndex].ctrlID;

   // Check, if request failed or Descriptor got less than 8 bytes
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK || !gDevices[deviceIndex].descriptor.bLength ||
       ProcessedURB->buffer2Length < DEV_DESC_SIZE_SHORT) {  
      #ifdef   DEBUG
         dsPrint2 (DBG_CRITICAL, "USBD: [Enum] Failed getting initial device descriptor (status %x, flags %x)\r\n", ProcessedURB->status, ProcessedURB->flags);
      #endif
      // release device entry and stop processing if failed to read device descriptor
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
      return;
    }

   #ifdef   DEBUG
      dsPrint3 (DBG_DETAILED, "USBD: [Enum] Descriptor - Length %d, Type %x, Version %x\r\n", gDevices[deviceIndex].descriptor.bLength, gDevices[deviceIndex].descriptor.bDescriptorType, gDevices[deviceIndex].descriptor.bcdUSB);
      dsPrint3 (DBG_DETAILED, "USBD: [Enum] Descriptor - Class %d, SubClass %d, Protocol %d\r\n", gDevices[deviceIndex].descriptor.bDeviceClass, gDevices[deviceIndex].descriptor.bDeviceSubClass, gDevices[deviceIndex].descriptor.bDeviceProtocol);
      dsPrint1 (DBG_DETAILED, "USBD: [Enum] Descriptor - MaxPacketSize0 %d\r\n", gDevices[deviceIndex].descriptor.bMaxPacketSize0);
   #endif

   EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_SETADDR, deviceIndex,
                      (PUCHAR)&gSetAddress[controllerID],
                      sizeof(gSetAddress[controllerID]),
                      (PUCHAR)NULL, 0);

   // use default address for unconfigured device
   FreshURB.deviceAddress = USB_DEFAULT_DEV_ADDR;

   // set desired address in setup packet 
   gSetAddress[controllerID].wValue = gDevices[deviceIndex].deviceAddress;

   // clear configuration index and configuration data buffer
   gGetConfiguration[controllerID].wValue = MAKEUSHORT(0,DESC_CONFIGURATION);
   setmem((PSZ)&gDevices[deviceIndex].configurationData,0,
          sizeof(gDevices[deviceIndex].configurationData));

   pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK) {
      // release device entry and stop processing if failed to send URB
      #ifdef   DEBUG
         dsPrint (DBG_CRITICAL, "USBD: [Enum] Setting device address failed\r\n");
      #endif
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
    }
 }

void EnumSetAddrCompleted (RP_GENIOCTL FAR *pRP_GENIOCTL) {
   USBRB FAR    *ProcessedURB;
   USBRB        FreshURB;
   UCHAR        controllerID;
   USHORT       deviceIndex, hubIndex;

   USHORT       hubDeviceIndex;

   // Set some standard variables...
   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   deviceIndex  = LOUSHORT(ProcessedURB->requestData2);
   hubIndex     = HIUSHORT(ProcessedURB->requestData2);
   controllerID = gDevices[deviceIndex].ctrlID;

   // Check, if device address setting failed
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK) {
      #ifdef   DEBUG
         dsPrint2 (DBG_CRITICAL, "USBD: [Enum] Failed setting device address (status %x, flags %x)\r\n", ProcessedURB->status, ProcessedURB->flags);
      #endif
      // release device entry and stop processing if failed to read device descriptor
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
      return;
    }

   // delay while address change take place
   SuppressProcessing( (ULONG)(PVOID)ProcessedURB, SET_ADDRESS_MAX_TIME); // D052201

   // device address set - default address could be used for other device
   if (hubIndex!=HUB_NO_HUB_INDEX) { // && !configurationOffset) {
      // continue parent hub status changed processing
      pRP_GENIOCTL->rph.Status        = USB_IDC_RC_OK;
      hubDeviceIndex                  = gHubs[hubIndex].deviceIndex;
      ProcessedURB->requestData2      = MAKEULONG(hubDeviceIndex,hubIndex);
      gHubs[hubIndex].defAddressInUse = FALSE;
      HubStatusChanged( pRP_GENIOCTL );  // continue hub status change processing
    }

   #ifdef   DEBUG
      dsPrint  (DBG_DETAILED, "USBD: [Enum] Getting full device descriptor now...\r\n");
   #endif

   //  fill in request block
   EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETDESC,
                      deviceIndex,
                      (PUCHAR)&gGetDescriptor, sizeof(gGetDescriptor),
                      (PUCHAR)&gDevices[deviceIndex].descriptor, DEV_DESC_SIZE);

   // Send out Request-Block...
   pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK) {
      // release device entry and stop processing if failed to get device descriptor
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
    }
 }


/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  GetDescCompleted                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Full Device Descriptor done                     */
/*                                                                    */
/* FUNCTION:  This routine continues device enumeration process:      */
/*            checks, if full device descriptor retrieved, then       */
/*            gets configuration length.                              */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  SetAddrCompleted                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:          DefAddrFailed                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void EnumGetDescCompleted (RP_GENIOCTL FAR *pRP_GENIOCTL) {
   USBRB FAR    *ProcessedURB;
   USBRB        FreshURB;
   UCHAR        controllerID;
   USHORT       deviceIndex, hubIndex;

   USHORT       DescriptorTry;

   // Set some standard variables...
   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   deviceIndex  = LOUSHORT(ProcessedURB->requestData2);
   hubIndex     = HIUSHORT(ProcessedURB->requestData2);
   controllerID = gDevices[deviceIndex].ctrlID;

   DescriptorTry = LOUSHORT(ProcessedURB->requestData3);

   // Check, if we got full device descriptor...
   if ((pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
       || !gDevices[deviceIndex].descriptor.bNumConfigurations
       || !gDevices[deviceIndex].descriptor.bLength
       || (ProcessedURB->buffer2Length!=DEV_DESC_SIZE)) {
      #ifdef   DEBUG
         dsPrint2 (DBG_CRITICAL, "USBD: [Enum] Failed to get full device descriptor (status %x, flags %x)\r\n", ProcessedURB->status, ProcessedURB->flags);
         dsPrint1 (DBG_CRITICAL, "USBD: [Enum] InBufferLength was %d\r\n", ProcessedURB->buffer2Length);
      #endif

      DescriptorTry++;
      if (DescriptorTry<3) {
         // We try a) DEV_DESC_SIZE and b) STD_PKT_SIZE
         //  cause some hardware is brain-damaged and reports bad maxPacketSize
         #ifdef   DEBUG
            dsPrint (DBG_DETAILED, "USBD: [Enum] Trying again to get full descriptor...\r\n");
         #endif

         //  fill in request block
         EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETDESC,
                            deviceIndex,
                            (PUCHAR)&gGetDescriptor, sizeof(gGetDescriptor),
                            (PUCHAR)&gDevices[deviceIndex].descriptor, DEV_DESC_SIZE);

         if (DescriptorTry==1) {
            FreshURB.maxPacketSize = DEV_DESC_SIZE;
          } else {
            FreshURB.maxPacketSize = USB_STANDARD_PKT_SIZE;
          }

         // Remember Try-Count...
         FreshURB.requestData3 = MAKEULONG(DescriptorTry,0);

         // Send out Request-Block...
         pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
         if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
            return;
       }

      // release device entry and stop processing if failed to read device descriptor
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
      return;
    }

   #ifdef   DEBUG
      dsPrint2 (DBG_DETAILED, "USBD: [Enum] Descriptor - VendorID %x, DeviceID %x\r\n", gDevices[deviceIndex].descriptor.idVendor, gDevices[deviceIndex].descriptor.idProduct);
      dsPrint1 (DBG_DETAILED, "USBD: [Enum] Descriptor - NumConfigurations %d\r\n", gDevices[deviceIndex].descriptor.bNumConfigurations);

      dsPrint (DBG_DETAILED, "USBD: [Enum] Getting configuration 0 length...\r\n");
   #endif

   EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETCONFLEN,
                      deviceIndex,
                      (PUCHAR)&gGetConfiguration[controllerID], sizeof(gGetConfiguration[controllerID]),
                      (PUCHAR)&gDevices[deviceIndex].configurationData, DEV_CONFIG_SIZE_UPTOTLEN);

   // Configuration-Data offset (used to get multiple configurations) is set to
   //  0 by InitURB (saved in requestData3 of URB)
 
   // retrieve only total descriptor length field
   gGetConfiguration[controllerID].wLength = DEV_CONFIG_SIZE_UPTOTLEN;
   // reset getting to configuration 0
   gGetConfiguration[controllerID].wValue  = MAKEUSHORT(0,DESC_CONFIGURATION);
 
   // Send out Request-Block...
   pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
 }

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  GetConfLenCompleted                              */
/*                                                                    */
/* DESCRIPTIVE NAME:  Get Configuration Length completed              */
/*                                                                    */
/* FUNCTION:  This routine continues device enumeration process:      */
/*            checks previous I/O operation return code, starts       */
/*            device descriptor retrieving process.                   */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetConfLenCompleted                                 */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void EnumGetConfLenCompleted (RP_GENIOCTL FAR *pRP_GENIOCTL) {
   USBRB FAR    *ProcessedURB;
   USBRB        FreshURB;
   UCHAR        controllerID;
   USHORT       deviceIndex, hubIndex;

   DeviceConfiguration *devCfg;
   USHORT       CfgOffset;
   UCHAR        curCfgNo;

   // Set some standard variables...
   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   deviceIndex  = LOUSHORT(ProcessedURB->requestData2);
   hubIndex     = HIUSHORT(ProcessedURB->requestData2);
   controllerID = gDevices[deviceIndex].ctrlID;
   // get configuration data offset from USB request block
   CfgOffset    = LOUSHORT(ProcessedURB->requestData3);
   devCfg       = (DeviceConfiguration *)(gDevices[deviceIndex].configurationData)+CfgOffset;
   curCfgNo     = (UCHAR)(LOBYTE(gGetConfiguration[controllerID].wValue));

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK || !devCfg->wTotalLength ||
       (devCfg->wTotalLength+CfgOffset)>MAX_CONFIG_LENGTH) {
      // release device entry if failed to get configuration data or configuration is to long to process
      #ifdef   DEBUG
         dsPrint3 (DBG_CRITICAL, "USBD: [Enum] Failed to get configuration %d length (status %x, flags %x)\r\n", curCfgNo, ProcessedURB->status, ProcessedURB->flags);
      #endif
      // release device entry and stop processing if failed to read device descriptor
//      FreeDeviceEntry( deviceIndex );
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
      return;
    }

   #ifdef   DEBUG
      dsPrint1 (DBG_DETAILED, "USBD: [Enum] Getting configuration %d...\r\n", curCfgNo);
   #endif

   EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETCONF,
                      deviceIndex,
                      (PUCHAR)&gGetConfiguration[controllerID], sizeof(gGetConfiguration[controllerID]),
                      (PUCHAR)&gDevices[deviceIndex].configurationData+CfgOffset, devCfg->wTotalLength);

    // retrieve only total descriptor length field
    gGetConfiguration[controllerID].wLength = devCfg->wTotalLength;
 
    // Send out Request-Block...
    pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
 }

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  GetConfCompleted                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Device configuration retrieving completed       */
/*                                                                    */
/* FUNCTION:  This routine continues device enumeration process       */
/*            by analyzing previous I/O completion code and then      */
/*            proceeds separately for HUB devices and non-hub devices:*/
/*            1) for non-hub device GetConfCompleted calls all the    */
/*               registered Device Class drivers to inform them on    */
/*               new attached device; registers device with RM        */
/*            2) for hub devices GetConfCompleted initiates           */
/*               configuration selection process.                     */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetConfCompleted                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  sets error code in pRP_GENIOCTL->rph.Status              */
/*           USB_IDC_RC_EXCEEDSMAX - set if no more space in hub      */
/*                                   device table                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:          CreateDeviceName                             */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:          USBCallIDC                                   */
/*                       RMCreateDevice                               */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void EnumGetConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL ) {
   USBRB FAR    *ProcessedURB;
   USBRB        FreshURB;
   UCHAR        controllerID;
   USHORT       deviceIndex, hubIndex;

   RP_GENIOCTL  rp_USBReq;
   DeviceConfiguration *devCfg;
   USHORT       CfgOffset, classIndex;
   UCHAR        curCfgNo;

   // Set some standard variables...
   ProcessedURB = (USBRB FAR *)pRP_GENIOCTL->ParmPacket;
   deviceIndex  = LOUSHORT(ProcessedURB->requestData2);
   hubIndex     = HIUSHORT(ProcessedURB->requestData2);
   controllerID = gDevices[deviceIndex].ctrlID;
   // get configuration data offset from USB request block
   CfgOffset    = LOUSHORT(ProcessedURB->requestData3);
   devCfg       = (DeviceConfiguration *)(gDevices[deviceIndex].configurationData)+CfgOffset;
   curCfgNo     = (UCHAR)(LOBYTE(gGetConfiguration[controllerID].wValue));

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK || !devCfg->bLength || !devCfg->wTotalLength ||
       (devCfg->wTotalLength!=ProcessedURB->buffer2Length)) {
      // if request failed, Configuration descriptor bad or length mismatch
      #ifdef   DEBUG
         dsPrint3 (DBG_CRITICAL, "USBD: [Enum] Failed to get configuration %d (status %x, flags %x)\r\n", curCfgNo, ProcessedURB->status, ProcessedURB->flags);
      #endif
      // release device entry and stop processing if failed to read device descriptor
//      FreeDeviceEntry( deviceIndex );
      FailedEnumDevice (pRP_GENIOCTL, deviceIndex);
      return;
    }

   // Read further configurations, if available and if it fits into buffer...
   curCfgNo++;
   if ((curCfgNo<gDevices[deviceIndex].descriptor.bNumConfigurations) &&
       ((CfgOffset+DEV_CONFIG_SIZE_UPTOTLEN)<MAX_CONFIG_LENGTH)) {
      #ifdef   DEBUG
         dsPrint1 (DBG_DETAILED, "USBD: [Enum] Getting configuration %d length...\r\n", curCfgNo);
      #endif

      EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_GETCONFLEN,
                         deviceIndex,
                         (PUCHAR)&gGetConfiguration[controllerID], sizeof(gGetConfiguration[controllerID]),
                         (PUCHAR)&gDevices[deviceIndex].configurationData+CfgOffset, DEV_CONFIG_SIZE_UPTOTLEN);

      // Configuration-Data offset (used to get multiple configurations) is set to
      //  0 by InitURB (saved in requestData3 of URB)
 
      // retrieve only total descriptor length field
      gGetConfiguration[controllerID].wLength = DEV_CONFIG_SIZE_UPTOTLEN;
      // set to next configuration...
      gGetConfiguration[controllerID].wValue  = MAKEUSHORT(curCfgNo,DESC_CONFIGURATION);

      // Update configuration offset...
      FreshURB.requestData3 = MAKEULONG(CfgOffset+devCfg->wTotalLength,0);
 
      // Send out Request-Block...
      pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);
      return;
    }

   // Reset gInitReseted variables, cause we completed Basic Enum process
   gInitResetedDeviceHubIndex[controllerID]      = 0;
   gInitResetedDevicePortNum[controllerID]       = 0;
   gInitResetedDeviceMaxPacketSize[controllerID] = 0;

   devCfg = (DeviceConfiguration *)gDevices[deviceIndex].configurationData;

   if (gDevices[deviceIndex].descriptor.bDeviceClass==DEV_CLASS_HUB) {
      // process HUB device - add new entry in Hub data table  
      for (hubIndex=0; hubIndex<gNoOfHubDataEntries; hubIndex++) {
         if (!gHubs[hubIndex].deviceAddress)
            break;
       }
      if (hubIndex+1>=gMaxHubDevices) {
         // no space in Hub data entry table - set error return code and return
         pRP_GENIOCTL->rph.Status=USB_IDC_RC_EXCEEDSMAX;
         return;
       }
      if (hubIndex>=gNoOfHubDataEntries)
         gNoOfHubDataEntries = hubIndex+1;

      // fill in hub entry
      gHubs[hubIndex].ctrlID                = controllerID;
      gHubs[hubIndex].deviceAddress         = gDevices[deviceIndex].deviceAddress; // hub device address
      gHubs[hubIndex].deviceIndex           = deviceIndex; // index to hub device in device table
      gHubs[hubIndex].listeningStatusPipe   = FALSE; // not listening status change pipe
      gHubs[hubIndex].defAddressInUse       = FALSE; // not using default device address
      gHubs[hubIndex].defAddressUseDelayed  = FALSE; // true if queued for default address use
      gHubs[hubIndex].statusPipeToggle      = FALSE; // 04/11/2000 MB
      #ifdef   DEBUG
         dsPrint1 (DBG_DETAILED, "USBD: [Enum] Hub successfully added (hubIndex %d\r\n", hubIndex);
      #endif

      // Now set configuration for hub class device...
      EnumHelperInitURB (&FreshURB, USBD_IRQ_STATUS_SETCONF,
                         deviceIndex,
                         (PUCHAR)&gSetConfiguration[controllerID], sizeof(gSetConfiguration[controllerID]),
                         NULL, 0);

      // Save our own hubIndex in there, not the parentHubIndex
      FreshURB.requestData2=MAKEULONG(deviceIndex,hubIndex);

      // Set configuration 1...
      gSetConfiguration[controllerID].wValue = devCfg->bConfigurationValue;

      // Send out Request-Block...
      pRP_GENIOCTL->rph.Status = EnumHelperAcceptIO (&FreshURB);

    } else {

      // Call Class Device drivers to finish device setup and register device with RM
      UCHAR        ResourceBuf[ 12 ];
      PAHRESOURCE  pResourceList = (PAHRESOURCE)ResourceBuf;
      USBCDServe   classDriverInfo;
      APIRET       rc;
      UCHAR        deviceName[32];
      UCHAR        parentHubAddr;

      classDriverInfo.pDeviceInfo=(DeviceInfo FAR *)(gDevices+deviceIndex);

      setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
      rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
      rp_USBReq.Category=USB_IDC_CATEGORY_CLASS;
      rp_USBReq.Function=USB_IDC_FUNCTION_CHKSERV;
      rp_USBReq.ParmPacket=(PVOID)&classDriverInfo;
      #ifdef   DEBUG
         if (!(gDevices[deviceIndex].descriptor.idVendor==0x046a &&   // ignore Cherry G80-3000 as kbd
               gDevices[deviceIndex].descriptor.idProduct==0x0001 &&  // to use it
               gDevices[deviceIndex].descriptor.bcdDevice==0x0100))   // only as hub
      #endif
         for (classIndex=0; classIndex<gNoOfRegisteredClassDrvs; classIndex++)
         {
            USBCallIDC( gClassDrivers[classIndex].usbIDC,
                        gClassDrivers[classIndex].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );
         }

      // crete RM device with device address dependent name
      setmem((PSZ)&ResourceBuf, 0, sizeof(ResourceBuf));
      pResourceList->NumResource = 0;
      if (gDevices[deviceIndex].parentHubIndex==HUB_NO_HUB_INDEX)
         parentHubAddr=0;
      else
         parentHubAddr=gHubs[gDevices[deviceIndex].parentHubIndex].deviceAddress;
      CreateDeviceName ( deviceName, gUSBDeviceName, gDevices[deviceIndex].ctrlID,
                         parentHubAddr, gDevices[deviceIndex].deviceAddress );
      gDeviceStruct[controllerID].DevDescriptName=deviceName;

      rc=RMCreateDevice(gHostControllers[controllerID].hDriver,
                        &gDevices[deviceIndex].rmDevHandle,
                        &gDeviceStruct[controllerID],
                        gHostControllers[controllerID].hAdapter,
                        pResourceList);

      #ifdef   DEBUG
         if (rc)
            dsPrint1(DBG_IRQFLOW, "USBD: GetConfCompleted - failed to add device - RM code -%x\r\n", rc);
           else
            dsPrint (DBG_DETAILED, "USBD: [Enum] Device successfully added\r\n");
      #endif
   }
}
// Kiewitz-End

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  CreateDeviceName                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Create device name                              */
/*                                                                    */
/* FUNCTION:  This routine creates device name used in RM calls       */
/*            based on name template, controller ID, hub address and  */
/*            device address.                                         */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   CreateDeviceName                                   */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  PSZ template - name template address                       */
/*         PSZ name - created name                                    */
/*         UCHAR ctrlId - contreller ID                               */
/*         UCHAR parentHubAddr - parent hub adress                    */
/*         UCHAR deviceAddress - device address                       */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  ConvertCharToStr                             */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void CreateDeviceName ( PSZ Template, PSZ name, UCHAR ctrlId, UCHAR parentHubAddr, UCHAR deviceAddress )
{
   for (;*name; Template++, name++)
      *Template=*name;

   ConvertCharToStr( ctrlId, Template );
   while ( *(++Template) );
   *Template='.'; Template++;
   ConvertCharToStr( parentHubAddr, Template );
   while ( *(++Template) );
   *Template='.'; Template++;
   ConvertCharToStr( deviceAddress, Template );
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  SetConfCompleted                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Set HUB device configuration completed          */
/*                                                                    */
/* FUNCTION:  This routine continues HUB device setup process:        */
/*            checks previous I/O return code and initiates hub class */
/*            configuration data retrieving process.                  */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  SetConfCompleted                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void SetConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;
   USHORT       deviceIndex, hubIndex;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device index from address set configuration request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
   {  // failed to set device configuration - release entries in device and hub tables
      FreeDeviceEntry( deviceIndex );
      FreeHubEntry( hubIndex );
#ifdef   DEBUG
      dsPrint3(DBG_CRITICAL, "USBD: SetConfCompleted: - failed to set hub configuration DeviceIndex=%d, HUBIndex=%d,Configuration value=%d\r\n",\
               deviceIndex, hubIndex, gSetConfiguration[processedRB->controllerId].wValue);
#endif
      return;
   }

   // save hub device configuration value
   gDevices[deviceIndex].bConfigurationValue=(UCHAR)gSetConfiguration[processedRB->controllerId].wValue;

   // fill in request block to get hub configuration data length for hub class device
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // 0, default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   setmem((PSZ)&gHubs[hubIndex].hubDescriptor, 0, sizeof(gHubs[hubIndex].hubDescriptor));
   //LR0323begin
   if (gDevices[deviceIndex].descriptor.bcdUSB > 0x0100)
   {  // USB revision 1.1+
      gGetHubConfigurationLength[hcdReqBlock.controllerId].wValue = MAKEUSHORT(0, DESC_HUB11);
   }
   else
   {
      gGetHubConfigurationLength[hcdReqBlock.controllerId].wValue = MAKEUSHORT(0, DESC_HUB);
   }
   //LR0323end
   hcdReqBlock.buffer1=(PUCHAR)&gGetHubConfigurationLength[hcdReqBlock.controllerId];       // pointer to get configuration setup packet
   hcdReqBlock.buffer1Length=sizeof(gGetHubConfigurationLength[hcdReqBlock.controllerId]);  // setup packet size
   hcdReqBlock.buffer2=(PUCHAR)&gHubs[hubIndex].hubDescriptor;       // pointer to configuration buffer
   hcdReqBlock.buffer2Length=gGetHubConfigurationLength[hcdReqBlock.controllerId].wLength;  // configuration buffer size - only to read in descriptor length
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to get hub configuration length
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to get hub configuration length
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to get hub configuration length
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set current (USBD) layer as IRQ processor
   hcdReqBlock.requestData1=USBD_IRQ_STATUS_GTHCONFL; // USBD I/O call type ID - get hub configuration data length
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub indexes
   hcdReqBlock.requestData3=0;   // not used
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
/* SUBROUTINE NAME:  GetHConfLenCompleted                             */
/*                                                                    */
/* DESCRIPTIVE NAME:  Get Hub device configuration completed          */
/*                                                                    */
/* FUNCTION:  This routine continues HUB device setup process:        */
/*            checks previous I/O return code and initiates hub class */
/*            configuration data retrieving process.                  */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetHConfLenCompleted                                */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void GetHConfLenCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;
   USHORT       deviceIndex, hubIndex;
   UCHAR        descriptorLength;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

   descriptorLength=gHubs[hubIndex].hubDescriptor.bLength;
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK && !descriptorLength)
   {  // failed to get hub configuration - release entries in device and hub tables
      FreeDeviceEntry( deviceIndex );
      FreeHubEntry( hubIndex );
#ifdef   DEBUG
      dsPrint(DBG_CRITICAL, "USBD: GetHConfLenCompleted - failed to get hub configuration length\r\n");
#endif
      return;
   }
#ifdef   DEBUG
   dsPrint2(DBG_IRQFLOW, "USBD: GetHConfLenCompleted -  hub configuration length=%d, a=%d\r\n",
            descriptorLength, gDevices[deviceIndex].deviceAddress);
#endif

   // clear number of powered on ports
   gHubs[hubIndex].poweredOnPorts=0;

   // fill in request block to get hub configuration data for hub class device
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   setmem((PSZ)&gHubs[hubIndex].hubDescriptor, 0, sizeof(gHubs[hubIndex].hubDescriptor));
   gHubs[hubIndex].getPortStatus.bmRequestType=REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_CLASS; // Characteristics of request
   gHubs[hubIndex].getPortStatus.bRequest=REQ_GET_DESCRIPTOR; // get hub descriptor request
   //LR0323begin
   if (gDevices[deviceIndex].descriptor.bcdUSB > 0x0100)
   {  // USB revision 1.1+
      gHubs[hubIndex].getPortStatus.wValue = MAKEUSHORT(0, DESC_HUB11);
   }
   else
   {
      gHubs[hubIndex].getPortStatus.wValue = MAKEUSHORT(0, DESC_HUB);
   }
   //LR0323end
   gHubs[hubIndex].getPortStatus.wIndex=0;
   gHubs[hubIndex].getPortStatus.wLength=descriptorLength;
   hcdReqBlock.buffer1=(PUCHAR)&gHubs[hubIndex].getPortStatus;       // pointer to get status setup packet
   hcdReqBlock.buffer1Length=sizeof(gHubs[hubIndex].getPortStatus);  // setup packet size
   hcdReqBlock.buffer2=(PUCHAR)&gHubs[hubIndex].hubDescriptor;       // pointer to configuration buffer
   hcdReqBlock.buffer2Length=descriptorLength;  // configuration descriptor buffer size size
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to get hub configuration length
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to get hub configuration length
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to get hub configuration length
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=USBD_IRQ_STATUS_GTHCONF;  // USBD I/O call type ID - get hub configuration data
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub indexes
   hcdReqBlock.requestData3=0;           // not used
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
/* SUBROUTINE NAME:  GetHConfCompleted                                */
/*                                                                    */
/* DESCRIPTIVE NAME:  Retrieving Hub configuration completed          */
/*                                                                    */
/* FUNCTION:  This routine finishes Hub configuration:                */
/*            checks previous I/O return code, powers on hub ports,   */
/*            registers new Hub as RM device and starts listening     */
/*            'Status Changed' pipe.                                  */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetHConfCompleted                                   */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  ListenStatusChangedPipe                      */
/*    ROUTINES:          HubSetPortFeature                            */
/*                       CreateDeviceName                             */
/*                       SuppressProcessing                           */
/*                                                                    */
/* EXTERNAL REFERENCES:  RMCreateDevice                               */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/

void GetHConfCompleted( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex;
   APIRET       rc;
   UCHAR        ResourceBuf[12];
   UCHAR        deviceName[32];
   PSZ          nameTemplate;
   PAHRESOURCE  pResourceList = (PAHRESOURCE)ResourceBuf;
   struct   HubConfiguration
   {
      DeviceConfiguration  configuration;
      DeviceInterface      interface;
      DeviceEndpoint       endPoint[1];
   }           *hubConfiguration;
   UCHAR       endPointIndex;
   UCHAR       parentHubAddr;
   BOOL        powerOnIntExp; // 01/17/2001 MB

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK && !gHubs[hubIndex].hubDescriptor.bLength)
   {  // failed to get hub configuration - release entries in device and hub tables
      FreeDeviceEntry( deviceIndex );
      FreeHubEntry( hubIndex );
#ifdef   DEBUG
      dsPrint(DBG_CRITICAL, "USBD: GetHConfCompleted - failed to retrieve hub configuration\r\n");
#endif
      return;
   }

#ifdef   DEBUG
   dsPrint2(DBG_DETAILED, "USBD: GetHConfCompleted - numports %d, poweredon=%d\r\n",
            gHubs[hubIndex].hubDescriptor.bNbrPorts, gHubs[hubIndex].poweredOnPorts);
#endif
   // switch on all the hub ports
   if (gHubs[hubIndex].poweredOnPorts<gHubs[hubIndex].hubDescriptor.bNbrPorts)
   {
      switch (gHubs[hubIndex].hubDescriptor.wHubCharacteristics & HUB_DESC_CHAR_POWER_MASK)
      {
      // D032701  case HUB_DESC_CHAR_POWER_GANGED:
      case HUB_DESC_CHAR_POWER_INDIV:
         gHubs[hubIndex].poweredOnPorts++;
         gHubs[hubIndex].portNum=(UCHAR)gHubs[hubIndex].poweredOnPorts;
         HubSetPortFeature( pRP_GENIOCTL, PORT_POWER, USBD_IRQ_STATUS_GTHCONF );
         return;
      case HUB_DESC_CHAR_POWER_GANGED:                                                         // D032701
         if (gDevices[deviceIndex].parentHubIndex == HUB_NO_HUB_INDEX) {                       // D032701
            gHubs[hubIndex].poweredOnPorts=gHubs[hubIndex].hubDescriptor.bNbrPorts;            // D032701
         } else if (gHubs[hubIndex].poweredOnPorts < gHubs[hubIndex].hubDescriptor.bNbrPorts) {// D032701
            gHubs[hubIndex].poweredOnPorts++;                                                  // D032701
            gHubs[hubIndex].portNum=(UCHAR)gHubs[hubIndex].poweredOnPorts;                     // D032701
            HubSetPortFeature( pRP_GENIOCTL, PORT_POWER, USBD_IRQ_STATUS_GTHCONF );            // D032701
            return;                                                                            // D032701
         }                                                                                     // D032701
         break;                                                                                // D032701
      default: // all ports are powered on when hub is powered on
         gHubs[hubIndex].poweredOnPorts=gHubs[hubIndex].hubDescriptor.bNbrPorts;
         break;
      }
   }

   if (gHubs[hubIndex].poweredOnPorts==gHubs[hubIndex].hubDescriptor.bNbrPorts)
   {  // delay processing to stabilize power supply
      ULONG    powerOnDelay;

      powerOnDelay=USB_POWER_ON_DELAYT2+gHubs[hubIndex].hubDescriptor.bPwrOn2PwrGood*2;
      gHubs[hubIndex].poweredOnPorts++;
#ifdef   DEBUG
      dsPrint1(DBG_DETAILED, "USBD: GetHConfCompleted: delaying for %ld ms\r\n",
               powerOnDelay);
#endif
      SuppressProcessing( (ULONG)(HubInfo FAR *)&gHubs[hubIndex], powerOnDelay );
   }

   // get hub 'Status Changed pipe' address
   hubConfiguration=(struct HubConfiguration *)gDevices[deviceIndex].configurationData;
   for (endPointIndex=0; endPointIndex<hubConfiguration->interface.bNumEndpoints; endPointIndex++)
   {
      if ((hubConfiguration->endPoint[endPointIndex].bmAttributes&DEV_ENDPT_ATTRMASK)!=DEV_ENDPT_INTRPT)
         continue;
      gHubs[hubIndex].statusPipeID=(UCHAR)(hubConfiguration->endPoint[endPointIndex].bEndpointAddress&DEV_ENDPT_ADDRMASK);
      break;
   }

   // build new device name to be used by RM
   pResourceList->NumResource = 0;
   if (gDevices[deviceIndex].deviceAddress==1)
      nameTemplate=gRootHubDeviceName;
   else
      nameTemplate=gHubDeviceName;
   if (gDevices[deviceIndex].parentHubIndex==HUB_NO_HUB_INDEX)
      parentHubAddr=0;
   else
      parentHubAddr=gHubs[gDevices[deviceIndex].parentHubIndex].deviceAddress;

   CreateDeviceName ( deviceName, nameTemplate, gDevices[deviceIndex].ctrlID,
                      parentHubAddr, gDevices[deviceIndex].deviceAddress );
   gDeviceStruct[processedRB->controllerId].DevDescriptName=deviceName;

   rc=RMCreateDevice(ghDriver,
                     &gDevices[deviceIndex].rmDevHandle,
                     gDeviceStruct+processedRB->controllerId,
                     ghAdapter,
                     pResourceList);

#ifdef   DEBUG
   if (rc)
      dsPrint1(DBG_CRITICAL, "USBD: GetHConfCompleted - failed to add hub device - RM code - %x\r\n", rc);
#endif

   // 01/17/2001 MB - added to process hubs with permanent power on - no POWER ON status change is reported
   //                and no data is sent to status change pipe
   switch (gHubs[hubIndex].hubDescriptor.wHubCharacteristics&HUB_DESC_CHAR_POWER_MASK)
   {
   // D032701 case HUB_DESC_CHAR_POWER_GANGED:
   case HUB_DESC_CHAR_POWER_INDIV:
      powerOnIntExp=TRUE;
      break;
   case HUB_DESC_CHAR_POWER_GANGED:                                     // D032701
      if (gDevices[deviceIndex].parentHubIndex == HUB_NO_HUB_INDEX)     // D032701
         powerOnIntExp=FALSE;                                           // D032701
      else                                                              // D032701
         powerOnIntExp=TRUE;                                            // D032701
      break;                                                            // D032701
   default: // all ports are powered on when hub is powered on
      powerOnIntExp=FALSE;
      break;
   }

   if(powerOnIntExp)
      ListenStatusChangedPipe( pRP_GENIOCTL );
   else
   {
      UCHAR        byteCountInStatusBitmap;
   
      // set status changed flags to '1' to force status change processing for all ports
      byteCountInStatusBitmap=(UCHAR)((UCHAR)((gHubs[hubIndex].hubDescriptor.bNbrPorts+7)>>3));
      //LR0612begin
      if (byteCountInStatusBitmap > 1)
      {
         setmem ((PSZ)&gHubs[hubIndex].portChangeBitmap, -1, byteCountInStatusBitmap);         
      }
      gHubs[hubIndex].portChangeBitmap[byteCountInStatusBitmap-1] = (byteCountInStatusBitmap-1)?
         (UCHAR)((1 << (gHubs[hubIndex].hubDescriptor.bNbrPorts + 1) % sizeof(UCHAR)) - 1):
         (UCHAR)((1 <<  gHubs[hubIndex].hubDescriptor.bNbrPorts + 1) - 1);
      gHubs[hubIndex].portChangeBitmap[0] ^= 1;
      //LR0612end
      HubStatusChanged( pRP_GENIOCTL );
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  SuppressProcessing                               */
/*                                                                    */
/* DESCRIPTIVE NAME:  Suppresses thread execution                     */
/*                                                                    */
/* FUNCTION:  This routine suppresses thread execution for speciefied */
/*            time period.                                            */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  SuppressProcessing                                  */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  ULONG eventId - value used as identifier                   */
/*         ULONG waitTime - required wait time in ms                  */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  DevHelp_ProcBlock                            */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void SuppressProcessing( ULONG eventId, ULONG waitTime )
{
   USHORT   rc, repeatCount;

   for (repeatCount=0; repeatCount<PROCBLOCK_RETRIES; repeatCount++)
   {
      CLISave();
      rc=DevHelp_ProcBlock(eventId,waitTime,WAIT_IS_INTERRUPTABLE);
      if (rc==WAIT_TIMED_OUT)
         break;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  ListenStatusChangedPipe                          */
/*                                                                    */
/* DESCRIPTIVE NAME:  Starts listening in Status Changed pipe         */
/*                                                                    */
/* FUNCTION:  This routine starts listening 'Status Changed' pipe.    */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  ListenStatusChangedPipe                             */
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
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void ListenStatusChangedPipe( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;
   UCHAR        byteCountInStatusBitmap;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

   if (gHubs[hubIndex].listeningStatusPipe)   // pipe already opened
      return;

   byteCountInStatusBitmap=(UCHAR)((UCHAR)((gHubs[hubIndex].hubDescriptor.bNbrPorts+7)>>3)); // 03/30/2000 MB
   setmem((PSZ)&gHubs[hubIndex].portChangeBitmap, 0, byteCountInStatusBitmap);

   // fill in request block to start listening to 'Status Changed' pipe
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gHubs[hubIndex].deviceAddress;
   hcdReqBlock.endPointId=gHubs[hubIndex].statusPipeID;    // use hub status changed interrupt pipe ID
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_IN|USRB_FLAGS_DET_INTRPT;
   if(gHubs[hubIndex].statusPipeToggle)   // 04/11/2000 MB
      hcdReqBlock.flags|=USRB_FLAGS_DET_DTGGLEON;
   hcdReqBlock.buffer1=(PUCHAR)&gHubs[hubIndex].portChangeBitmap;       // address of hub status changed buffer
   hcdReqBlock.buffer1Length=byteCountInStatusBitmap;  // status changed buffer length
   hcdReqBlock.buffer2=NULL;     // no additonal data to be sent to/from host
   hcdReqBlock.buffer2Length=0;  // to complete this request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to read "status changed" data
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to read "status changed" data
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to read "status changed" data
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=USBD_IRQ_STATUS_CHANGED;  // USBD I/O call type ID - read hub status changed data
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub indexes
   hcdReqBlock.requestData3=0;           // index in hub device table to current hub
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

#ifdef   DEBUG
   dsPrint4(DBG_IRQFLOW, "USBD: ListenStatusChangedPipe - devindex=%d,hubIndex=%d,bmaplength=%d, toggle %d\r\n",
            deviceIndex,hubIndex,byteCountInStatusBitmap, gHubs[hubIndex].statusPipeToggle);
#endif
   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );
   pRP_GENIOCTL->rph.Status=rp_USBReq.rph.Status;

   gHubs[hubIndex].listeningStatusPipe= (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK);


#ifdef   DEBUG
   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
      dsPrint4(DBG_CRITICAL, "USBD: ListenStatusChangedPipe - failed devInd=%d,hInd=%d,bLen=%d, Status=%x\r\n",
               deviceIndex,hubIndex,byteCountInStatusBitmap,pRP_GENIOCTL->rph.Status);
#endif
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  HubStatusChanged                                 */
/*                                                                    */
/* DESCRIPTIVE NAME:  Hub device port status changed                  */
/*                                                                    */
/* FUNCTION:  This routine is called when hub has detected status     */
/*            change in its downstream ports. Port status reading     */
/*            is initiated for a single per call port with reported   */
/*            changes. If all the ports has been processed it reopens */
/*            Status Changed pipe.                                    */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  HubStatusChanged                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  ListenStatusChangedPipe                      */
/*    ROUTINES:          StartEnumDevice                              */
/*                       GetPortStatus                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/

void HubStatusChanged( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex;
   UCHAR        byteCountInStatusBitmap, bitmapStatusIndex, portNum, bitMap;
   UCHAR        bitPos, bitMask, ctrlID;
   BOOL         lowSpeedDevice;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   //  get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

#ifdef   DEBUG
   dsPrint4(DBG_IRQFLOW, "USBD: HubStatusChanged entered. lstatus=%d,dindx=%d,hubind=%d,cbtmap=%x\r\n",
            gHubs[hubIndex].listeningStatusPipe,deviceIndex,hubIndex,
            gHubs[hubIndex].portChangeBitmap[0]);
#endif

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {
      gHubs[hubIndex].statusPipeToggle=(processedRB->flags&USRB_FLAGS_DET_DTGGLEON)!=0;   // 04/11/2000 MB
      byteCountInStatusBitmap=(UCHAR)((UCHAR)((gHubs[hubIndex].hubDescriptor.bNbrPorts+7)>>3)); // 03/30/2000 MB

      // Get port no to process
      portNum=0;
      for (bitmapStatusIndex=0; bitmapStatusIndex<byteCountInStatusBitmap; bitmapStatusIndex++)
      {
         bitMap=gHubs[hubIndex].portChangeBitmap[bitmapStatusIndex];
         bitMask=1;
         for (bitPos=0; bitPos<8; bitPos++)
         {
            if (bitMap&bitMask)
               break;
            portNum++;
            bitMask= (UCHAR)(bitMask<<1);
         }
         gHubs[hubIndex].portChangeBitmap[bitmapStatusIndex]=(UCHAR)(bitMap & ~bitMask);  // clear port status changed flag
         if (bitPos<8)
            break;
      }
      if (bitmapStatusIndex>=byteCountInStatusBitmap)
         portNum=0;
   }
   else
   {
      portNum=0;
      gHubs[hubIndex].statusPipeToggle=FALSE;   // 04/11/2000 MB
#ifdef   DEBUG
      dsPrint2(DBG_CRITICAL, "USBD: HubStatusChanged failed. devindex=%d,hubind=%d\r\n",deviceIndex,hubIndex);
#endif
   }
   gHubs[hubIndex].portNum=portNum;
   gHubs[hubIndex].listeningStatusPipe=FALSE;

   if (!portNum)
   {
      if (hubIndex!=HUB_NO_HUB_INDEX && gHubs[hubIndex].deviceAddress &&
          pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
      {
         processedRB->requestData2=MAKEULONG(deviceIndex,hubIndex);
         processedRB->requestData3=0;
         ListenStatusChangedPipe( pRP_GENIOCTL );  // re-create "Status Changed" pipe
      }

      ctrlID=gHubs[hubIndex].ctrlID;
      // find out unprocessed hub ports with changed status
      for (hubIndex=0; hubIndex<gNoOfHubDataEntries; hubIndex++)
      {
         if (!gHubs[hubIndex].deviceAddress)
            continue;
         if (gHubs[hubIndex].ctrlID!=ctrlID)
            continue;
         if (!gHubs[hubIndex].listeningStatusPipe && gHubs[hubIndex].portNum && gHubs[hubIndex].defAddressUseDelayed)
            break;
      }
      if (hubIndex<gNoOfHubDataEntries)
      {  //  process device attached to this port
         gHubs[hubIndex].defAddressInUse=TRUE;
         gHubs[hubIndex].defAddressUseDelayed=FALSE;
         lowSpeedDevice=(gHubs[hubIndex].portStatus.wPortStatus&STATUS_PORT_LOW_SPEED)!=0;
         StartEnumDevice( pRP_GENIOCTL, processedRB->controllerId, lowSpeedDevice,
                        hubIndex, gHubs[hubIndex].portNum );
      }
      return;
   }

   gHubs[hubIndex].resetRetries=0;     // reset completed retry count

   GetPortStatus( pRP_GENIOCTL ); // read port status
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  GetPortStatus                                    */
/*                                                                    */
/* DESCRIPTIVE NAME:  Get port status routine                         */
/*                                                                    */
/* FUNCTION:  This routine is called to read port status and status   */
/*            change data.                                            */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  GetPortStatus                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void GetPortStatus( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   //  get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: GetPortStatus entered. dindx=%d,hubind=%d,Port=%d\r\n",
            deviceIndex,hubIndex,gHubs[hubIndex].portNum);
#endif

   // fill in request block to read port status information
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   // fill in hub class get status block
   gHubs[hubIndex].getPortStatus.bmRequestType=REQTYPE_XFERDIR_DEVTOHOST|REQTYPE_TYPE_CLASS|REQTYPE_TYPE_HUBCLASS_PORT;    // Characteristics of request
   gHubs[hubIndex].getPortStatus.bRequest=REQ_GET_STATUS;         // get status request
   gHubs[hubIndex].getPortStatus.wValue=0;                        // not used for hub request
   gHubs[hubIndex].getPortStatus.wIndex=gHubs[hubIndex].portNum;  // port of interest
   gHubs[hubIndex].getPortStatus.wLength=sizeof(gHubs[hubIndex].portStatus);  // status block size
   hcdReqBlock.buffer1=(PUCHAR)&gHubs[hubIndex].getPortStatus;       // pointer to get status setup packet
   hcdReqBlock.buffer1Length=sizeof(gHubs[hubIndex].getPortStatus);  // setup packet size
   hcdReqBlock.buffer2=(PUCHAR)&gHubs[hubIndex].portStatus;          // status buffer address
   hcdReqBlock.buffer2Length=sizeof(gHubs[hubIndex].portStatus);     // status buffer length
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to get port status
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to get port status
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to get port status
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=USBD_IRQ_STATUS_GETPRTST; // USBD I/O call type ID - read port status information
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub indexes
   hcdReqBlock.requestData3=0;                        // not used
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
/* SUBROUTINE NAME:  HubPortStatusRetrieved                           */
/*                                                                    */
/* DESCRIPTIVE NAME:  Hub port status information retrieved           */
/*                                                                    */
/* FUNCTION:  In case of error in status retrieving routine calls     */
/*            HubStatusChanged to continue processing of other hub    */
/*            ports. Othervise all changed properties are             */
/*            acknowledged and routine starts processing both device  */
/*            connected/disconnected port states.                     */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :  HubPortStatusRetrieved                              */
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
/* INTERNAL REFERENCES:  HubStatusChanged                             */
/*    ROUTINES:          HubAckStatusChanges                          */
/*                       HubSetPortFeature                            */
/*                       StartEnumDevice                              */
/*                       DetachSingle                                 */
/*                                                                    */
/* EXTERNAL REFERENCES:  DevHelp_ProcBlock                            */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void HubPortStatusRetrieved( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex, hIndex;
   BOOL         lowSpeedDevice;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   //  get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
   {  // failed to read port status information
#ifdef   DEBUG
      dsPrint4(DBG_CRITICAL, "USBD: HubPortStatusRetrieved Failed for Port%d, Status=%x,devIndx=%d, hubindx=%d\r\n",
               gHubs[hubIndex].portNum, pRP_GENIOCTL->rph.Status, deviceIndex, hubIndex);
#endif
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_OK;// clear error code to bypass error processing in HubStatusChanged
      HubStatusChanged( pRP_GENIOCTL );      // continue hub status change processing
      return;
   }

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: HubPortStatusRetrieved Port%d Status=%x,PortChange=%x",
            gHubs[hubIndex].portNum, gHubs[hubIndex].portStatus.wPortStatus,gHubs[hubIndex].portStatus.wPortChange);
   dsPrint2(DBG_IRQFLOW, "(%x,%x)\r\n",
            gHubs[hubIndex].portStatus.wPortStatus,gHubs[hubIndex].portStatus.wPortChange);
#endif

   if (gHubs[hubIndex].portStatus.wPortStatus & STATUS_PORT_CONNECTION)
   {  // if device is attached to port try to reset port turning it into "enable" status
      if (!(gHubs[hubIndex].portStatus.wPortStatus & STATUS_PORT_OVER_CURRENT))
      {
         if (!(gHubs[hubIndex].portStatus.wPortStatus&STATUS_PORT_ENABLE))
         {
            if (!(gHubs[hubIndex].portStatus.wPortStatus&STATUS_PORT_RESET) && !gHubs[hubIndex].resetRetries)
            {
#ifdef   DEBUG
               dsPrint(DBG_IRQFLOW, "USBD: HubPortStatusRetrieved: resetting device\r\n");
#endif
               SuppressProcessing( (ULONG)(HubInfo FAR *)&gHubs[hubIndex], USB_INSERTION_DELAY );  // 02/17/2000 MB
               gHubs[hubIndex].resetRetries++;
               HubSetPortFeature( pRP_GENIOCTL, PORT_RESET, USBD_IRQ_STATUS_PORTCHG );
               return;
            }
            else
            {  // wait until reset completes
#ifdef   DEBUG
               dsPrint(DBG_IRQFLOW, "USBD: HubPortStatusRetrieved: reading port status\r\n");
#endif
               gHubs[hubIndex].resetRetries++;
               if (gHubs[hubIndex].resetRetries<=HUB_RESET_RETRIES)
               {
                  GetPortStatus( pRP_GENIOCTL ); // read port status
               }
               else
                  HubStatusChanged( pRP_GENIOCTL );  // continue hub status change processing
               return;
            }
         }
      }
      else
      {
         DevHelp_Beep(2000, 1000);  // over current detected
      }
   }

   if (gHubs[hubIndex].portStatus.wPortChange)
   {  // acknowledge all the changes
#ifdef   DEBUG
      dsPrint(DBG_IRQFLOW, "USBD: HubPortStatusRetrieved called to ack\r\n");
#endif
      HubAckStatusChanges( pRP_GENIOCTL );
      return;
   }

   if (gHubs[hubIndex].portStatus.wPortStatus&STATUS_PORT_CONNECTION)
   {
      // device detected - read in its device descriptor
#ifdef   DEBUG
      dsPrint(DBG_IRQFLOW, "USBD: HubPortStatusRetrieved called to add new\r\n");
#endif

      // check whether default address (0) is in use
      for (hIndex=0; hIndex<gNoOfHubDataEntries; hIndex++)
      {
         if (!gHubs[hIndex].deviceAddress)
            continue;
         if (gHubs[hubIndex].ctrlID!=gHubs[hIndex].ctrlID)
            continue;
         if (gHubs[hIndex].defAddressInUse)
            break;
      }
      if (hIndex>=gNoOfHubDataEntries)
      {  //  default address is not used - start device setup
         gHubs[hubIndex].defAddressInUse=TRUE;
         lowSpeedDevice=(gHubs[hubIndex].portStatus.wPortStatus&STATUS_PORT_LOW_SPEED)!=0;
         StartEnumDevice( pRP_GENIOCTL, processedRB->controllerId, lowSpeedDevice,
                        hubIndex, gHubs[hubIndex].portNum );
      }
      else
         gHubs[hubIndex].defAddressUseDelayed=TRUE;
   }
   else
   {
      // no device attached to port - free all assoaciated I/O requests,
      // device/hub data entries and RM devices, notify class drivers
      DetachHubPortDevices( processedRB->controllerId, hubIndex, gHubs[hubIndex].portNum );

      pRP_GENIOCTL->rph.Status=USB_IDC_RC_OK;
      HubStatusChanged( pRP_GENIOCTL );  // continue hub status change processing 
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  DetachHubPortDevices                             */
/*                                                                    */
/* DESCRIPTIVE NAME:  Detach devices attached to given hub port       */
/*                                                                    */
/* FUNCTION:  This routine processes "device detached" request -      */
/*            deletes RM resources for current device and in case of  */
/*            hub devices deletes also connected to hub devices.      */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   DetachHubPortDevices                               */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  UCHAR controllerId - device controller ID                  */
/*         USHORT parentHubIndex - index to parent hub in hub table   */
/*         UCHAR portNum - port to be processed                       */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  DetachDevice                                 */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void DetachHubPortDevices( UCHAR controllerId, USHORT parentHubIndex, UCHAR portNum )  // 07/12/1999 MB - deleted static keyword
{
   DetachDevice( controllerId, parentHubIndex, portNum );

   // decrease number device entries if possible
   for (; gNoOfDeviceDataEntries>0; gNoOfDeviceDataEntries--)
   {
      if (gDevices[gNoOfDeviceDataEntries-1].deviceAddress)
         break;
   }
   // decrease number hub entries if possible
   for (; gNoOfHubDataEntries>0; gNoOfHubDataEntries--)
   {
      if (gHubs[gNoOfHubDataEntries-1].deviceAddress)
         break;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  FreeDeviceEntry                                  */
/*                                                                    */
/* DESCRIPTIVE NAME:  Free Device Data Entry                          */
/*                                                                    */
/* FUNCTION:  This routine processes releases device data entry by    */
/*            device address field to 0.                              */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   FreeDeviceEntry                                    */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  USHORT deviceIndex - index to device entry to be released  */
//*                                                                   */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  decreases index to last used device data entry           */
/*           in gNoOfDeviceDataEntries                                */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void FreeDeviceEntry( USHORT deviceIndex )
{
   if (deviceIndex<gNoOfDeviceDataEntries)
   {
      gDevices[deviceIndex].deviceAddress=0;
   }

   // decrease number device entries if possible
   for (; gNoOfDeviceDataEntries>0; gNoOfDeviceDataEntries--)
   {
      if (gDevices[gNoOfDeviceDataEntries-1].deviceAddress)
         break;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  FreeHubEntry                                     */
/*                                                                    */
/* DESCRIPTIVE NAME:  Free Device Data Entry                          */
/*                                                                    */
/* FUNCTION:  This routine processes releases hub device data entry   */
/*            by hub device address field to 0.                       */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   FreeHubEntry                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  USHORT hubIndex - index to hub device entry to be released */
//*                                                                   */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  decreases index to last used hub data entry              */
/*           in gNoOfHubDataEntries                                   */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void FreeHubEntry( USHORT hubIndex )
{
   if (hubIndex<gNoOfHubDataEntries)
   {
      gHubs[hubIndex].deviceAddress=0;
   }

   // decrease number hub entries if possible
   for (; gNoOfHubDataEntries>0; gNoOfHubDataEntries--)
   {
      if (gHubs[gNoOfHubDataEntries-1].deviceAddress)
         break;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  DetachDevice                                     */
/*                                                                    */
/* DESCRIPTIVE NAME:  Detach device                                   */
/*                                                                    */
/* FUNCTION:  This routine processes "device detached" request -      */
/*            deletes RM resources for current device and in case of  */
/*            hub devices deletes also connected to hub devices.      */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   DetachDevice                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  UCHAR controllerId - device controller ID                  */
/*         USHORT parentHubIndex - index to parent hub in hub table   */
/*         UCHAR portNum - port to be processed, if 0 routine         */
/*                         processes all hub ports                    */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  DetachSingle                                 */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  RMDestroyDevice                              */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void DetachDevice( UCHAR controllerId, USHORT parentHubIndex, UCHAR portNum )
{
   USHORT    deviceIndex, hubIndex;
   APIRET    rc;
   UCHAR     deviceAddress;

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: DetachDevice controllerId %d, parentHubIndex %d, portNum%d\r\n",
            controllerId, parentHubIndex, portNum);
#endif
   for (deviceIndex=0; deviceIndex<gNoOfDeviceDataEntries; deviceIndex++)
   {
      deviceAddress=gDevices[deviceIndex].deviceAddress;
      if (!deviceAddress)
         continue;
      if (gDevices[deviceIndex].ctrlID!=controllerId)
         continue;
      if (gDevices[deviceIndex].parentHubIndex!=parentHubIndex)
         continue;
      if (portNum && gDevices[deviceIndex].portNum!=portNum)
         continue;
#ifdef   DEBUG
      rc=0;
#endif
      if (gDevices[deviceIndex].descriptor.bDeviceClass==DEV_CLASS_HUB)
      {
         DetachDevice( controllerId, deviceIndex, 0 );   // process all attached to this hub devices

         // delete hub data entry
         for (hubIndex=0; hubIndex<gNoOfHubDataEntries; hubIndex++)
         {
            if (gHubs[hubIndex].deviceAddress==deviceAddress)
            {
               if (gHubs[hubIndex].defAddressInUse)
               {   //   cancel pending default address request
                  USBCancel      cancelIO;
                  RP_GENIOCTL    rp_USBReq;

                  cancelIO.controllerId=controllerId;
                  cancelIO.deviceAddress=USB_DEFAULT_DEV_ADDR;
                  cancelIO.endPointId=USBCANCEL_CANCEL_ALL;

                  setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
                  rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
                  rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
                  rp_USBReq.Function=USB_IDC_FUNCTION_CANCEL;
                  rp_USBReq.ParmPacket=(PVOID)&cancelIO;
                  USBCallIDC( gHostControllers[controllerId].usbIDC,
                              gHostControllers[controllerId].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );  

               }
               FreeHubEntry( hubIndex );
               break;
            }
         }
         if (gDevices[deviceIndex].rmDevHandle)
            rc=RMDestroyDevice(ghDriver, gDevices[deviceIndex].rmDevHandle);
         gDevices[deviceIndex].rmDevHandle=NULL;
         DetachSingle(deviceIndex,TRUE);
      }
      else
      {
         if (gDevices[deviceIndex].rmDevHandle)
            rc=RMDestroyDevice(gHostControllers[controllerId].hDriver, gDevices[deviceIndex].rmDevHandle);
         gDevices[deviceIndex].rmDevHandle=NULL;
         DetachSingle(deviceIndex,FALSE);
      }

#ifdef   DEBUG
      if (rc)
         dsPrint1(DBG_CRITICAL, "USBD: RMDestroyDevice - failed to destroy device - RM code - %x\r\n", rc);
      dsPrint2(DBG_IRQFLOW, "USBD: DetachDevice devaddress %d, index %d\r\n", deviceAddress,deviceIndex);
#endif

      if (deviceIndex+1==gNoOfDeviceDataEntries)
         gNoOfDeviceDataEntries--;
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  DetachSingle                                     */
/*                                                                    */
/* DESCRIPTIVE NAME:  Detach single device                            */
/*                                                                    */
/* FUNCTION:  This routine calls all the registered class drivers     */
/*            to inform on detached device, free device entry and     */
/*            cancel all I/O requests to that device.                 */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   DetachSingle                                       */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  USHORT deviceIndex - index in device table for device to   */
/*                              be processed                          */
/*         BOOL dontCallClassDrv - if TRUE suppresses Class driver    */
/*                                 calls (for HUB devices)            */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:          USBCallIDC                                   */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
static void DetachSingle(USHORT deviceIndex, BOOL dontCallClassDrv)
{
   USBDetach      detachDeviceData;
   USBCancel      cancelIO;
   RP_GENIOCTL    rp_USBReq;
   USHORT         classIndex;
   UCHAR          controllerId, deviceAddress;

   controllerId=gDevices[deviceIndex].ctrlID;
   deviceAddress=gDevices[deviceIndex].deviceAddress;
#ifdef   DEBUG
   dsPrint2(DBG_IRQFLOW, "USBD: DetachSingle controllerId %d, deviceAddress %d\r\n", controllerId,deviceAddress);
#endif

   // inform all the registered Class Device drivers that device has been detached
   detachDeviceData.controllerId=controllerId;
   detachDeviceData.deviceAddress=deviceAddress;

   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_CLASS;
   rp_USBReq.Function=USB_IDC_FUNCTION_DETDEV;
   rp_USBReq.ParmPacket=(PVOID)&detachDeviceData;

   if (!dontCallClassDrv)
      for (classIndex=0; classIndex<gNoOfRegisteredClassDrvs; classIndex++)
      {
#ifdef   DEBUG
         dsPrint1(DBG_DETAILED, "USBD: DetachSingle calling class driver %d\r\n", classIndex);
#endif
         USBCallIDC( gClassDrivers[classIndex].usbIDC,
                     gClassDrivers[classIndex].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );
      }

   FreeDeviceEntry( deviceIndex ); // delete device entry

   // cancel all I/O requests for device to be detached
   cancelIO.controllerId=controllerId;
   cancelIO.deviceAddress=deviceAddress;
   cancelIO.endPointId=USBCANCEL_CANCEL_ALL;

   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_CANCEL;
   rp_USBReq.ParmPacket=(PVOID)&cancelIO;

   USBCallIDC( gHostControllers[controllerId].usbIDC,
               gHostControllers[controllerId].usbDS, (RP_GENIOCTL FAR *)&rp_USBReq );

}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  HubAckStatusChanges                              */
/*                                                                    */
/* DESCRIPTIVE NAME:  Acknowledge hub port status changes             */
/*                                                                    */
/* FUNCTION:  This routine is called from HubPortStatusRetrieved      */
/*            repeatedely until all change flags are cleared (changes */
/*            are acknowledged).                                      */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   HubAckStatusChanges                                */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void HubAckStatusChanges( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex, featureSelector;
   UCHAR        portNum;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);
   portNum=gHubs[hubIndex].portNum;

   // set feature selector value and clear corresponding flag bit in port change status word
   if (gHubs[hubIndex].portStatus.wPortChange&STATUS_PORT_CONNECTION)
   {
      featureSelector=C_PORT_CONNECTION;
      gHubs[hubIndex].portStatus.wPortChange&=~STATUS_PORT_CONNECTION;
   }
   else
      if (gHubs[hubIndex].portStatus.wPortChange&STATUS_PORT_ENABLE)
   {
      featureSelector=C_PORT_ENABLE;
      gHubs[hubIndex].portStatus.wPortChange&=~STATUS_PORT_ENABLE;
   }
   else
      if (gHubs[hubIndex].portStatus.wPortChange&STATUS_PORT_SUSPEND)
   {
      featureSelector=C_PORT_SUSPEND;
      gHubs[hubIndex].portStatus.wPortChange&=~STATUS_PORT_SUSPEND;
   }
   else
      if (gHubs[hubIndex].portStatus.wPortChange&STATUS_PORT_OVER_CURRENT)
   {
      featureSelector=C_PORT_OVER_CURRENT;
      gHubs[hubIndex].portStatus.wPortChange&=~STATUS_PORT_OVER_CURRENT;
   }
   else
      if (gHubs[hubIndex].portStatus.wPortChange&STATUS_PORT_RESET)
   {
      featureSelector=C_PORT_RESET;
      gHubs[hubIndex].portStatus.wPortChange&=~STATUS_PORT_RESET;
   }
   else
      return;

   // fill in request block to clear selected port feature
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   gHubs[hubIndex].getPortStatus.bmRequestType=REQTYPE_TYPE_CLASS|REQTYPE_TYPE_HUBCLASS_PORT;    // Characteristics of request
   gHubs[hubIndex].getPortStatus.bRequest=REQ_CLEAR_FEATURE;   // clear hub feature block
   gHubs[hubIndex].getPortStatus.wValue=featureSelector;       // clear feature selector
   gHubs[hubIndex].getPortStatus.wIndex=(USHORT)portNum;       // destination port number
   gHubs[hubIndex].getPortStatus.wLength=0;                    // no extra in/out data
   hcdReqBlock.buffer1=(PUCHAR)&gHubs[hubIndex].getPortStatus;       // pointer to clear feature setup packet
   hcdReqBlock.buffer1Length=sizeof(gHubs[hubIndex].getPortStatus);  // setup packet size
   hcdReqBlock.buffer2=NULL;     // no additonal data to be sent to/from host
   hcdReqBlock.buffer2Length=0;  // to complete this request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to clear hub feature
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to clear hub feature
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to clear hub feature
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;       // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=USBD_IRQ_STATUS_PORTCHG; // USBD I/O call type ID - read port status information
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub indexes
   hcdReqBlock.requestData3=0;   // not used
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );

   switch (featureSelector) {                                                 // D032601
   case C_PORT_ENABLE:                                                        // D052001
   case C_PORT_RESET:                                                         // D032601
      SuppressProcessing( (ULONG)(PVOID)processedRB, HUB_RECOVERY_WAITTIME);  // D032601
      break;                                                                  // D032601
   }                                                                          // D032601

   pRP_GENIOCTL->rph.Status=rp_USBReq.rph.Status;
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  HubSetPortFeature                                */
/*                                                                    */
/* DESCRIPTIVE NAME:  Enable hub port features                        */
/*                                                                    */
/* FUNCTION:  This routine is called repeatedely from                 */
/*            HubPortStatusRetrieved to move port into 'enabled'      */
/*            status.                                                 */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   HubSetPortFeature                                  */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*         USHORT featureSelector                                     */
/*         ULONG irqSwitch                                            */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  USBAcceptIO                                  */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void HubSetPortFeature( RP_GENIOCTL FAR *pRP_GENIOCTL, USHORT featureSelector, ULONG irqSwitch )
{
   USBRB FAR    *processedRB;
   USHORT       deviceIndex, hubIndex;
   UCHAR        portNum;
   USBRB        hcdReqBlock;
   RP_GENIOCTL  rp_USBReq;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device and hub indexes from USB request block
   deviceIndex=LOUSHORT(processedRB->requestData2);
   hubIndex=HIUSHORT(processedRB->requestData2);
   portNum=gHubs[hubIndex].portNum;

   // fill in request block to set port feature
   hcdReqBlock.controllerId=processedRB->controllerId;
   hcdReqBlock.deviceAddress=gDevices[deviceIndex].deviceAddress;
   hcdReqBlock.endPointId=USB_DEFAULT_CTRL_ENDPT;    // default control endpoint
   hcdReqBlock.status=0;        // not used
   hcdReqBlock.flags=USRB_FLAGS_TTYPE_SETUP;
   gHubs[hubIndex].getPortStatus.bmRequestType=REQTYPE_TYPE_CLASS;    // Characteristics of request
   if (gHubs[hubIndex].portNum)   // if port specified send request to hub port
      gHubs[hubIndex].getPortStatus.bmRequestType|=REQTYPE_TYPE_HUBCLASS_PORT;
   gHubs[hubIndex].getPortStatus.bRequest=REQ_SET_FEATURE;  // set port(hub) feature request
   gHubs[hubIndex].getPortStatus.wValue=featureSelector;    // port(hub) feature selector
   gHubs[hubIndex].getPortStatus.wIndex=(USHORT)portNum;    // destination port number (0 for hub)
   gHubs[hubIndex].getPortStatus.wLength=0;                 // no additional data to be transferred
   hcdReqBlock.buffer1=(PUCHAR)&gHubs[hubIndex].getPortStatus;       // pointer to set port feature setup packet
   hcdReqBlock.buffer1Length=sizeof(gHubs[hubIndex].getPortStatus);  // setup packet size
   hcdReqBlock.buffer2=NULL;     // no additonal data to be sent to/from host
   hcdReqBlock.buffer2Length=0;  // to complete this request
   hcdReqBlock.serviceTime=USB_DEFAULT_SRV_INTV;   // use default service frequency to set hub feature
   hcdReqBlock.maxPacketSize=USB_DEFAULT_PKT_SIZE; // use default packet size to set hub feature
   hcdReqBlock.maxErrorCount=USB_MAX_ERROR_COUNT;  // use maximum error retry count to set hub feature
   hcdReqBlock.usbIDC=(PUSBIDCEntry)USBDidc;       // set USBD IDC routine to finish IRQ processing
   hcdReqBlock.usbDS=GetDS(); // set data segment value
   hcdReqBlock.category=USB_IDC_CATEGORY_USBD;        // set USBD layer as IRQ processor
   hcdReqBlock.requestData1=irqSwitch; // USBD I/O call type ID
   hcdReqBlock.requestData2=MAKEULONG(deviceIndex,hubIndex);   // save device and hub index
   hcdReqBlock.requestData3=0;   // not used
   setmem((PSZ)&rp_USBReq, 0, sizeof(rp_USBReq));
   rp_USBReq.rph.Cmd=CMDGenIOCTL;   // IOCTL
   rp_USBReq.Category=USB_IDC_CATEGORY_HOST;
   rp_USBReq.Function=USB_IDC_FUNCTION_ACCIO;
   rp_USBReq.ParmPacket=(PVOID)&hcdReqBlock;

   USBAcceptIO( (RP_GENIOCTL FAR *)&rp_USBReq );
   pRP_GENIOCTL->rph.Status=rp_USBReq.rph.Status;

   if(featureSelector==PORT_RESET) //  02/17/2000 MB - collected all reset delay calls here
   {
      if (gDevices[deviceIndex].parentHubIndex == HUB_NO_HUB_INDEX)                                // D052101
         SuppressProcessing( (ULONG)(HubInfo FAR *)&gHubs[hubIndex], ROOT_HUB_RESET_WAITTIME );    // D052101
      else                                                                                         // D052101
         SuppressProcessing( (ULONG)(HubInfo FAR *)&gHubs[hubIndex], HUB_RESET_WAITTIME );
   }

#ifdef   DEBUG
   dsPrint3(DBG_IRQFLOW, "USBD: HubSetPortFeature - a=%d, Port%d, selector:%d",
            gDevices[deviceIndex].deviceAddress, gHubs[hubIndex].portNum,featureSelector);
   dsPrint2(DBG_IRQFLOW, ",wPortStatus=%x, Status=%x\r\n",
            gHubs[hubIndex].portStatus.wPortStatus, pRP_GENIOCTL->rph.Status);
#endif
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  PortStatusChanged                                */
/*                                                                    */
/* DESCRIPTIVE NAME:  Hub Port status change routine                  */
/*                                                                    */
/* FUNCTION:  This routine is called from IRQ processor to continue   */
/*            port status change processing.                          */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   PortStatusChanged                                  */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  HubStatusChanged                             */
/*    ROUTINES:          GetPortStatus                                */
/*                                                                    */
/* EXTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void PortStatusChanged( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR    *processedRB;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   if (pRP_GENIOCTL->rph.Status!=USB_IDC_RC_OK)
   {  // failed to read port status information
#ifdef   DEBUG
      dsPrint2(DBG_CRITICAL, "USBD: PortStatusChanged failed. a=%d, Status=%x\r\n",
               processedRB->deviceAddress, pRP_GENIOCTL->rph.Status);
#endif
      pRP_GENIOCTL->rph.Status=USB_IDC_RC_OK;  // to bypass error processing in HubStatusChanged
      HubStatusChanged( pRP_GENIOCTL );  // continue hub status change processing
      return;
   }

   GetPortStatus( pRP_GENIOCTL );
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  ExtConfSet                                       */
/*                                                                    */
/* DESCRIPTIVE NAME:  Configuration Set processing routine            */
/*                                                                    */
/* FUNCTION:  This routine informs Class drivers that required        */
/*            configuration is set.                                   */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   ExtConfSet                                         */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void ExtConfSet( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR               *processedRB;
   SetupPacket FAR         *setPacket;
   USHORT                  deviceIndex, classIndex;
   USHORT                  callersDS, irqSwitch;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device index from request block
   deviceIndex=(USHORT)processedRB->requestData2;
   callersDS=LOUSHORT(processedRB->requestData3);
   irqSwitch=HIUSHORT(processedRB->requestData3);

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {  // failed to read port status information
      setPacket=(SetupPacket FAR *)processedRB->buffer1;
      gDevices[deviceIndex].bConfigurationValue=(UCHAR)setPacket->wValue;

#ifdef   DEBUG
      dsPrint2(DBG_IRQFLOW, "USBD: ExtConfSet %d for deviceIndex=%d\r\n",
               gDevices[deviceIndex].bConfigurationValue, deviceIndex);
#endif
   }
   else
   {
      gDevices[deviceIndex].bConfigurationValue=0; // 23/09/98 MB - reset configuration value if failed
#ifdef   DEBUG
      dsPrint1(DBG_CRITICAL, "USBD: ExtConfSet Status=%x\r\n",pRP_GENIOCTL->rph.Status);
#endif
   }
   // get callers class driver IDC data (entry address and DS value)
   for (classIndex=0; classIndex<gNoOfRegisteredClassDrvs; classIndex++)
   {
      if (gClassDrivers[classIndex].usbDS==callersDS)
         break;
   }
   if (classIndex<gNoOfRegisteredClassDrvs)
   {  // call Class driver if identified
      pRP_GENIOCTL->Category=(UCHAR)HIUSHORT(processedRB->requestData1);
      processedRB->requestData1=irqSwitch;
      processedRB->requestData2=0;
      processedRB->requestData3=0;
      USBCallIDC( gClassDrivers[classIndex].usbIDC,
                  gClassDrivers[classIndex].usbDS, pRP_GENIOCTL );
   }
}

/******************* START OF SPECIFICATIONS **************************/
/*                                                                    */
/* SUBROUTINE NAME:  ExtIntfaceSet                                    */
/*                                                                    */
/* DESCRIPTIVE NAME:  Interface Set processing routine                */
/*                                                                    */
/* FUNCTION:  This routine informs Class drivers that required        */
/*            interface is set.                                       */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task Time                                                 */
/*                                                                    */
/* ENTRY POINT :   ExtIntfaceSet                                      */
/*    LINKAGE  :  CALL NEAR                                           */
/*                                                                    */
/* INPUT:  RP_GENIOCTL FAR *pRP_GENIOCTL                              */
/*                                                                    */
/* EXIT-NORMAL:  none                                                 */
/*                                                                    */
/* EXIT-ERROR:  none                                                  */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*    ROUTINES:                                                       */
/*                                                                    */
/* EXTERNAL REFERENCES:  USBCallIDC                                   */
/*    ROUTINES:                                                       */
/*                                                                    */
/******************* END  OF  SPECIFICATIONS **************************/
void ExtIntfaceSet( RP_GENIOCTL FAR *pRP_GENIOCTL )
{
   USBRB FAR         *processedRB;
   SetupPacket FAR   *setPacket;
   USHORT            deviceIndex, classIndex;
   USHORT            callersDS, irqSwitch;

   processedRB=(USBRB FAR *)pRP_GENIOCTL->ParmPacket;

   // get device index from request block
   deviceIndex=(USHORT)processedRB->requestData2;
   callersDS=LOUSHORT(processedRB->requestData3);
   irqSwitch=HIUSHORT(processedRB->requestData3);

   if (pRP_GENIOCTL->rph.Status==USB_IDC_RC_OK)
   {  // failed to read port status information
      setPacket=(SetupPacket FAR *)processedRB->buffer1;
      gDevices[deviceIndex].bInterfaceNumber=(UCHAR)setPacket->wValue;
   }

   // get callers class driver IDC data (entry address and DS value)
   for (classIndex=0; classIndex<gNoOfRegisteredClassDrvs; classIndex++)
   {
      if (gClassDrivers[classIndex].usbDS==callersDS)
         break;
   }
   if (classIndex<gNoOfRegisteredClassDrvs)
   {  // call Class driver if identified
      pRP_GENIOCTL->Category=(UCHAR)HIUSHORT(processedRB->requestData1);
      processedRB->requestData1=irqSwitch;
      processedRB->requestData2=0;
      processedRB->requestData3=0;
      USBCallIDC( gClassDrivers[classIndex].usbIDC,
                  gClassDrivers[classIndex].usbDS, pRP_GENIOCTL );
   }
}
