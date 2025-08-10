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
/* SCCSID = "src/dev/usb/INCLUDE/USBTYPE.H, usb, c.basedd 98/07/10" */
/*
*
*/
/************************** START OF SPECIFICATIONS ***************************/
/*                                                                            */
/*   SOURCE FILE NAME:  UHCI.H                                                */
/*                                                                            */
/*   DESCRIPTIVE NAME:  Universal Host Controller Interface (UHCI)            */
/*                      structure and flag definitons.                        */
/*                                                                            */
/*   FUNCTION: This header file contains UHCI specific data structure and     */
/*                      flag definitions                                      */
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
/*  ---------- --------  ----------   -------                                 */
/*             97/12/18  MB                                                   */
/*  07/04/1999 99/04/07  MB           added descriptor part size definitions  */
/*  02/17/2000 00/02/17  MB           Increased power-on delay time definition*/
/*  LR0323     01/03/23  LR           Added USB 1.1 hub descriptor type       */
/*                                                                            */
/**************************** END OF SPECIFICATIONS ***************************/
#ifndef _USBTYPE_h_
   #define _USBTYPE_h_

typedef struct _setup_Packet_
{
   UCHAR      bmRequestType;     // (00) Characteristics of request
   UCHAR      bRequest;          // (01) Specific Request
   USHORT     wValue;            // (02) Word-sized field (value depends on request)
   USHORT     wIndex;            // (04) typically Index or Offset
   USHORT     wLength;           // (06) Number of bytes to Transfer
                                 // (08)
}  SetupPacket;    

   #define  REQTYPE_XFERDIR_DEVTOHOST     0x80  // data transfer direction device to host
   #define  REQTYPE_XFERDIR_HOSTTODEV     0x00  // data transfer direction host to device
   #define  REQTYPE_TYPE_MASK             0x60  // type mask
   #define  REQTYPE_TYPE_STANDARD         0x00  // Standard
   #define  REQTYPE_TYPE_CLASS            0x20  // class
   #define  REQTYPE_TYPE_VENDOR           0x40  // vendor
   #define  REQTYPE_TYPE_RESERV           0x60  // reserved

   #define  REQTYPE_RECIPIENT_DEVICE      0
   #define  REQTYPE_RECIPIENT_INTERFACE   1
   #define  REQTYPE_RECIPIENT_ENDPOINT    2
   #define  REQTYPE_RECIPIENT_OTHER       3
   #define  REQTYPE_RECIPIENT_MASK        0x0f

// HUB class specific request type modifiers
   #define  REQTYPE_TYPE_HUBCLASS_PORT    0x03  // request to/from hub port


// Request values                                                
   #define  REQ_GET_STATUS                0
   #define  REQ_CLEAR_FEATURE             1
   #define  REQ_SET_FEATURE               3
   #define  REQ_SET_ADDRESS               5
   #define  REQ_GET_DESCRIPTOR            6
   #define  REQ_SET_DESCRIPTOR            7
   #define  REQ_GET_CONFIGURATION         8
   #define  REQ_SET_CONFIGURATION         9
   #define  REQ_GET_INTERFACE             10
   #define  REQ_SET_INTERFACE             11
   #define  REQ_SYNCH_FRAME               12

// HUB class specific request type modifiers
   #define  REQ_GET_STATE                 2

// general descriptor types
   #define  DESC_DEVICE                   0x01
   #define  DESC_CONFIGURATION            0x02
   #define  DESC_STRING                   0x03
   #define  DESC_INTERFACE                0x04
   #define  DESC_ENDPOINT                 0x05
// class specific dscriptor types
   #define  DESC_HUB                      0x00
   #define  DESC_HUB11                    0x29  //LR0323 USB 1.1 hub descriptor type

typedef struct _device_deschead_
{
   CHAR    bLength;              // (00) Size of descriptor in bytes
   CHAR    bDescriptorType;      // (01) Descriptor type
   UCHAR   bDescriptorSubtype;   // (02) Descriptor subtype   // 28/09/98 MB
                                 // (03)
}  DeviceDescHead;

