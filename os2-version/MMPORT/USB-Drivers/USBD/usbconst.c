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
/* SCCSID = "src/dev/usb/USBD/USBCONST.C, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  USBCONST.C                                            */
/*                                                                            */
/*   DESCRIPTIVE NAME:  USB device driver constants                           */
/*                                                                            */
/*   FUNCTION: This module allocates the global constants (strings) for the   */
/*             USB device driver.                                             */
/*                                                                            */
/*   NOTES: This module exists so that the strings will be placed after       */
/*          the device driver header in the data segment.                     */
/*                                                                            */
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
/*  Mark    yy/mm/dd  Programmer      Comment                                 */
/*  ----    --------  ----------      -------                                 */
/*          98/01/31  MB              Original developer.                     */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/

const char gDDName[]="USBD.SYS";
const char gDDDesc[]="USB device driver";
const char gVendorID[]="IBM OS/2";
const char gAdapterName[]="USB Hub Controller";
const char gRootHubDeviceName[]="USB Root Hub Device_";  // should not exceed 21 character
const char gHubDeviceName[]="USB Hub Device_";           // should not exceed 21 character
const char gUSBDeviceName[]="USB Device_";               // should not exceed 21 character

