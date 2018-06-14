/* MMPORT - Casio WMP-1 - by Jimi (jimi@klinikum-konstanz.de) */
call RxFuncAdd "USBLoadFuncs","Usb4Rexx","USBLoadFuncs"
call USBLoadFuncs "quiet"
call RxFuncAdd "XtraLoadFuncs","XtraRexx","XtraLoadFuncs"
call XtraLoadFuncs "quiet"

/* Standard code */
Parse Arg MMPORT_Command MMPORT_CommandData
if Length(MMPORT_Command)\=4 Then Do
   say "This is an MMPORT Script. Please execute MMPORT Main."
   exit
End

/* Please forgive bad programming skills in here, b/c this still contains some */
/*  old code blocks - no time for cleaning up - they work anyway, so who cares */
say "DIAG Casio WMP-1 MMPORT Driver v1.0 - by Jimi"

MMPORT_ScratchFile   = "CasioWMP.tmp"

MMC_ID               = ""
MMC_OtherInfo        = ""
MMC_TotalSize        = 0
MMC_TotalFree        = 0
MMC_BiggestBlockFree = 0
MMC_TOC              = ""
MMC_TrackCount       = 0
MMC_TrackOffset.0    = 0
MMC_TrackSize.0      = 0
MMC_TrackTitle.0     = ""
MMC_TrackArtist.0    = ""
MMC_TrackAlbum.0     = ""
MMC_TrackAnimation.0 = ""

MMC_UsedSpace.0      = 0
MMC_UsedSpaceSize.0  = 0

/* Open and initialize device */
rc = USBOpen(DeviceHandle, "0x07CF", "0x3801", 0)
if rc\="DONE" Then Do
   say "NOHW"
   exit
End

Status = USBVendorMessageIn(DeviceHandle, 1, 4, 0, 4)
if C2D(SubStr(Status,2,1))=0 Then Do
   /* Device needs to get setup... */
   say "DIAG Setting Configuration..."
   rc = USBDeviceSetConfiguration(DeviceHandle, 1)
   if rc\="DONE" Then Do
      say "ERR! Failed to setup device"
      exit
   End
   Do Forever
      call XtraSleep 250
      Status = USBVendorMessageIn(DeviceHandle, 1, 4, 0, 4)
      if C2D(SubStr(Status,2,1))\=0 Then
         leave
   End
End
if C2D(SubStr(Status,2,1))=2 Then Do
   say "DIAG Watch needs to connect to dongle..."
   rc = USBVendorMessageIn(DeviceHandle, 1, 11, 0, 1)
   if rc\=D2C(1) Then Do
      say "ERR! Connecting to watch failed"
      exit
   End

   say "DIAG Connected, waiting for watch to settle... "
   SettleTimeout = 50
   Do Forever
      call XtraSleep 200
      rc = USBVendorMessageIn(DeviceHandle, 1, 4, 0, 4)
      say rc
      if rc=D2C(3)||D2C(3)||D2C(0)||D2C(0) Then
         leave
      SettleTimeout = SettleTimeout-1
      If SettleTimeout=0 Then Do
         say "DIAG Watch does not settle - unable to connect"
         say "NOHW"
         exit
      End
   End
End

MMC_ID = USBVendorMessageIn(DeviceHandle, 1, 9, 0, 16)
MMC_TotalSize = SubStr(MMC_ID,4,3)
Select
 When MMC_TotalSize="32M" Then Do
   say "DIAG Size of device is 32MB"
   MMC_TotalSize=32047104 /* No exactly 32M :-) */
 End
 Otherwise Do
   say "ERR! Size of device couldnt get read"
   exit
 End
End

/* Unknown contents in here. I don't know what this is for          */
/* MMC_OtherInfo = USBVendorMessageIn(DeviceHandle, 1, 10, 0, 16) */

call TOC_ReadFromWatch
call TOC_ParseIntoArray
say "DIAG Watch currently contains "MMC_TrackCount" Tracks"

