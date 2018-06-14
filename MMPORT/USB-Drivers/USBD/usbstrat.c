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
/*   SOURCE FILE NAME:  USBSTRAT.C                                            */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USBD driver strategy routine.                         */
/*                                                                            */
/*   FUNCTION: These routines handle the task time routines for the strategy  */
/*             entry point of USBD device driver.                             */
/*                                                                            */
/*   NOTES:                                                                   */
/*      DEPENDENCIES: None                                                    */
/*      RESTRICTIONS: None                                                    */
/*                                                                            */
/*   ENTRY POINTS:                                                            */
/*             USBStrategy                                                    */
/*             CmdError                                                       */
/*             USBInitComplete                                                */
/*                                                                            */
/*   EXTERNAL REFERENCES:                                                     */
/*                                                                            */
/* Change Log                                                                 */
/*                                                                            */
/*  Mark      yy/mm/dd  Programmer    Comment                                 */
/*  ------    --------  ----------    -------                                 */
/*            98/01/16  MB            Original developer.                     */
/* 01/20/2000 00/01/20  MB            Added APM support call.                 */
/*  LR0612    01/06/12  LR            Added delayed host start in             */
/*                                    USBInitComplete.                        */
/*  LR0831    01/08/31  LR            Fixed delayed host start in             */
/*                                    USBInitComplete function.               */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

#include        "usb.h"

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBStrategy                                      */
/*                                                                    */
/* DESCRIPTIVE NAME:  Strategy entry point for the USBD device        */
/*                    driver.                                         */
/*                                                                    */
/* FUNCTION:  The function of this routine is to call strategy worker */
/*            routine to process request packet.                      */
/*                                                                    */
/* NOTES:                                                             */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBStrategy                                          */
/*    LINKAGE:  CALL FAR                                              */
/*                                                                    */
/* INPUT:  es:bx -> kernel request packet                             */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS: None                                                      */
/*                                                                    */
/* INTERNAL REFERENCES:  CmdError                                     */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
#pragma optimize("eglt", off)
void FAR USBDStrategy()
{
   RPH        far  *pRP;
   USHORT       Cmd;

   _asm
   {
      mov word ptr pRP[0], bx
      mov word ptr pRP[2], es
   }

   Cmd = pRP->Cmd;
#ifdef DEBUG
   dsPrint1( DBG_HLVLFLOW, "USBD: USBDStrategy: Cmd = %x\r\n", Cmd );
#endif

   if (Cmd > MAX_USB_CMD)
   {
      CmdError( pRP );
   }
   else
   {
      /*---------------------*/
      /* Call Worker Routine */
      /*---------------------*/
      (*gStratList[Cmd])(pRP);
   }
#ifdef DEBUG
   dsPrint1( DBG_HLVLFLOW, "USBD: USBDStrategy: Exit: Cmd = %x\r\n", Cmd );
#endif

}
#pragma optimize("", on)

/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  CmdError                                         */
/*                                                                    */
/* DESCRIPTIVE NAME:  Command not supported in the device driver      */
/*                                                                    */
/* FUNCTION:  The function of this routine is to return command not   */
/*            supported for the request.                              */
/*                                                                    */
/* NOTES: This is an immediate command that is not put on the FIFO    */
/*        queue.                                                      */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  CmdError                                             */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  pRP-> kernel request packet                                */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS: pRP->Status = STDON + STERR + ERROR_I24_BAD_COMMAND       */
/*                                                                    */
/* INTERNAL REFERENCES:  None                                         */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/
void CmdError( RPH FAR *pRP)
{
#ifdef DEBUG
   dsPrint1( DBG_CRITICAL, "USBD: CmdError - Strategy Command = %d\r\n", pRP->Cmd );
#endif

   pRP->Status = STDON + STERR + ERROR_I24_BAD_COMMAND;
   return;
}


/********************** START OF SPECIFICATIONS ***********************/
/*                                                                    */
/* SUBROUTINE NAME:  USBInitComplete                                  */
/*                                                                    */
/* DESCRIPTIVE NAME:  Initialization complete                         */
/*                                                                    */
/* FUNCTION:  The function of this routine is to finish USBD          */
/*            driver initialization.                                  */
/*                                                                    */
/* NOTES: This is an immediate command that is not put on the FIFO    */
/*        queue.                                                      */
/*                                                                    */
/* CONTEXT: Task time                                                 */
/*                                                                    */
/* ENTRY POINT:  USBInitComplete                                      */
/*    LINKAGE:  CALL NEAR                                             */
/*                                                                    */
/* INPUT:  pRP-> kernel request packet                                */
/*                                                                    */
/* EXIT-NORMAL: N/A                                                   */
/*                                                                    */
/* EXIT-ERROR:  N/A                                                   */
/*                                                                    */
/* EFFECTS: pRP->Status = STATUS_DONE                                 */
/*                                                                    */
/* INTERNAL REFERENCES:  None                                         */
/*                                                                    */
/* EXTERNAL REFERENCES:  None                                         */
/*                                                                    */
/************************ END OF SPECIFICATIONS ***********************/

void USBInitComplete (RPH FAR *pRP)
{
   //LR0612begin
#ifdef DEBUG
   dsPrint1 (DBG_HLVLFLOW, "USBD: USBInitComplete start=%x\r\n", gDelayHostStart);
#endif
   
   if (gDelayHostStart)
   {  // delayed host start
//LR0831      gDelayHostStart = FALSE;
      DevHelp_ArmCtxHook (0, gStartHostHookHandle);
   }
   //LR0612end
   USBAPMRegister(); // 01/20/2000 MB - added to support APM

   pRP->Status = STATUS_DONE;

#ifdef DEBUG
   dsPrint1( DBG_HLVLFLOW, "USBD: USBInitComplete Rc=%x\r\n",pRP->Status );
#endif

   return;
}

