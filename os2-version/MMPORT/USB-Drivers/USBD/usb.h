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
/* SCCSID = "src/dev/usb/USBD/USB.H, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USB.H                                                 */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USBD device driver master include file.               */
/*                                                                            */
/*   FUNCTION: This module is the USBD device driver master                   */
/*             include file.                                                  */
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
/*  Mark    yy/mm/dd  Programmer      Comment                                 */
/*  ----    --------  ----------      -------                                 */
/*          98/01/16  MB                                                      */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

// USBD specific debug definitions
#ifdef   DEBUG
   #define         dsPrint(l,s)              dsPrint5x(gUSBDMsgLevel,(l),(s),0,0,0,0,0)
   #define         dsPrint1(l,s,a)           dsPrint5x(gUSBDMsgLevel,(l),(s),(a),0,0,0,0)
   #define         dsPrint2(l,s,a,b)         dsPrint5x(gUSBDMsgLevel,(l),(s),(a),(b),0,0,0)
   #define         dsPrint3(l,s,a,b,c)       dsPrint5x(gUSBDMsgLevel,(l),(s),(a),(b),(c),0,0)
   #define         dsPrint4(l,s,a,b,c,d)     dsPrint5x(gUSBDMsgLevel,(l),(s),(a),(b),(c),(d),0)
#endif

#include "usbcmmon.h"         // USB stack common include files
#include "usbmisc.h"          // externals defined in usbmisc.lib

#include "usbdtype.h"         /* USBD driver specific typedefs */
#include "usbproto.h"         /* USBD driver function prototypes */
#include "usbextrn.h"         /* USBD driver external data declarations */