/* We now got TOC in memory, so process MMPORT command */
Select
 When MMPORT_Command="TREE" Then Do
   CurPos = 1
   Do Forever
      if CurPos>MMC_TrackCount Then Leave
      CurID = D2X(MMC_TrackOffset.CurPos,8)
      say "LIST "CurID
      say "DLST "CurID' TITLE "'MMC_TrackTitle.CurPos'"'
      say "DLST "CurID' ARTIST "'MMC_TrackArtist.CurPos'"'
      say "DLST "CurID' ALBUM "'MMC_TrackAlbum.CurPos'"'
      CurPos = CurPos+1
   End
   call TOC_CalcFreeSpace
   say "FREE "MMC_TotalFree" "MMC_BiggestFreeBlock
 End
 When MMPORT_Command="DSET" Then Do
   Parse Value MMPORT_CommandData With ID " " DetailType ' "' String '"'
   /* Search for the ID */
   CurPos = TOC_SearchForID(ID)
   if CurPos=0 Then Do
      say "DIAG Unknown ID"
      exit
   End
   Select
    When DetailType="TITLE" Then  MMC_TrackTitle.CurPos = String
    When DetailType="ARTIST" Then MMC_TrackArtist.CurPos = String
    When DetailType="ALBUM" Then  MMC_TrackAlbum.CurPos = String
    Otherwise Do
      say "DIAG Unsupported DetailType"
      exit
    End
   End
   call TOC_GenerateFromArray
   call TOC_WriteToWatch ""
 End
 When MMPORT_Command="KILL" Then Do
   /* Search for the ID */
   CurPos = TOC_SearchForID(MMPORT_CommandData)
   if CurPos=0 Then Do
      say "DIAG Unknown ID"
      exit
   End

   /* Release that block first */
   call TOC_FreeSpace MMC_TrackOffset.CurPos

   if CurPos<MMC_TrackCount Then Do
      /* We are in the middle of the tracks, so rearrange */
      Do Forever
         if CurPos=MMC_TrackCount Then
            Leave

         OldPos = CurPos+1
         MMC_TrackOffset.CurPos    = MMC_TrackOffset.OldPos
         MMC_TrackSize.CurPos      = MMC_TrackSize.OldPos
         MMC_TrackTitle.CurPos     = MMC_TrackTitle.OldPos
         MMC_TrackArtist.CurPos    = MMC_TrackArtist.OldPos
         MMC_TrackAlbum.CurPos     = MMC_TrackAlbum.OldPos
         MMC_TrackAnimation.CurPos = MMC_TrackAnimation.OldPos

         CurPos = CurPos+1
      End
   End
   MMC_TrackCount            = MMC_TrackCount-1

   call TOC_GenerateFromArray
   call TOC_WriteToWatch ""
   call TOC_CalcFreeSpace
   say "FREE "MMC_TotalFree" "MMC_BiggestFreeBlock
 End
 When MMPORT_Command="UPLD" Then Do
   Parse Value MMPORT_CommandData With MediaType ' "' MP3_Filename '" ' MP3_Bitrate " " MP3_Samplerate " " MP3_Mode " "
   if MediaType\="MP3" Then Do
      say "ERR! Unsupported media"
      exit
   End
   if (MP3_Bitrate<96) | (MP3_Bitrate>128) | (MP3_Samplerate\=44100) Then Do
      say "ERR! Unsupported MP3"
      exit
   End
   MP3_Size = Stream(MP3_Filename,"c","query size")
   if MP3_Size=0 Then Do
      say "ERR! Invalid MP3"
      exit
   End
   NewOffset = TOC_FindSpace(MP3_Size)
   if NewOffset=0 Then Do
      say "NSPC"
      exit
   End
   call TOC_UseSpace NewOffset, MP3_Size
   MMC_TrackCount            = MMC_TrackCount+1
   CurPos                    = MMC_TrackCount
   MMC_TrackOffset.CurPos    = NewOffset
   MMC_TrackSize.CurPos      = MP3_Size
   MMC_TrackTitle.CurPos     = ""
   MMC_TrackArtist.CurPos    = ""
   MMC_TrackAlbum.CurPos     = ""
   MMC_TrackAnimation.CurPos = ""

   /* Now first upload the MP3 */
   say "DIAG Uploading MP3 (Size "MP3_Size")..."
   MP3_Data = CharIn(MP3_Filename,1,MP3_Size)
   call Stream MP3_Filename, "c", "close"
   if Length(MP3_Data)<>MP3_Size Then Do
      say "ERR! Could not read MP3"
      exit
   End
   if MMC_Write(NewOffset, MP3_Data) Then Do
      MP3_Data = ""
      /* Finally update TOC */
      call TOC_GenerateFromArray
      call TOC_WriteToWatch ""
      call TOC_CalcFreeSpace
      say "IDCH "D2X(NewOffset,8)
      say "FREE "MMC_TotalFree" "MMC_BiggestFreeBlock
   End
 End
 When MMPORT_Command="DNLD" Then Do
   Parse Value MMPORT_CommandData With ID ' "' MP3_Filename '"'

   /* Search for the ID */
   CurPos = TOC_SearchForID(ID)
   if CurPos=0 Then Do
      say "DIAG Unknown ID"
      exit
   End
   say "DIAG Downloading MP3 (Size "MMC_TrackSize.CurPos")..."
   MP3_Data = MMC_Read(MMC_TrackOffset.CurPos, MMC_TrackSize.CurPos)
   call CharOut MP3_Filename, MP3_Data, 1; MP3_Data = ""
 End
 When MMPORT_Command="SRTA" Then Do
   Parse Value MMPORT_CommandData With SortID ' ' RelativeID

   if SortID=RelativeID Then Do
      say "DIAG Duplicate IDs"
      exit
   End

   /* Search for the IDs */
   EntryPos    = TOC_SearchForID(SortID)
   RelativePos = TOC_SearchForID(RelativeID)
   if (EntryPos=0) | (RelativePos=0) Then Do
      say "DIAG Unknown ID"
      exit
   End

   if EntryPos=RelativePos+1 Then Do
      say "DIAG Already at that position"
      exit
   End

   /* Save Entry that is getting sorted */
   MMC_TrackOffset.0    = MMC_TrackOffset.EntryPos
   MMC_TrackSize.0      = MMC_TrackSize.EntryPos
   MMC_TrackTitle.0     = MMC_TrackTitle.EntryPos
   MMC_TrackArtist.0    = MMC_TrackArtist.EntryPos
   MMC_TrackAlbum.0     = MMC_TrackAlbum.EntryPos
   MMC_TrackAnimation.0 = MMC_TrackAnimation.EntryPos

   if RelativePos>EntryPos Then Do
      /* New position is above old position */
      NewPos    = RelativePos
      SourcePos = EntryPos+1
      DestinPos = EntryPos
      Do Forever
         if DestinPos=NewPos Then
            Leave
         MMC_TrackOffset.DestinPos    = MMC_TrackOffset.SourcePos
         MMC_TrackSize.DestinPos      = MMC_TrackSize.SourcePos
         MMC_TrackTitle.DestinPos     = MMC_TrackTitle.SourcePos
         MMC_TrackArtist.DestinPos    = MMC_TrackArtist.SourcePos
         MMC_TrackAlbum.DestinPos     = MMC_TrackAlbum.SourcePos
         MMC_TrackAnimation.DestinPos = MMC_TrackAlbum.SourcePos
         SourcePos = SourcePos+1
         DestinPos = DestinPos+1
      End
   End
     else Do
      /* New position is below old position */
      NewPos    = RelativePos+1
      SourcePos = EntryPos-1
      DestinPos = EntryPos
      Do Forever
         if DestinPos=NewPos Then
            Leave
         MMC_TrackOffset.DestinPos    = MMC_TrackOffset.SourcePos
         MMC_TrackSize.DestinPos      = MMC_TrackSize.SourcePos
         MMC_TrackTitle.DestinPos     = MMC_TrackTitle.SourcePos
         MMC_TrackArtist.DestinPos    = MMC_TrackArtist.SourcePos
         MMC_TrackAlbum.DestinPos     = MMC_TrackAlbum.SourcePos
         MMC_TrackAnimation.DestinPos = MMC_TrackAlbum.SourcePos
         SourcePos = SourcePos-1
         DestinPos = DestinPos-1
      End
   End

   /* Now set the actual entry to the new position */
   MMC_TrackOffset.NewPos    = MMC_TrackOffset.0
   MMC_TrackSize.NewPos      = MMC_TrackSize.0
   MMC_TrackTitle.NewPos     = MMC_TrackTitle.0
   MMC_TrackArtist.NewPos    = MMC_TrackArtist.0
   MMC_TrackAlbum.NewPos     = MMC_TrackAlbum.0
   MMC_TrackAnimation.NewPos = MMC_TrackAnimation.0

   /* Finally update TOC */
   call TOC_GenerateFromArray
   call TOC_WriteToWatch ""
   say "DIAG Sorting successful"
 End
 When MMPORT_Command="SRTB" Then Do
   Parse Value MMPORT_CommandData With SortID ' ' RelativeID ' '

   if SortID=RelativeID Then Do
      say "DIAG Duplicate IDs"
      exit
   End

   /* Search for the IDs */
   EntryPos    = TOC_SearchForID(SortID)
   RelativePos = TOC_SearchForID(RelativeID)
   if (EntryPos=0) | (RelativePos=0) Then Do
      say "DIAG Unknown ID"
      exit
   End

   if EntryPos=RelativePos-1 Then Do
      say "DIAG Already at that position"
      exit
   End

   /* Save Entry that is getting sorted */
   MMC_TrackOffset.0    = MMC_TrackOffset.EntryPos
   MMC_TrackSize.0      = MMC_TrackSize.EntryPos
   MMC_TrackTitle.0     = MMC_TrackTitle.EntryPos
   MMC_TrackArtist.0    = MMC_TrackArtist.EntryPos
   MMC_TrackAlbum.0     = MMC_TrackAlbum.EntryPos
   MMC_TrackAnimation.0 = MMC_TrackAnimation.EntryPos

   if RelativePos>EntryPos Then Do
      /* New position is above old position */
      NewPos    = RelativePos-1
      SourcePos = EntryPos+1
      DestinPos = EntryPos
      Do Forever
         if DestinPos=NewPos Then
            Leave
         MMC_TrackOffset.DestinPos    = MMC_TrackOffset.SourcePos
         MMC_TrackSize.DestinPos      = MMC_TrackSize.SourcePos
         MMC_TrackTitle.DestinPos     = MMC_TrackTitle.SourcePos
         MMC_TrackArtist.DestinPos    = MMC_TrackArtist.SourcePos
         MMC_TrackAlbum.DestinPos     = MMC_TrackAlbum.SourcePos
         MMC_TrackAnimation.DestinPos = MMC_TrackAlbum.SourcePos
         SourcePos = SourcePos+1
         DestinPos = DestinPos+1
      End
   End
     else Do
      /* New position is below old position */
      NewPos    = RelativePos
      SourcePos = EntryPos-1
      DestinPos = EntryPos
      Do Forever
         if DestinPos=NewPos Then
            Leave
         MMC_TrackOffset.DestinPos    = MMC_TrackOffset.SourcePos
         MMC_TrackSize.DestinPos      = MMC_TrackSize.SourcePos
         MMC_TrackTitle.DestinPos     = MMC_TrackTitle.SourcePos
         MMC_TrackArtist.DestinPos    = MMC_TrackArtist.SourcePos
         MMC_TrackAlbum.DestinPos     = MMC_TrackAlbum.SourcePos
         MMC_TrackAnimation.DestinPos = MMC_TrackAlbum.SourcePos
         SourcePos = SourcePos-1
         DestinPos = DestinPos-1
      End
   End

   /* Now set the actual entry to the new position */
   MMC_TrackOffset.NewPos    = MMC_TrackOffset.0
   MMC_TrackSize.NewPos      = MMC_TrackSize.0
   MMC_TrackTitle.NewPos     = MMC_TrackTitle.0
   MMC_TrackArtist.NewPos    = MMC_TrackArtist.0
   MMC_TrackAlbum.NewPos     = MMC_TrackAlbum.0
   MMC_TrackAnimation.NewPos = MMC_TrackAnimation.0

   /* Finally update TOC */
   call TOC_GenerateFromArray
   call TOC_WriteToWatch ""
 End
 When MMPORT_Command="FLSH" Then Do
  call TOC_WriteToWatch "FORCED"
  say "DIAG flushing content"
 End
 Otherwise Do
  say "NSUP"
 End
