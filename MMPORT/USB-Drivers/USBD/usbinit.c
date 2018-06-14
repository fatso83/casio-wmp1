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
/* SCCSID = "src/dev/usb/USBD/USBINIT.C, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBINIT.C                                             */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USB device driver initialization                      */
/*                      routine.                                              */
/*                                                                            */
/*   FUNCTION: This routine:                                                  */
/*						1) analyzes BASEDEV= parameters;                      */
/*						2) checks for HCD layer driver presence and fails     */
/*							to initialize if no HCD drivers found;		      */
/*						3) initalizes USBD driver's local structures          */
/*             	        4) registers driver & adapter with RM.                */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             USBInit                                                        */
/*                                                                            */
/*   EXTERNAL REFERENCES:                                                     */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark      yy/mm/dd  Programmer    Comment                                 */
/*  ------    --------  ----------    -------                                 */
/*            98/01/31  MB                                                    */
/* 31/05/1999 99/05/31  MB            Added floppy boot support code          */
/* 05/16/2000 00/05/16  MB            Changed init exit code to quiet exit if */
/*                                    required host controller driver(s) not  */
/*                                    installed.                              */
/* LR0904     01/09/04  LR            Changed CheckHCDDriversLoaded function. */
/*                                    USBD will be loaded if at least one of  */
/*                                    requested HC drivers is loaded.         */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

#include "usb.h"