typedef struct _device_descriptor_
{
   UCHAR    bLength;             // (00) Size of descriptor in bytes
   UCHAR    bDescriptorType;     // (01) 0x01 - DEVICE Descriptor type
   USHORT   bcdUSB;              // (02) USB Specification Release Number
   UCHAR    bDeviceClass;        // (04) Class Code
   UCHAR    bDeviceSubClass;     // (05) SubClass Code
   UCHAR    bDeviceProtocol;     // (06) Protocol Code
   UCHAR    bMaxPacketSize0;     // (07) Maximum packet size for endpoint 0
   USHORT   idVendor;            // (08) Vendor ID
   USHORT   idProduct;           // (10) Product ID
   USHORT   bcdDevice;           // (12) Device release number
   UCHAR    iManufacturer;       // (14) Index of string descriptor describing manufacturer
   UCHAR    iProduct;            // (15) Index of string descriptor describing product
   UCHAR    iSerialNumber;       // (16) Index of string descriptor describing device's serial number
   UCHAR    bNumConfigurations;  // (17) Number of possible configurations
                                 // (18)
}  DeviceDescriptor;

   #define DEV_DESC_SIZE_SHORT               8 // descriptor size up to bMaxPacketSize0 field  // 07/04/1999 MB
   #define DEV_DESC_SIZE                    18 // Total Descriptor Size

   #define DEV_CLASS_RESERVED             0x00
   #define DEV_CLASS_AUDIO                0x01
   #define DEV_CLASS_COMMUNICATIONS       0x02
   #define DEV_CLASS_HUMAN_INTERFACE      0x03
   #define DEV_CLASS_MONITOR              0x04
   #define DEV_CLASS_PHYSICAL_INTERFACE   0x05
   #define DEV_CLASS_POWER                0x06
   #define DEV_CLASS_PRINTER              0x07
   #define DEV_CLASS_STORAGE              0x08
   #define DEV_CLASS_HUB                  0x09
   #define DEV_CLASS_VENDOR_SPECIFIC      0xFF

   #define  DEV_SUBCLASS_HUBSUBCLASS      0x09

typedef struct _device_configuration_
{
   UCHAR    bLength;             // (00) Size of descriptor in bytes
   UCHAR    bDescriptorType;     // (01) 0x02 - CONFIGURATION Descriptor type
   USHORT   wTotalLength;        // (02) total data length returned in request
   UCHAR    bNumInterfaces;      // (04) number of interfaces in this configuration
   UCHAR    bConfigurationValue; // (05) value to be used in Set configuration
   UCHAR    iConfiguration;      // (06) String index describing this configuration
   UCHAR    bmAttributes;        // (07) Configuration characteristics
   UCHAR    MaxPower;            // (08) power consumption in 2 mA units
                                 // (09)
}  DeviceConfiguration;

   #define  DEV_CONFIG_SIZE_UPTOTLEN      4  // descriptor size up to wTotalLength field  // 07/04/1999 MB

   #define  DEV_CONFIG_BUSPOWRD           0x80  // bus powered device
   #define  DEV_CONFIG_SLFPOWRD           0x40  // self powered device
   #define  DEV_CONFIG_RWAKEUP            0x20  // remote wakup supported


typedef struct _device_interface_
{
   UCHAR    bLength;             // (00) Size of descriptor in bytes
   UCHAR    bDescriptorType;     // (01) 0x04 - INTERFACE Descriptor type
   UCHAR    bInterfaceNumber;    // (02) 0 based index in interface array for this item
   UCHAR    bAlternateSetting;   // (03) value to select alternate interface
   UCHAR    bNumEndpoints;       // (04) no of endpoints used by current interface (excluding enpoint 0)
   UCHAR    bInterfaceClass;     // (05) Class code
   UCHAR    bInterfaceSubClass;  // (06) Subclass code
   UCHAR    bInterfaceProtocol;  // (07) Protocol code
   UCHAR    iInterface;          // (08) descriptor string index
                                 // (09)
}  DeviceInterface;

typedef struct _device_endpoint_
{
   UCHAR    bLength;          // (00) Size of descriptor in bytes
   UCHAR    bDescriptorType;  // (01) 0x05 - ENDPOINT Descriptor type
   UCHAR    bEndpointAddress; // (02) address of endpoint
   UCHAR    bmAttributes;     // (03) endpoint's attributes
   USHORT   wMaxPacketSize;   // (04) maximum packet size for this endpoint
   UCHAR    bInterval;        // (06) interval for polling endpoint for data
                              // (07)
}  DeviceEndpoint;


   #define  DEV_ENDPT_ADDRMASK   0x0f
   #define  DEV_ENDPT_DIRMASK    0x80
   #define  DEV_ENDPT_DIRIN      0x80
   #define  DEV_ENDPT_DIROUT     0x00
   #define  DEV_ENDPT_ATTRMASK   0x03
   #define  DEV_ENDPT_CTRL       0x00
   #define  DEV_ENDPT_ISOHR      0x01
   #define  DEV_ENDPT_BULK       0x02
   #define  DEV_ENDPT_INTRPT     0x03