End
exit




MMC_Read:
   Procedure Expose DeviceHandle
   Parse Arg Offset, Length

   TempNum = C2D(BitAnd(D2C(Length,4),D2C(511,4)))
   if TempNum\=0 Then
      TransferLength = Length+(512-TempNum)
     else
      TransferLength = Length

   GetSetup = D2C(0,4)||REVERSE(D2C(Offset,4))||REVERSE(D2C(Offset+TransferLength,4))||REVERSE(D2C(TransferLength,4))
   rc = USBVendorMessageOut(DeviceHandle, 1, 1, 0, GetSetup)
   if rc="DONE" Then Do
      DataBuffer = USBBulkRead(DeviceHandle, 0, 130, TransferLength)
      if Length(DataBuffer)\=TransferLength Then Do
         say "MMC Reading failed"
         exit
      End
      return Left(DataBuffer, Length)
   End
   return ""

MMC_Write:
   Procedure Expose DeviceHandle
   Parse Arg Offset, DataBuffer

   Length = Length(DataBuffer)
   TempNum = C2D(BitAnd(D2C(Length,4),D2C(511,4)))
   if TempNum\=0 Then Do
      Length = Length+(512-TempNum)
      DataBuffer = DataBuffer||Copies(D2C(0),(512-TempNum))
   End

   /* Get status from Device */
   Status = USBVendorMessageIn(DeviceHandle, 1, 4, 0, 4)

   PutSetup = D2C(0,4)||REVERSE(D2C(Offset,4))||REVERSE(D2C(Offset+Length,4))||REVERSE(D2C(Length,4))
   rc = USBVendorMessageOut(DeviceHandle, 1, 0, 0, PutSetup)
   if rc="DONE" Then Do
      call USBBulkWrite DeviceHandle, 0, 1, DataBuffer
      return 1
   End
   return 0