static BOOL RegisterUSBD(void);
static BOOL CheckHCDDriversLoaded( PSZ cmdLine, USHORT start, USHORT end );

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBInit                                          */
/*                                                                    */
/* DESCRIPTIVE NAME:  Initializes the USBD device driver.             */
/*                                                                    */
/* FUNCTION:  The function of this routine is to                      */
/*            1) process BASEDEV= line parameters                     */
/*            2) display initialization messages if required          */
/*            3) register driver and adapter with RM.                 */
/*            4) initialize internal data structures.                 */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Initialization time                                       */
/*                                                                    */
/* ENTRY POINT:  USBInit                                              */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  pRP-> kernel request packet                                */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS:  pRPO->CodeEnd = end of code segment                      */
/*           pRPO->DataEnd = end of data segment                      */
/*           pRP->Status = STATUS_DONE                                */
/*           pRP->Status = STDON + STERR + ERROR_I24_GEN_FAILURE      */
/*                                                                    */
/* INTERNAL REFERENCES:  RegisterUSBD                                 */
/*                                                                    */
/* EXTERNAL REFERENCES:  RMCreateDriver                               */
/*                       RMCreateAdapter                              */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
void USBInit( RPH FAR *pRP )
{
   PRPINITOUT      pRPO;          /* output request packet far pointer */
   PRPINITIN       pRPI;          /* input request packet far pointer */
   UINT            ctrlID;
   KeyData         keyData[3]={"V",CFSTR_TYPE_DEC,0,0,
      "REQ:",CFSTR_TYPE_STRING,0,0,
      "I13",CFSTR_TYPE_DEC,0,0}; // 31/05/1999 MB
   PSZ             cmdLine;
   ULONG           cmdRStatus, errColumn;
   USHORT          cmdRc;

#ifdef   DEBUG
   dsPrint(DBG_HLVLFLOW, "USBD: USBInit started\r\n");
#endif

   pRP->Status = STATUS_DONE;
   if (ghDriver) // initialization already passed
      return;

   pRPI = (PRPINITIN) pRP;
   pRPO = (PRPINITOUT) pRP;

   Device_Help = ((PRPINITIN)pRP)->DevHlpEP;  /* save devhlp entry point */

   // process CONFIG.SYS BASEDEV= line parameters
   cmdLine=(PSZ) MAKEP( SELECTOROF(pRPI->InitArgs),
                        OFFSETOF(((PDDD_PARM_LIST)pRPI->InitArgs)->cmd_line_args) );
   cmdRStatus=ProcessConfigString(cmdLine, sizeof(keyData)/sizeof(keyData[0]), (KeyData FAR *)&keyData);
   cmdRc=LOUSHORT(cmdRStatus); errColumn=(ULONG)HIUSHORT(cmdRStatus);
   switch (cmdRc)  // set cmd line processing errors
   {
   case CFSTR_UNKWN_KEYS:
      SetLongValue( gVMessages[INIT_MESSAGE_UNKNOWNKWD], errColumn );
      gMessageCount=AddToMsgArray( gMessageIDs, INIT_MESSAGE_UNKNOWNKWD, gMessageCount, MAX_INIT_MESSAGE_COUNT );
      break;
   case CFSTR_CONVERR:
      SetLongValue( gVMessages[INIT_MESSAGE_INVNUMERIC], errColumn );
      gMessageCount=AddToMsgArray( gMessageIDs, INIT_MESSAGE_INVNUMERIC, gMessageCount, MAX_INIT_MESSAGE_COUNT );
      break;
   default:
      break;
   }
   if (keyData[1].value &&  // check for HCD layer drivers
       !CheckHCDDriversLoaded( cmdLine, LOUSHORT(keyData[1].value), HIUSHORT(keyData[1].value) ))
   {
      pRP->Status = STATUS_DONE | STERR | ERROR_I24_QUIET_INIT_FAIL; // 05/16/2000 MB - exit code changed to quiet exit code
      gMessageCount=AddToMsgArray( gMessageIDs, INIT_MESSAGE_NO_HCD, gMessageCount, MAX_INIT_MESSAGE_COUNT );
   }
   gVerbose= keyData[0].keyStatus!=CFSTR_STATUS_NOTFOUND;

   gDelayHostStart = keyData[2].keyStatus!=CFSTR_STATUS_NOTFOUND; // 31/05/1999 MB
   if(gDelayHostStart && (pRP->Status == STATUS_DONE) )  // 05/16/2000 MB - fixed error code processing
   {
      USHORT         ctxOffset;  
      USHORT         rc;

      ctxOffset=(USHORT)(LONG)(StartHostCtxHookRtn); // IRQ processing thread
      rc = DevHelp_AllocateCtxHook((NPFN)ctxOffset, &gStartHostHookHandle);

      if (rc)
      {
         pRP->Status = STATUS_DONE | STERR | STATUS_ERR_UNKCMD;
         gMessageCount=AddToMsgArray( gMessageIDs, INIT_MESSAGE_SETCTXFAILED, gMessageCount, MAX_INIT_MESSAGE_COUNT );
#ifdef DEBUG
      dsPrint(DBG_CRITICAL, " USBD: USBDInit failed to allocate CTX hook routine\r\n");
#endif
      }
   }

   /*---------------------------------------------------*/
   /* Register device driver with resource manager      */
   /*---------------------------------------------------*/
   if (pRP->Status == STATUS_DONE)
      RegisterUSBD();

   if (pRP->Status == STATUS_DONE)
   {
      //	initialize local structures
      setmem((PSZ)&gHostControllers,0,sizeof(gHostControllers));
      setmem((PSZ)&gClassDrivers,0,sizeof(gClassDrivers));
      setmem((PSZ)&gDevices,0,sizeof(gDevices));
      setmem((PSZ)&gHubs,0,sizeof(gHubs));
      for (ctrlID=1; ctrlID<gMaxControllers; ctrlID++)
      {
         gSetAddress[ctrlID]=gSetAddress[0];
         gSetConfiguration[ctrlID]=gSetConfiguration[0];
         gDeviceStruct[ctrlID]=gDeviceStruct[0];
         gGetConfiguration[ctrlID]=gGetConfiguration[0];
         gGetHubConfigurationLength[ctrlID]=gGetHubConfigurationLength[0];
      }
   }

   if (pRP->Status == STATUS_DONE)
   {
      SetLongValue( gVMessages[INIT_MESSAGE_LOADED], (ULONG)gDriverStruct.MajorVer );   
      SetLongValue( gVMessages[INIT_MESSAGE_LOADED], (ULONG)gDriverStruct.MinorVer );   
      gMessageCount=AddToMsgArray( gMessageIDs, INIT_MESSAGE_LOADED, gMessageCount, MAX_INIT_MESSAGE_COUNT );
      pRPO->CodeEnd = ((USHORT) &USBInit) - 1;         /* set end of code segment */
      pRPO->DataEnd = ((USHORT) &gInitDataStart) - 1;  /* set end of data segment */
   }
   else
   {
      pRPO->CodeEnd = 0;          /* set end of code segment */
      pRPO->DataEnd = 0;       /* set end of data segment */
   }

   TTYWrite(gVMessages, gMessageIDs, gMessageCount);

#ifdef   DEBUG
   dsPrint1(DBG_HLVLFLOW, "USBD: USBInit ended. Status %x\r\n", pRP->Status);
#endif
   return;
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  RegisterUSBD                                     */
/*                                                                    */
/* DESCRIPTIVE NAME:  Registers the USBD device driver resources.     */
/*                                                                    */
/* FUNCTION:  The function of this routine is to register USBD        */
/*            driver and adapter.                                     */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Initialization time                                       */
/*                                                                    */
/* ENTRY POINT:  RegisterUSBD                                         */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  none                                                       */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS:  Fills in ghDriver, ghAdapter                             */
/*                                                                    */
/* INTERNAL REFERENCES:  RegisterUSBD                                 */
/*                                                                    */
/* EXTERNAL REFERENCES:  RMCreateDriver                               */
/*                       RMCreateAdapter                              */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
static BOOL RegisterUSBD(void)
{
   APIRET      rc=RMRC_SUCCESS;
   UCHAR       ResourceBuf[ 12 ];  
   ADJUNCT     AdjAdaptNum;
   PAHRESOURCE pResourceList = (PAHRESOURCE)ResourceBuf;

   rc=RMCreateDriver( &gDriverStruct, &ghDriver);

   if (rc==RMRC_SUCCESS)
   {
      pResourceList->NumResource = 0;

      gAdapterStruct.HostBusType = AS_HOSTBUS_PCI;
      AdjAdaptNum.pNextAdj       = NULL;
      AdjAdaptNum.AdjLength      = sizeof(ADJUNCT);
      AdjAdaptNum.AdjType        = ADJ_ADAPTER_NUMBER;
      AdjAdaptNum.Adapter_Number = 0;
      gAdapterStruct.pAdjunctList = &AdjAdaptNum;

      rc=RMCreateAdapter(ghDriver, &ghAdapter,  &gAdapterStruct,
                         NULL, pResourceList);
   }

   return (rc==RMRC_SUCCESS);
}

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  CheckHCDDriversLoaded                            */
/*                                                                    */
/* DESCRIPTIVE NAME:  Required Driver check.                          */
/*                                                                    */
/* FUNCTION:  The function of this routine is to check for HCD        */
/*            layer drivers loaded. HCD driver list is obtained       */
/*            from comandline parameters.
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Initialization time                                       */
/*                                                                    */
/* ENTRY POINT:  CheckHCDDriversLoaded                                */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  PSZ cmdLine  - pointer command line passed to driver       */
/*         USHORT start - offset to HCD driver name list              */ 
/*         USHORT end   - offset to next after last character in      */
/*                        driver list                                 */
/*                                                                    */
/*                                                                    */
/* EXIT-NORMAL: TRUE, at least one of requested HC drivers is loaded  */
/*                                                                    */
/* EXIT-ERROR:  FALSE, requested HC drivers are not loaded            */
/*                                                                    */
/* EFFECTS:  none                                                     */
/*                                                                    */
/* INTERNAL REFERENCES:  none                                         */
/*                                                                    */
/* EXTERNAL REFERENCES:  setmem                                       */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/

static BOOL CheckHCDDriversLoaded( PSZ cmdLine, USHORT start, USHORT end )
{
   PSZ      drvNameChar=cmdLine+start;
   BOOL     loaded = FALSE;   //LR0904 was = TRUE
   USHORT   charIndex=0;
   CHAR     nChar;

   gHCDDriverName[sizeof(gHCDDriverName)-1]=0;

   if (*(cmdLine+start)=='(' && end>0 && *(cmdLine+end-1)==')')
   {  // ignore enclosing braces
      start++; end--; end--;
   }
   for (;start<=end; start++)
   {
      nChar=*(cmdLine+start);
      if (nChar!=',' && nChar && nChar!=' ')
      {  // fill in driver's name separated by ','
         if (charIndex<sizeof(gHCDDriverName)-1)
         {
            if (!charIndex)
               setmem(gHCDDriverName,' ',sizeof(gHCDDriverName)-1);
            gHCDDriverName[charIndex]=nChar;
            charIndex++;
         }
      }
      if (nChar==',' || start==end)  // process name when separator reached or on last character
      {
         charIndex=0;
         // check if driver loaded
         if (!DevHelp_AttachDD (gHCDDriverName, (NPBYTE)&gDDTable))  //LR0904 ! added
         {
            loaded = TRUE; //LR0904 was = FALSE
            break;
         }
      }
   }
   return (loaded);
}

