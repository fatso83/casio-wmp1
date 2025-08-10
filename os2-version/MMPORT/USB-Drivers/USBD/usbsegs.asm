; SCCSID = src/dev/usb/USBD/USBSEGS.ASM, usb, c.basedd 98/07/10
;
;   Licensed Material -- Property of IBM
;
;   (c) Copyright IBM Corp. 1997, 1998  All Rights Reserved
;

        .XCREF
        .XLIST
         INCLUDE devhdr.inc
        .LIST
        .CREF

         EXTRN  _USBDStrategy:FAR
         EXTRN  _USBDidc:FAR

DDHEADER segment word public 'DDHEADER'
        EVEN
_headers    LABEL  WORD
            DW       -1                                         ; Device header pointer
            DW       -1                                         ; Device header pointer
            dw       DEV_CHAR_DEV OR DEVLEV_3                   ; ATTRIBUTE
            dw       OFFSET   _USBDStrategy                     ; POINTER TO STRATEGY ROUTINE
            dw       OFFSET   _USBDidc                          ; POINTER TO PDD-PDD IDC ROUTINE
            db       'USBD$   '                                 ; DEVICE NAME
            DW       0                                          ; Protect-mode CS Strategy Selector
            DW       0                                          ; Protect-mode DS selector
            DW       0                                          ; Real-mode CS Strategy Segment
            DW       0                                          ; Real-mode DS segment
            dd      DEV_ADAPTER_DD OR DEV_INITCOMPLETE          ; CAPABILITIES BIT STRIP
DDHeader ends

_DATA   segment word public 'DATA'
_DATA   ends

CONST   segment word public 'CONST'
CONST   ends

_BSS    segment word public 'BSS'
_BSS    ends

RMCode  segment word public 'CODE'
RMCode  ends

Code    segment word public 'CODE'
Code    ends

_TEXT   segment word public 'CODE'
_TEXT    ends

; Programmer cannot control location of CONST and _BSS class segments
; being grouped.  They are always last.  Do not put anything in these
; segments since they will be truncated after device driver initialization.
DGROUP  GROUP   DDHeader, CONST, _BSS, _DATA
CGROUP  GROUP   RMCode, Code, _TEXT
        end
        