TOC_ReadFromWatch:
   /* Get from scratch file, if possible */
   if Length(MMPORT_ScratchFile)>0 Then Do
      if Stream(MMPORT_ScratchFile,"c","query size")=16896 Then Do
         MMC_TOC       = CharIn(MMPORT_ScratchFile,,16896)
         call Stream MMPORT_ScratchFile, "c", "close"
         return
      End
   End
   MMC_TOC       = MMC_Read(0, 16896)
   return

TOC_WriteToWatch:
   Parse Arg IsForced
   if (Length(MMPORT_ScratchFile)=0) | (IsForced="FORCED") Then
      call MMC_Write 0, MMC_TOC
     else Do
      call CharOut MMPORT_ScratchFile, MMC_TOC, 1
      call Stream MMPORT_ScratchFile, "c", "close"
   End
   say "DIAG TOC updated"
   return

TOC_ParseIntoArray:
   MMC_TrackCount = C2D(REVERSE(SUBSTR(MMC_TOC,5,4)))
   CurPos = 513; TrackNo = 0
   Do Forever
      TrackNo = TrackNo+1
      if TrackNo>MMC_TrackCount Then
         Leave
      MMC_TrackOffset.TrackNo    = C2D(REVERSE(SUBSTR(MMC_TOC,CurPos,4)))
      MMC_TrackSize.TrackNo      = C2D(REVERSE(SUBSTR(MMC_TOC,CurPos+4,4)))
      MMC_TrackTitle.TrackNo     = STRIP(SUBSTR(MMC_TOC,CurPos+8,32),Trailing,D2C(0))
      MMC_TrackArtist.TrackNo    = STRIP(SUBSTR(MMC_TOC,CurPos+40,32),Trailing,D2C(0))
      MMC_TrackAlbum.TrackNo     = STRIP(SUBSTR(MMC_TOC,CurPos+72,32),Trailing,D2C(0))
      MMC_TrackAnimation.TrackNo = SUBSTR(MMC_TOC,CurPos+8192,128)

      /* Mark this block as used */
      call TOC_UseSpace MMC_TrackOffset.TrackNo, MMC_TrackSize.TrackNo

      CurPos = CurPos+128
      /* We processed 8192 TOC data... */
      if CurPos>8704 Then Leave
   End
   return
   CurPos = 1
   Do Forever
      if CurPos>MMC_TrackCount Then Leave
      say MMC_UsedSpace.CurPos" ("MMC_UsedSpaceSize.CurPos") -> "MMC_UsedSpace.CurPos+MMC_UsedSpaceSize.CurPos
      CurPos = CurPos+1
   End
   return