typedef struct _hub_descriptor_
{
   UCHAR    bLength;             // (00) Size of descriptor in bytes
   UCHAR    bDescriptorType;     // (01) 0x00 - HUB DECRIPTOR type
   UCHAR    bNbrPorts;           // (02) number of downstream ports that this hub supports
   USHORT   wHubCharacteristics; // (03) Hub device characteristics
   UCHAR    bPwrOn2PwrGood;      // (05) Delay time (in 2ms intervals) to be operatinal after power on
   UCHAR    bHubContrCurrent;    // (06) max current req for hub electronics in mA
   UCHAR    portFlags[64];       // (07) device type and power control masks
                                 // (71)
}  HubDescriptor;

   #define  HUB_DESC_CHAR_POWER_MASK      0x0003   // power switching mode
   #define  HUB_DESC_CHAR_POWER_GANGED    0x0000   // value - all ports power at once
   #define  HUB_DESC_CHAR_POWER_INDIV     0x0001   // value for individual port power switching
   #define  HUB_DESC_CHAR_POWER_NOSWTCH   0x0002   // flag - powered on together with hub

   #define  HUB_DESC_CHAR_COMPOUND_DEV    0x0004 

   #define  HUB_DESC_CHAR_OCURRENT_MASK   0x0018   // over-current Protection Mode
   #define  HUB_DESC_CHAR_OCURRENT_GLOBAL 0x0000   // value - global over-current protection
   #define  HUB_DESC_CHAR_OCURRENT_INDIV  0x0008   // value for individual over-current protection
   #define  HUB_DESC_CHAR_OCURRENT_NOPROT 0x0010   // flag - no over-current protection

// general feature selectors
   #define  DEVICE_REMOTE_WAKEUP    1
   #define  ENDPOINT_STALL          0

// Hub Class Feature Selectors
   #define  C_HUB_LOCAL_POWER       0 
   #define  C_HUB_OVER_CURRENT      1
   #define  PORT_CONNECTION         0 
   #define  PORT_ENABLE             1 
   #define  PORT_SUSPEND            2 
   #define  PORT_OVER_CURRENT       3 
   #define  PORT_RESET              4 
   #define  PORT_POWER              8 
   #define  PORT_LOW_SPEED          9 
   #define  C_PORT_CONNECTION       16 
   #define  C_PORT_ENABLE           17 
   #define  C_PORT_SUSPEND          18 
   #define  C_PORT_OVER_CURRENT     19 
   #define  C_PORT_RESET            20

// hub/port status data structure
typedef struct _portStatus
{
   WORD     wPortStatus;      // (00) current port status flags
   WORD     wPortChange;      // (02) port status changed flags
                              // (04)
}  PortStatus;

   #define  STATUS_PORT_CONNECTION      0x0001   // device present on this port
   #define  STATUS_PORT_ENABLE          0x0002   // port is enabled
   #define  STATUS_PORT_SUSPEND         0x0004   // port is suspended
   #define  STATUS_PORT_OVER_CURRENT    0x0008   // over current condition exist on this port, power shut off
   #define  STATUS_PORT_RESET           0x0010   // reset signalling asserted
   #define  STATUS_PORT_POWER           0x0100   // port is powered on
   #define  STATUS_PORT_LOW_SPEED       0x0200   // low speed device attached to this port

   #define  USB_MAX_DEVICE_ADDRESS      127

   #define  USB_DEFAULT_PKT_SIZE        0
   #define  USB_DEFAULT_SRV_INTV        0
   #define  USB_DEFAULT_DEV_ADDR        0
   #define  USB_DEFAULT_CTRL_ENDPT      0
   #define  USB_MAX_ERROR_COUNT         3

   #define  USB_STANDARD_PKT_SIZE       8

   #define  USB_POWER_ON_DELAYT2        250      //   general power on delay time (miliseconds) // 02/17/2000 MB
   #define  USB_INSERTION_DELAY         250      //   insertion, power stabilaze delay time (miliseconds) // 02/17/2000 MB

// packet transmission time calculation constants
   #define  TTIME_FULLSPEED_NONISO_CDELAY    9107L
   #define  TTIME_FULLSPEED_ISOIN_CDELAY     7268L
   #define  TTIME_FULLSPEED_ISOUT_CDELAY     6265L
   #define  TTIME_LOWSPEED_IN_CDELAY         64060L
   #define  TTIME_LOWSPEED_OUT_CDELAY        64107L

   #define  TTIME_FULLSPEED_BITSTUFF_MUL     8354L
   #define  TTIME_LOWSPEED_BITSTUFF_IN_MUL   67667L
   #define  TTIME_LOWSPEED_BITSTUFF_OUT_MUL  66700L

   #define  TTIME_BITSTUFF_ADDCON            3167L
   #define  TTIME_BITSTUFF_MUL               9334L

// max frame transfer time (ns)
   #define  MAX_FRAME_TTIME                  1000000L

#endif

