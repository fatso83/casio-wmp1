
#ifdef __cplusplus
  extern "C" {
#endif

// Put these into an USB.h file

#define USB_CLASS_PER_INTERFACE   0 /* for DeviceClass */
#define USB_CLASS_AUDIO     1
#define USB_CLASS_COMM      2
#define USB_CLASS_HID     3
#define USB_CLASS_PRINTER   7
#define USB_CLASS_MASS_STORAGE    8
#define USB_CLASS_HUB     9
#define USB_CLASS_DATA      10
#define USB_CLASS_VENDOR_SPEC   0xff

/*
 * USB types
 */
#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

/*
 * USB recipients
 */
#define USB_RECIP_DEVICE      0x00
#define USB_RECIP_INTERFACE   0x01
#define USB_RECIP_ENDPOINT    0x02
#define USB_RECIP_OTHER       0x03

/*
 * USB directions
 */
#define USB_DIR_OUT     0
#define USB_DIR_IN      0x80


typedef ULONG USBHANDLE, *PUSBHANDLE;
typedef ULONG USBNOTIFY, *PUSBNOTIFY;
typedef ULONG ISOHANDLE, *PISOHANDLE;

#define USB_NOT_INIT 8000
#define USB_ERROR_NO_MORE_NOTIFICATIONS 8001
#define USB_ERROR_OUTOF_RESOURCES 8002
#define USB_ERROR_INVALID_ENDPOINT 8003

#define USB_ANY_PRODUCTVERSION 0xFFFF
#define USB_OPEN_FIRST_UNUSED 0

#ifdef USB_BIND_DYNAMIC
  typedef APIRET APIENTRY USBREGISTERDEVICENOTIFICATION( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved,
                                                 USHORT usVendor,
                                                 USHORT usProduct,
                                                 USHORT usBCDVersion);
  typedef USBREGISTERDEVICENOTIFICATION *PUSBREGISTERDEVICENOTIFICATION;

  typedef APIRET APIENTRY USBDEREGISTERNOTIFICATION( USBNOTIFY NotifyID);

  typedef USBDEREGISTERNOTIFICATION *PUSBDEREGISTERNOTIFICATION;

  typedef APIRET APIENTRY USBOPEN( PUSBHANDLE pHandle,
                           USHORT usVendor,
                           USHORT usProduct,
                           USHORT usBCDDevice,
                           USHORT usEnumDevice);
  typedef USBOPEN *PUSBOPEN;
  typedef APIRET APIENTRY USBCLOSE( USBHANDLE Handle);
  typedef USBCLOSE *PUSBCLOSE;

  typedef APIRET APIENTRY USBCTRLMESSAGE( USBHANDLE Handle,
                                  UCHAR  ucRequestType,
                                  UCHAR  ucRequest,
                                  USHORT usValue,
                                  USHORT usIndex,
                                  USHORT usLength,
                                  UCHAR  *pData,
                                  ULONG  ulTimeout);
  typedef USBCTRLMESSAGE *PUSBCTRLMESSAGE;
#else
  APIRET APIENTRY UsbQueryNumberDevices( ULONG *pulNumDev);

  APIRET APIENTRY UsbQueryDeviceReport( ULONG ulDevNumber,
                                        ULONG *ulBufLen,
                                        CHAR *pData);
  APIRET APIENTRY UsbRegisterChangeNotification( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved);

  APIRET APIENTRY UsbRegisterDeviceNotification( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved,
                                                 USHORT usVendor,
                                                 USHORT usProduct,
                                                 USHORT usBCDVersion);

  APIRET APIENTRY UsbDeregisterNotification( USBNOTIFY NotifyID);

  APIRET APIENTRY UsbOpen( PUSBHANDLE pHandle,
                           USHORT usVendor,
                           USHORT usProduct,
                           USHORT usBCDDevice,
                           USHORT usEnumDevice);

  APIRET APIENTRY UsbClose( USBHANDLE Handle);

  APIRET APIENTRY UsbCtrlMessage( USBHANDLE Handle,
                                  UCHAR  ucRequestType,
                                  UCHAR  ucRequest,
                                  USHORT usValue,
                                  USHORT usIndex,
                                  USHORT usLength,
                                  UCHAR  *pData,
                                  ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkRead( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               ULONG  *ulNumBytes,
                               UCHAR  *pData,
                               ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkWrite( USBHANDLE Handle,
                                UCHAR  Endpoint,
                                UCHAR  Interface,
                                ULONG  ulNumBytes,
                                UCHAR  *pData,
                                ULONG  ulTimeout);

  APIRET APIENTRY UsbIrqStart( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               USHORT usNumBytes,
                               UCHAR  *pData,
                               PHEV   pHevModified);
  APIRET APIENTRY UsbIrqStop(  USBHANDLE Handle,
                               HEV       HevModified);

  APIRET APIENTRY UsbIsoStart( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               ISOHANDLE *phIso);
  APIRET APIENTRY UsbIsoStop( ISOHANDLE hIso);

  APIRET APIENTRY UsbIsoDequeue( ISOHANDLE hIso,
                                 UCHAR * pBuffer,
                                 ULONG ulNumBytes);
  APIRET APIENTRY UsbIsoEnqueue( ISOHANDLE hIso,
                                 const UCHAR * pBuffer,
                                 ULONG ulNumBytes);
  APIRET APIENTRY UsbIsoPeekQueue( ISOHANDLE hIso,
                                   UCHAR * pByte,
                                   ULONG ulOffset);
  APIRET APIENTRY UsbIsoGetLength( ISOHANDLE hIso,
                                   ULONG *pulLength);


  // Standard USB Requests See 9.4. in USB 1.1. spec.

  #define UsbDeviceClearFeature(HANDLE, FEAT)             UsbCtrlMessage(HANDLE, 0x00, 0x01, FEAT, 0, 0, NULL, 0)
  #define UsbInterfaceClearFeature(HANDLE, IFACE, FEAT)   UsbCtrlMessage(HANDLE, 0x01, 0x01, FEAT, IFACE, 0, NULL, 0)
  #define UsbEndpointClearFeature(HANDLE, ENDPOINT, FEAT) UsbCtrlMessage(HANDLE, 0x02, 0x01, FEAT, ENDPOINT, 0, NULL, 0)
  #define FEATURE_DEVICE_REMOTE_WAKEUP 1
  #define FEATURE_ENDPOINT_HALT 0
  #define UsbEndpointClearHalt(HANDLE, ENDPOINT) UsbEndpointClearFeature(HANDLE, ENDPOINT, FEATURE_ENDPOINT_HALT)

  #define UsbDeviceGetConfiguration(HANDLE, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x08, 0, 0, 1, DATA, 0)

  // 09 01 2003 - KIEWITZ
  #define UsbDeviceGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)        UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0100|INDEX), LID, LEN, DATA, 0)
  #define UsbInterfaceGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)     UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0400|INDEX), LID, LEN, DATA, 0)
  #define UsbEndpointGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)      UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0500|INDEX), LID, LEN, DATA, 0)
  #define UsbConfigurationGetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0200|INDEX), LID, LEN, DATA, 0)
  #define UsbStringGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)        UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0300|INDEX), LID, LEN, DATA, 0)

  #define UsbInterfaceGetAltSetting(HANDLE, IFACE, SETTING)  UsbCtrlMessage(HANDLE, 0x81, 0x0A, 0, IFACE, 1, SETTING, 0)

  #define UsbDeviceGetStatus(HANDLE, STATUS)                UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, 0, 2, STATUS, 0)
  #define UsbInterfaceGetStatus(HANDLE, IFACE, STATUS)      UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, IFACE, 2, STATUS, 0)
  #define UsbEndpointGetStatus(HANDLE, ENDPOINT, STATUS)  UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, ENDPOINT, 2, STATUS, 0)
  #define STATUS_ENDPOINT_HALT       0x0001
  #define STATUS_DEVICE_SELFPOWERD   0x0001
  #define STATUS_DEVICE_REMOTEWAKEUP 0x0002

  #define UsbDeviceSetAddress(HANDLE, ADDRESS)      UsbCtrlMessage(HANDLE, 0x80, 0x05, ADDRESS, 0, 0, NULL, 0)
  #define UsbDeviceSetConfiguration(HANDLE, CONFIG) UsbCtrlMessage(HANDLE, 0x00, 0x09, CONFIG, 0, 0, NULL, 0)

  // 09 01 2003 - KIEWITZ
  #define UsbDeviceSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0100|INDEX), LID, LEN, DATA, 0)
  #define UsbInterfaceSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0400|INDEX), LID, LEN, DATA, 0)
  #define UsbEndpointSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0500|INDEX), LID, LEN, DATA, 0)
  #define UsbConfigurationSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0200|INDEX), LID, LEN, DATA, 0)
  #define UsbStringSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0300|INDEX), LID, LEN, DATA, 0)

  #define UsbDeviceSetFeature(HANDLE, FEAT)             UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, 0, 0, NULL, 0)
  #define UsbInterfaceSetFeature(HANDLE, IFACE, FEAT)   UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, IFACE, 0, NULL, 0)
  #define UsbEndpointSetFeature(HANDLE, ENDPOINT, FEAT) UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, ENDPOINT, 0, NULL, 0)

  #define UsbInterfaceSetAltSetting(HANDLE, IFACE, ALTSET) UsbCtrlMessage(HANDLE, 0x01, 0x0B, ALTSET, IFACE, 0, NULL, 0)

  #define UsbEndpointSynchFrame(HANDLE, ENDPOINT, FRAMENUM) UsbCtrlMessage(HANDLE, 0x82, 0x0B, 0, ENDPOINT, 2, FRAMENUM, 0)
#endif


#ifdef __cplusplus
}
#endif