TOC_GenerateFromArray:
   say "DIAG Updating TOC..."
   /* Update counters in TOC-Header */
   MMC_TOC = Copies(D2C(0),4)||REVERSE(D2C(MMC_TrackCount,4))||Copies(D2C(0),2)||REVERSE(D2C(MMC_TrackCount,4))||Copies(D2C(0),2)||SubStr(MMC_TOC,17,496)

   CurPos = 1
   Do Forever
      if CurPos>MMC_TrackCount Then Leave

      if Length(MMC_TrackTitle.CurPos)>31 Then
         MMC_TrackTitle.CurPos = Left(MMC_TrackTitle.CurPos,31)
      if Length(MMC_TrackArtist.CurPos)>31 Then
         MMC_TrackArtist.CurPos = Left(MMC_TrackArtist.CurPos,31)
      if Length(MMC_TrackArtist.CurPos)>31 Then
         MMC_TrackAlbum.CurPos = Left(MMC_TrackAlbum.CurPos,31)
      TrackEntry = REVERSE(D2C(MMC_TrackOffset.CurPos,4))||REVERSE(D2C(MMC_TrackSize.CurPos,4))
      TrackEntry = TrackEntry||MMC_TrackTitle.CurPos||Copies(D2C(0),32-Length(MMC_TrackTitle.CurPos))
      TrackEntry = TrackEntry||MMC_TrackArtist.CurPos||Copies(D2C(0),32-Length(MMC_TrackArtist.CurPos))
      TrackEntry = TrackEntry||MMC_TrackAlbum.CurPos||Copies(D2C(0),32-Length(MMC_TrackAlbum.CurPos))
      TrackEntry = TrackEntry||D2C(0,4)
      TrackEntry = TrackEntry||Copies(D2C(0),20)
      
      /* Add this track to TOC */
      MMC_TOC = MMC_TOC||TrackEntry

      CurPos = CurPos+1
   End
   /* Enlarge to get 8192 TOC */
   MMC_TOC = MMC_TOC||Copies(D2C(0),128*(64-MMC_TrackCount))

   CurPos = 1
   Do Forever
      if CurPos>MMC_TrackCount Then Leave
      if Length(MMC_TrackAnimation.CurPos)=128 Then
         MMC_TOC = MMC_TOC||MMC_TrackAnimation.CurPos
        else
         MMC_TOC = MMC_TOC||Copies(D2C(0),128)

      CurPos = CurPos+1
   End
   /* Enlarge to get 16384 TOC */
   MMC_TOC       = MMC_TOC||Copies(D2C(0),128*(64-MMC_TrackCount))
   return

TOC_SearchForID:
   Parse Arg SearchID
   CurPos = 1
   Do Forever
      if CurPos>MMC_TrackCount Then Leave
      CurID = D2X(MMC_TrackOffset.CurPos,8)
      if CurID=SearchID Then
         return CurPos
      CurPos = CurPos+1
   End
   return 0

TOC_UseSpace:
   Parse Arg ItemOffset, ItemLength
   InjectSpace = 1; TotalEntries = MMC_UsedSpace.0
   Do Forever
      if InjectSpace>TotalEntries Then
         Leave
      if ItemOffset<MMC_UsedSpace.InjectSpace Then
         Leave
      InjectSpace = InjectSpace+1
   End
   NewSpace = TotalEntries+1
   OldSpace = TotalEntries
   Do Forever
      if NewSpace=InjectSpace Then
         Leave
      MMC_UsedSpace.NewSpace     = MMC_UsedSpace.OldSpace
      MMC_UsedSpaceSize.NewSpace = MMC_UsedSpaceSize.OldSpace
      NewSpace = NewSpace-1
      OldSpace = OldSpace-1
   End
   MMC_UsedSpace.0 = MMC_UsedSpace.0+1
   MMC_UsedSpace.InjectSpace = ItemOffset
   /* Is ItemLength a multiple of 512? */
   TempNum = C2D(BitAnd(D2C(ItemLength,4),D2C(511,4)))
   if TempNum\=0 Then
      ItemLength = ItemLength+(512-TempNum)
   MMC_UsedSpaceSize.InjectSpace = ItemLength
   return

TOC_FreeSpace:
   Parse Arg ItemOffset
   CurSpace = 1; TotalEntries = MMC_UsedSpace.0
   Do Forever
      if CurSpace>TotalEntries Then
         return
      if ItemOffset=MMC_UsedSpace.CurSpace Then
         Leave
      CurSpace = CurSpace+1
   End
   NewSpace = CurSpace
   OldSpace = CurSpace+1
   Do Forever
      if NewSpace=TotalEntries Then
         Leave
      MMC_UsedSpace.NewSpace     = MMC_UsedSpace.OldSpace
      MMC_UsedSpaceSize.NewSpace = MMC_UsedSpaceSize.OldSpace
      NewSpace = NewSpace+1
      OldSpace = OldSpace+1
   End
   MMC_UsedSpace.0 = MMC_UsedSpace.0-1
   return

TOC_FindSpace:
   Parse Arg ItemLength
   EndOffset    = 16896
   NextOffset   = 0
   HitOffset    = 0
   HitFreeSpace = 0
   ItemLength   = ItemLength-1
   CurSpace     = 1; TotalEntries = MMC_UsedSpace.0
   Do Forever
      if CurSpace>TotalEntries Then
         NextOffset = MMC_TotalSize
        else
         NextOffset = MMC_UsedSpace.CurSpace
      FreeSpace  = NextOffset-EndOffset
      if FreeSpace>ItemLength Then
         if (HitFreeSpace=0) | (HitFreeSpace>FreeSpace) Then Do
            HitOffset    = EndOffset
            HitFreeSpace = FreeSpace
         End
      if CurSpace>MMC_TrackCount Then Leave
      EndOffset = NextOffset+MMC_UsedSpaceSize.CurSpace
      CurSpace = CurSpace+1
   End
   /* Reply offset of best Hit */
   return HitOffset

TOC_CalcFreeSpace:
   MMC_TotalFree = 0; MMC_BiggestFreeBlock = 0
   EndOffset = 16896
   CurSpace  = 1; TotalEntries = MMC_UsedSpace.0
   Do Forever
      if CurSpace>TotalEntries Then
         NextOffset = MMC_TotalSize
        else
         NextOffset = MMC_UsedSpace.CurSpace
      FreeSpace  = NextOffset-EndOffset
      if FreeSpace>MMC_BiggestFreeBlock Then
         MMC_BiggestFreeBlock = FreeSpace
      MMC_TotalFree = MMC_TotalFree+FreeSpace
      if CurSpace>MMC_TrackCount Then Leave
      EndOffset = NextOffset+MMC_UsedSpaceSize.CurSpace
      CurSpace = CurSpace+1
   End
   return
