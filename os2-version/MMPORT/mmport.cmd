/* MMPORT Main - by Jimi */
/* INCLUDE filewait */
/* INCLUDE error-handler */

call RxFuncAdd "SysLoadFuncs", "REXXUTIL", "SysLoadFuncs"
call SysLoadFuncs
call RxFuncAdd "XtraLoadFuncs","XtraRexx","XtraLoadFuncs"
call XtraLoadFuncs "quiet"
call RxFuncAdd "mciRxInit", "MCIAPI", "mciRxInit"

signal on syntax name ErrorHandler
signal on halt name ErrorHalt

say "MMPORT - Multimedia Portable/2 - by Jimi"

If XtraVersion()<"0.40" Then Do
   say "XtraRexx v0.40+ required."
   exit
End

Parse Arg Command ' ' Parameter

MMPORT_TrackCount = 0
MMPORT_Tracks.0   = 0
MMPORT_Title.0    = 0
MMPORT_Artist.0   = 0
MMPORT_NewID      = ""

MMPORT_Directory  = DIRECTORY()
MMPORT_TOCCache   = MMPORT_Directory"\TOCCache\"
MMPORT_FreeSpace  = 0
MMPORT_BiggestFreeBlock = 0
MMPORT_NoHardware = "TRUE"

Select
 When Command="CREATEICONS" Then Do
   call SysCreateObject "WPProgram", "MMPORT Upload", "<WP_DESKTOP>", "EXENAME="DIRECTORY()"\MMPORT.cmd;STARTUPDIR="DIRECTORY()";PARAMETERS=UPLD;OBJECTID=<MMPORT_UPLOAD>;ICONFILE="DIRECTORY()"\icons\basket.ico","UPDATE"
   call SysCreateObject "WPProgram", "MMPORT Commandline", "<WP_DESKTOP>", "EXENAME="DIRECTORY()"\MMPORT.cmd;STARTUPDIR="DIRECTORY()";PARAMETERS=CMDLINE;OBJECTID=<MMPORT_CMDLINE>;ICONFILE="DIRECTORY()"\icons\mmport.ico","UPDATE"
   say "Objects created/updated."
 End
 When Command="QUEUE" Then Do
   call MMPORT_ProcessQueue
 End
 When Command="QUEUEANIMATION" Then Do
   call MMPORT_ProcessQueueAnimation
 End
 When Command="UPLD" Then Do
   call MMPORT_EnqueueCommand Command, Parameter
 End
 When Command="DNLD" Then Do
   call MMPORT_EnqueueCommand Command, Parameter
 End
 When Command="CMDLINE" Then Do
   say "Command line requested"
   QueueWasActive = 0
   If LENGTH(STREAM(MMPORT_Directory"\QueueActive","c","query exists"))=0 Then Do
      /* If no queue currently active -> run TREE command */
      call MMPORT_EnqueueCommand "TREE", ""
      /* and wait for queue to finish */
      say "Waiting for Queue to finish reading TOC..."
      call XtraSleep 250
      Do Forever
         If LENGTH(STREAM(MMPORT_Directory"\QueueActive","c","query exists"))=0 Then
            Leave
         call XtraSleep 50
      End
   End
     else Do
      QueueWasActive = 1
   End
      
   Command   = "TREE"
   Parameter = ""
   Do Forever
      If Length(Command)=4 Then Do
         Command = Translate(Command)
         Select
          When Command="EXIT" Then
            exit
          When Command="QUIT" Then
            exit
          When Command="HELP" Then Do
            say ""
            say "  TREE            - displays all tracks on connected device"
            say "  UPLD [filename] - uploads file to device"
            say "  DNLD [filename] - downloads file from device"
            say "  DSET [track] [type]"
            say "        [string]  - sets track detail information"
            say "  KILL [track]    - deletes track on device"
            say "  SRTA [track] [track-before]"
            say "                  - sets track after [track-before]"
            say "  SRTB [track] [track-after]"
            say "                  - sets track before [track-after]"
            say "  EXIT/QUIT       - exit MMPORT/2 commandline"
          End
          When Command="TREE" Then Do
            If MMPORT_CheckHardware()="NOHW" Then Do
               call MMPORT_EnqueueCommand Command, Parameter
               call XtraSleep 250
               Do Forever
                  If LENGTH(STREAM(MMPORT_Directory"\QueueActive","c","query exists"))=0 Then
                     Leave
                  call XtraSleep 50
               End
            End
            call MMPORT_ReadTOCCache
            say ""
            If MMPORT_CheckHardware()="NOHW" Then Do
               say "Compatible device not connected"
            End
              else Do
               If MMPORT_TrackCount>0 Then Do
                  Do CurTrack=1 To MMPORT_TrackCount
                     say "Track "CurTrack" - "MMPORT_TrackTitle.CurTrack" / "MMPORT_TrackArtist.CurTrack
                  End
               End
                 else Do
                  say "No tracks on connected device"
               End
            End
/*            say " - Free space: "MMPORT_FreeSpace" ("MMPORT_BiggestFreeBlock")" */
            say ""
          End
          When Command="UPLD" Then Do
            call MMPORT_EnqueueCommand Command, Parameter
          End
          When Command="DNLD" Then Do
            Parse Value Parameter With TrackNo " " FileName
            if DataType(TrackNo)="NUM" Then Do
               if (TrackNo>0) & (TrackNo-1<MMPORT_TrackCount) Then Do
                  TrackID = MMPORT_Tracks.TrackNo
                  call MMPORT_EnqueueCommand Command, TrackID" "FileName
               End
                else say "This track does not exist"
            End
             else say "Invalid track number"
          End
          When Command="DSET" Then Do
            Parse Value Parameter With TrackNo " " DetailType ' "' String '"'
            if DataType(TrackNo)="NUM" Then Do
               if (TrackNo>0) & (TrackNo-1<MMPORT_TrackCount) Then Do
                  If Length(DetailType)>0 Then Do
                     TrackID = MMPORT_Tracks.TrackNo
                     call MMPORT_EnqueueCommand Command, TrackID' 'DetailType' "'String'"'
                     say "Command is being processed"
                  End
                    else
                     say "No detail type given"
               End
                else say "This track does not exist"
            End
             else say "Invalid track number"
          End
          When Command="KILL" Then Do
            if DataType(Parameter)="NUM" Then Do
               if (Parameter>0) & (Parameter-1<MMPORT_TrackCount) Then Do
                  TrackID = MMPORT_Tracks.Parameter
                  call MMPORT_EnqueueCommand Command, TrackID
               End
                else say "This track does not exist"
            End
             else say "Invalid track number"
          End
          When Command="SRTA" Then Do
            Parse Value Parameter With TrackNo " " SortTrackNo
            if DataType(TrackNo)="NUM" Then Do
               if DataType(SortTrackNo)="NUM" Then Do
                  if (TrackNo>0) & (TrackNo-1<MMPORT_TrackCount) Then Do
                     if (SortTrackNo>0) & (SortTrackNo-1<MMPORT_TrackCount) Then Do
                        if TrackNo<>SortTrackNo Then Do
                           if TrackNo<>SortTrackNo+1 Then Do
                              TrackID     = MMPORT_Tracks.TrackNo
                              SortTrackID = MMPORT_Tracks.SortTrackNo
                              call MMPORT_EnqueueCommand Command, TrackID" "SortTrackID
                           End
                            else say "Order would not change, operation aborted"
                        End
                         else say "Track numbers are the same"
                     End
                      else say "This track does not exist"
                  End
                   else say "This track does not exist"
               End
                else say "Invalid track number"
            End
             else say "Invalid track number"
          End
          When Command="SRTB" Then Do
            Parse Value Parameter With TrackNo " " SortTrackNo
            if DataType(TrackNo)="NUM" Then Do
               if DataType(SortTrackNo)="NUM" Then Do
                  if (TrackNo>0) & (TrackNo-1<MMPORT_TrackCount) Then Do
                     if (SortTrackNo>0) & (SortTrackNo-1<MMPORT_TrackCount) Then Do
                        if TrackNo<>SortTrackNo Then Do
                           if TrackNo<>SortTrackNo-1 Then Do
                              TrackID     = MMPORT_Tracks.TrackNo
                              SortTrackID = MMPORT_Tracks.SortTrackNo
                              call MMPORT_EnqueueCommand Command, TrackID" "SortTrackID
                           End
                            else say "Order would not change, operation aborted"
                        End
                         else say "Track numbers are the same"
                     End
                      else say "This track does not exist"
                  End
                   else say "This track does not exist"
               End
                else say "Invalid track number"
            End
             else say "Invalid track number"
          End
          Otherwise Do
            say "Unknown command, enter HELP for list of commands"
          End
         End
      End
        else Do
         say "Commands must be 4 chars, enter HELP to get help"
      End
      call CharOut ,"MMPORT:\>"

      Command = LineIn()
      Parse Value Command With Command " " Parameter
   End
 End
 Otherwise Do
   say "Invalid command-line. Try MMPORT CMDLINE to enter command-line mode!"
 End
End
exit

MMPORT_SearchTrack:
   Parse Arg ID
   SearchTrack = 1
   Do Forever
      if SearchTrack>MMPORT_TrackCount Then
         leave
      if MMPORT_Tracks.SearchTrack=ID Then
         return SearchTrack
      SearchTrack = SearchTrack+1
   End
   return 0

MMPORT_SearchTrackInTOCCache:
   Parse Arg ID
   call SysFileTree MMPORT_TOCCache"*."ID, TOCCache., "FO"
   If TOCCache.0>0 Then Do
      SearchTrack = FILESPEC("name", TOCCache.1)
      Parse Value SearchTrack With SearchTrack "."
      return STRIP(SearchTrack,"Leading",0)
   End
   return 0

MP3_ReadInfo:
   Procedure Expose MP3_Title MP3_Artist MP3_Album MP3_Bitrate MP3_Samplerate MP3_Mode MP3_TotalTime MP3_Size
   Parse Arg FileName

   MP3_Size = Stream(FileName,"c","query size")
   if MP3_Size<128 Then return 0

   DumpBuffer = CharIn(FileName,1,4)
   if SubStr(DumpBuffer,1,1)\=D2C(255) Then
      return 0                              /* Invalid Header */
   if BitAnd(SubStr(DumpBuffer,2,1),D2C(126))\=D2C(122) Then
      return 0                              /* Non MPEG2/Layer3 file */
   TempNum = C2D(BitAnd(SubStr(DumpBuffer,3,1),D2C(240)))
   Select
    When TempNum=16  Then MP3_Bitrate = 64
    When TempNum=32  Then MP3_Bitrate = 32
    When TempNum=48  Then MP3_Bitrate = 128
    When TempNum=64  Then MP3_Bitrate = 16
    When TempNum=80  Then MP3_Bitrate = 160
    When TempNum=96  Then MP3_Bitrate = 80
    When TempNum=112 Then MP3_Bitrate = 320
    When TempNum=128 Then MP3_Bitrate = 8
    When TempNum=144 Then MP3_Bitrate = 128
    When TempNum=160 Then MP3_Bitrate = 64
    When TempNum=176 Then MP3_Bitrate = 256
    When TempNum=192 Then MP3_Bitrate = 24
    When TempNum=208 Then MP3_Bitrate = 112
    When TempNum=224 Then MP3_Bitrate = 56
    Otherwise return 0
   End
   TempNum = C2D(BitAnd(SubStr(DumpBuffer,3,1),D2C(12)))
   Select
    When TempNum=0   Then MP3_Samplerate = 44100
    When TempNum=4   Then MP3_Samplerate = 48000
    When TempNum=8   Then MP3_Samplerate = 32000
    Otherwise return 0
   End
   TempNum = C2D(BitAnd(SubStr(DumpBuffer,4,1),D2C(192)))
   Select
    When TempNum=0   Then MP3_Mode = "Stereo"
    When TempNum=64  Then MP3_Mode = "Joint-Stereo"
    When TempNum=128 Then MP3_Mode = "Dual-Channel"
    When TempNum=192 Then MP3_Mode = "Mono"
   End
   /* Now check also for a ID3-Tag */
   MP3_Title  = ""
   MP3_Artist = ""
   MP3_Album  = ""
   DumpBuffer = CharIn(FileName,MP3_Size-127,128)
   if SubStr(DumpBuffer,1,3)="TAG" Then Do
      MP3_Title  = MP3_ExtractASCIIZ(DumpBuffer,4,30)
      MP3_Artist = MP3_ExtractASCIIZ(DumpBuffer,34,30)
      MP3_Album  = MP3_ExtractASCIIZ(DumpBuffer,64,30)
   End
   FrameSize   = 144*(MP3_BitRate*1024)/MP3_Samplerate
   TotalFrames = MP3_Size/FrameSize
   Select
    When MP3_Samplerate=44100 Then Duration = TotalFrames/38.5
    When MP3_Samplerate=48000 Then Duration = TotalFrames/32.5
    When MP3_Samplerate=32000 Then Duration = TotalFrames/27.8
   End
   MP3_TotalTime = Duration
   return 1

MP3_ExtractASCIIZ:
   Procedure
   Parse Arg DumpBuffer, StartPos, MaxLen
   CurPos = StartPos; CurLen = 0
   Do Forever
      if C2D(SubStr(DumpBuffer,CurPos,1))=0 Then
         Leave
      CurLen = CurLen+1
      if CurLen=MaxLen Then
         Leave
      CurPos = CurPos+1
   End
   return Strip(SubStr(DumpBuffer,StartPos,CurLen))

MMPORT_CheckHardware:
   If FILEWAIT_FileExists(MMPORT_Directory"\QueueNOHW")=1 Then
      return "NOHW"
   return ""

MMPORT_EnqueueCommand:
   Parse Arg Command, Parameter
   QueueDirectory = MMPORT_Directory"\queue\"
   call SysFileTree QueueDirectory"*", QueueFiles., "FO"
   If QueueFiles.0>0 Then Do
      QueueLast = QueueFiles.0
      QueueNo = FILESPEC("name",QueueFiles.QueueLast)
      If DATATYPE(QueueNo,"Number")=0 Then Do
         say "Invalid datatype in Queue"
         exit
      End
   End
     else Do
      QueueNo = 0
   End
   Do Forever
      QueueNo   = QueueNo+1
      ValueLen  = LENGTH(QueueNo)
      QueueFile = QueueDirectory||COPIES("0",8-ValueLen)||QueueNo
      If LENGTH(STREAM(QueueFile,"c","query exists"))=0 Then Do
         If STREAM(QueueFile,"c","open write")="READY:" Then Leave
      End
   End
   /* Write Command & Parameter into Queue */
   call LineOut QueueFile, Command
   call LineOut QueueFile, Parameter
   call STREAM QueueFile,"c","close"

   /* Some commands also affect our local TOC-Cache, which may be different */
   /*  than the one used by MMPORT or even the MMPORT-driver */
   Select
     When Command="DSET" Then Do
      Parse Value Parameter With TrackID " " DetailType ' "' String '"'
      TrackNo    = MMPORT_SearchTrack(TrackID)
      If TrackNo>0 Then Do
         DetailType = TRANSLATE(DetailType)
         /* If DetailType known to us, update internal table */
         Select
          When DetailType="TITLE"  Then MMPORT_TrackTitle.TrackNo  = String
          When DetailType="ARTIST" Then MMPORT_TrackArtist.TrackNo = String
          Otherwise Nop
         End
         /* Update to official TOC now, even if modification will be pending */
         call MMPORT_WriteEntryToTOCCache TrackID
      End
     End
     When Command="KILL" Then Do
      Parse Value Parameter With TrackID
      call MMPORT_DelEntryFromTOCCache TrackID
     End
     When Command="SRTA" Then Do
      Parse Value Parameter With TrackID " " RelativeTrackID
      call MMPORT_MoveBehindEntryInTOCCache TrackID, RelativeTrackID
     End
     When Command="SRTB" Then Do
      Parse Value Parameter With TrackID " " RelativeTrackID
      call MMPORT_MoveInFrontEntryInTOCCache TrackID, RelativeTrackID
     End
     Otherwise Nop
   End

   If LENGTH(STREAM(MMPORT_Directory"\QueueActive","c","query exists"))=0 Then Do
      /* No Queue active, so start one */
      '@start "MMPORT Queue" /C /B /MIN "'MMPORT_Directory'\mmport.cmd" QUEUE'
   End
   return QueueFile

/* ============================================= ACTUAL QUEUE PROCESSING === */
MMPORT_ProcessQueue:
   QueueActiveFile = MMPORT_Directory"\QueueActive"
   If STREAM(QueueActiveFile,"c","open write")<>"READY:" Then Do
      say "Queue already active!"
      exit
   End
   call MMPORT_EmptyTOCCache

   /* Start animation of the MMPORT Icon */
   '@start "MMPORT Queue" /C /B /MIN "'MMPORT_Directory'\mmport.cmd" QUEUEANIMATION'

   QueueDirectory = MMPORT_Directory"\queue\"

   /* Get tree into TOCCache and into memory */
   call MMPORT_ExecTREE

   Do Forever
      call SysFileTree QueueDirectory"*", QueueFile., "FO"
      If QueueFile.0=0 Then Do
         call MMPORT_ExecFLSH
         call SysFileTree QueueDirectory"*", QueueFile., "FO"
         If QueueFile.0=0 Then Leave
      End
      /* Process that Queue Entry */
      Do 300
         If LENGTH(STREAM(QueueFile.1,"c", "query exists"))=0 Then Leave
         If FILEWAIT_ForReadFile(QueueFile.1)=1 Then Do
            Command   = LineIn(QueueFile.1)
            Parameter = LineIn(QueueFile.1)
            call STREAM QueueFile.1,"c","close"
            say "- Processing "Command" "Parameter

            Select
             When Command="TREE" Then Do
               /* TREE is not executed, because tree is in memory */
             End
             When Command="UPLD" Then Do
               call MMPORT_ExecUPLD Parameter
             End
             When Command="DNLD" Then Do
               call MMPORT_ExecDNLD Parameter
             End
             When Command="DSET" Then Do
               call MMPORT_ExecDSET Parameter
             End
             When Command="KILL" Then Do
               call MMPORT_ExecKILL Parameter
             End
             When Command="SRTA" Then Do
               call MMPORT_ExecSRTA Parameter
             End
             When Command="SRTB" Then Do
               call MMPORT_ExecSRTB Parameter
             End
             Otherwise Nop
            End
            call SysFileDelete QueueFile.1
            Leave
         End
         call XtraSleep 200
      End
   End
   
   call STREAM QueueActiveFile,"c","close"
   call SysFileDelete QueueActiveFile

   /* Signal end-of-queue */
   call Beep 1800, 50
   call Beep 1200, 40
   call MMPORT_PlayWave "beep.wav"
   exit

MMPORT_PlayWave:
   Parse Arg WaveFileName
   InitRC = mciRxInit()
   MacRC = mciRxSendString("open "WaveFileName" alias playsound wait", 'RetSt', '0', '0')
   If MacRC=0 Then Do
      MacRC = mciRxSendString("play playsound wait", 'RetSt', '0', '0')
      MacRC = mciRxSendString("close playsound wait", 'RetSt', '0', '0')
   End
   call mciRxExit
   return

MMPORT_ProcessFeedback:
   call SysFileDelete MMPORT_Directory"\Queue"
   call SysFileDelete MMPORT_Directory"\QueueNOHW"
   ExportTracks    = 0
   GotTrackListing = 0
   do while (queued()<>0)
      parse pull Data, rxqueue
      Parse Value Data with Command " " Parameter
      select
       When Command="ERR!" Then Do
         say "Error: "Parameter
         return 0
       End
       When Command="DIAG" Then Do
         /* Diagnose feedback */
         say "Diagnose: "Parameter
       End
       When Command="LIST" Then Do
         If GotTrackListing=0 Then Do
            MMPORT_TrackCount = 0
            GotTrackListing   = 1
         End
         MMPORT_TrackCount                    = MMPORT_TrackCount+1
         MMPORT_Tracks.MMPORT_TrackCount      = Parameter
         MMPORT_TrackTitle.MMPORT_TrackCount  = ""
         MMPORT_TrackArtist.MMPORT_TrackCount = ""
         ExportTracks = 1
       End
       When Command="DLST" Then Do
         Parse Value Parameter with ID " " DetailName ' "' String '"'
         DetailName = Translate(DetailName)
         CurTrack = MMPORT_SearchTrack(ID)
         if CurTrack>0 Then Do
            Select
             When DetailName="TITLE" Then
               MMPORT_TrackTitle.CurTrack = String
             When DetailName="ARTIST" Then
               MMPORT_TrackArtist.CurTrack = String
             Otherwise Nop
            End
         End
       End
       When Command="FREE" Then Do
         Parse Value Parameter With MMPORT_FreeSpace " " MMPORT_BiggestFreeBlock
       End
       When Command="IDCH" Then Do
         Parse Value Parameter with ID " " PreviousID
         if Length(PreviousID)=0 Then Do
            /* We got a new ID, so assign it */
            MMPORT_TrackCount = MMPORT_TrackCount+1
            MMPORT_Tracks.MMPORT_TrackCount      = ID
            MMPORT_TrackTitle.MMPORT_TrackCount  = ""
            MMPORT_TrackArtist.MMPORT_TrackCount = ""
            MMPORT_NewID                         = ID
            /* We wont export this ID now because its done later in UPLD */
         End
          else Do
            /* Driver wants to change an ID */
            CurTrack = MMPORT_SearchTrack(PreviousID)
            if CurTrack>0 Then Do
               MMPORT_Tracks.CurTrack = ID
               ExportTracks = 1
            End
         End
       End
       When Command="NSPC" Then Do
         say "No space left!"
         return 0
       End
       When Command="NOHW" Then Do
         say "No hardware found!"
         call FILEWAIT_ForWriteFile MMPORT_Directory"\QueueNOHW"
         call FILEWAIT_Close MMPORT_Directory"\QueueNOHW"
         return 0
       End
       Otherwise Nop
      End
   End
   If ExportTracks=1 Then Do
      /* Export track-listing to TOCCache for e.g. commandline thread */
      call MMPORT_WriteTOCCache
   End
   return 1

MMPORT_ProcessQueueAnimation:
   QueueActiveFile = MMPORT_Directory"\QueueActive"
   AnimateNo = 4
   Do Forever
      call SysSetObjectData "<MMPORT_UPLOAD>", "ICONFILE="MMPORT_Directory"\Icons\basket"AnimateNo".ico"
      call XtraSLeep 250
      AnimateNo = AnimateNo+1
      If AnimateNo=10 Then AnimateNo = 4
      If LENGTH(STREAM(QueueActiveFile,"c","query exists"))=0 Then Leave
   End
   call SysSetObjectData "<MMPORT_UPLOAD>", "ICONFILE="MMPORT_Directory"\Icons\basket.ico"
   exit

MMPORT_EmptyTOCCache:
   call SysFileTree MMPORT_TOCCache"*", TOCCache., "FO"
   Do CurTrack=1 To TOCCache.0
      call SysFileDelete TOCCache.CurTrack
   End
   MMPORT_TrackCount = 0
   return

MMPORT_ReadTOCCache:
   call SysFileTree MMPORT_TOCCache"*", TOCCache., "FO"
   MMPORT_TrackCount = TOCCache.0
   Do CurTrack=1 To MMPORT_TrackCount
      TOCCacheFile = TOCCache.CurTrack
      TrackID = FILESPEC("name", TOCCache.CurTrack)
      Parse Value TrackID With "." TrackID
      MMPORT_Tracks.CurTrack = TrackID
      If FILEWAIT_ForReadFile(TOCCacheFile)=1 Then Do
         MMPORT_TrackTitle.CurTrack  = LineIn(TOCCacheFile)
         MMPORT_TrackArtist.CurTrack = LineIn(TOCCacheFile)
         call FILEWAIT_Close TOCCacheFile
      End
   End
   return

MMPORT_WriteTOCCache:
   Do CurTrack=1 To MMPORT_TrackCount
      TOCCacheFile = MMPORT_TOCCache||D2X(CurTrack,8)"."MMPORT_Tracks.CurTrack
      call FILEWAIT_ForKillFile TOCCacheFile
      If FILEWAIT_ForWriteFile(TOCCacheFile)=1 Then Do
         call LineOut TOCCacheFile, MMPORT_TrackTitle.CurTrack, 1
         call LineOut TOCCacheFile, MMPORT_TrackArtist.CurTrack
         call FILEWAIT_Close TOCCacheFile
      End
   End
   return

MMPORT_WriteEntryToTOCCache:
   Parse Arg TrackID
   TrackNo = MMPORT_SearchTrack(TrackID)
   If TrackNo>0 Then Do
      call SysFileTree MMPORT_TOCCache"*."TrackID, TOCCache., "FO"
      If TOCCache.0>0 Then Do
         TOCCacheFile = TOCCache.1
         call FILEWAIT_ForKillFile TOCCacheFile
         If FILEWAIT_ForWriteFile(TOCCacheFile)=1 Then Do
            call LineOut TOCCacheFile, MMPORT_TrackTitle.TrackNo, 1
            call LineOut TOCCacheFile, MMPORT_TrackArtist.TrackNo
            call FILEWAIT_Close TOCCacheFile
         End
      End
   End
   return

MMPORT_AddEntryToTOCCache:
   LastTrackNo = MMPORT_TrackCount
   call SysFileTree MMPORT_TOCCache"*", TOCCache., "FO"
   TOCCacheFile = MMPORT_TOCCache||D2X(TOCCache.0+1,8)"."MMPORT_Tracks.LastTrackNo
   If FILEWAIT_ForWriteFile(TOCCacheFile)=1 Then Do
      call LineOut TOCCacheFile, MMPORT_TrackTitle.LastTrackNo, 1
      call LineOut TOCCacheFile, MMPORT_TrackArtist.LastTrackNo
      call FILEWAIT_Close TOCCacheFile
   End
   return

/* This won't delete it from the local TOC cache, because this function is */
/*  used immediately when the KILL command is issued and NOT when its */
/*  processed, so the user will not see the entry anymore though the command */
/*  is pending */
MMPORT_DelEntryFromTOCCache:
   Parse Arg DelTrackID
   call SysFileTree MMPORT_TOCCache"*", TOCCache., "FO"
   RenTracks  = 0
   Do i=1 to TOCCache.0
      Parse Value FILESPEC("name", TOCCache.i) With CurTrackNo "." CurTrackID
      If RenTracks=1 Then Do
         NewTrackName = D2X(CurTrackNo-1,8)"."CurTrackID
         '@ren "'TOCCache.i'" "'NewTrackName'"'
      End
        else Do
         If CurTrackID=DelTrackID Then Do
            call FILEWAIT_ForKillFile TOCCache.i
            RenTracks = 1
         End
      End
   End
   return

MMPORT_DelEntryFromTOCCacheArray:
   Parse Arg DelTrackID
   CurTrack    = MMPORT_SearchTrack(DelTrackID)
   OldTrack    = CurTrack+1
   Do Forever
      if OldTrack>MMPORT_TrackCount Then
         leave
      MMPORT_Tracks.CurTrack      = MMPORT_Tracks.OldTrack
      MMPORT_TrackTitle.CurTrack  = MMPORT_TrackTitle.OldTrack
      MMPORT_TrackArtist.CurTrack = MMPORT_TrackArtist.OldTrack
      CurTrack = CurTrack+1
      OldTrack = OldTrack+1
   End
   MMPORT_TrackCount = MMPORT_TrackCount-1
   return

MMPORT_MoveBehindEntryInTOCCache:
   Parse Arg TrackID, RelativeTrackID
   CurTrackNo  = MMPORT_SearchTrack(TrackID)
   RelativeTrackNo = MMPORT_SearchTrack(RelativeTrackID)
   if RelativeTrackNo>CurTrackNo Then
      call MMPORT_MoveEntryInTOCCacheHelper RelativeTrackNo, +1
     else
      call MMPORT_MoveEntryInTOCCacheHelper RelativeTrackNo+1, -1

   /* Now also move in actual TOC-Cache */
   CurTrackNo      = MMPORT_SearchTrackInTOCCache(TrackID)
   RelativeTrackNo = MMPORT_SearchTrackInTOCCache(RelativeTrackID)
   if RelativeTrackNo>CurTrackNo Then
      call MMPORT_MoveEntryInTOCCacheHelper2 RelativeTrackNo, +1
     else
      call MMPORT_MoveEntryInTOCCacheHelper2 RelativeTrackNo+1, -1
   return

MMPORT_MoveInFrontEntryInTOCCache:
   Parse Arg TrackID, RelativeTrackID
   CurTrackNo  = MMPORT_SearchTrack(TrackID)
   RelativeTrackNo = MMPORT_SearchTrack(RelativeTrackID)
   if RelativeTrackNo>CurTrackNo Then
      call MMPORT_MoveEntryInTOCCacheHelper RelativeTrackNo-1, +1
     else
      call MMPORT_MoveEntryInTOCCacheHelper RelativeTrackNo, -1

   /* Now also move in actual TOC-Cache */
   CurTrackNo      = MMPORT_SearchTrackInTOCCache(TrackID)
   RelativeTrackNo = MMPORT_SearchTrackInTOCCache(RelativeTrackID)
   if RelativeTrackNo>CurTrackNo Then
      call MMPORT_MoveEntryInTOCCacheHelper2 RelativeTrackNo-1, +1
     else
      call MMPORT_MoveEntryInTOCCacheHelper2 RelativeTrackNo, -1
   return

MMPORT_MoveEntryInTOCCacheHelper:
   Parse Arg DestinationTrackNo, Stepping

   MMPORT_Tracks.0      = MMPORT_Tracks.CurTrackNo
   MMPORT_TrackTitle.0  = MMPORT_TrackTitle.CurTrackNo
   MMPORT_TrackArtist.0 = MMPORT_TrackArtist.CurTrackNo
   SrcPos  = CurTrackNo+Stepping
   DstPos  = CurTrackNo
   Do Forever
      if DstPos=DestinationTrackNo Then
         Leave
      MMPORT_Tracks.DstPos      = MMPORT_Tracks.SrcPos
      MMPORT_TrackTitle.DstPos  = MMPORT_TrackTitle.SrcPos
      MMPORT_TrackArtist.DstPos = MMPORT_TrackArtist.SrcPos
      SrcPos = SrcPos+Stepping
      DstPos = DstPos+Stepping
   End
   MMPORT_Tracks.DestinationTrackNo      = MMPORT_Tracks.0
   MMPORT_TrackTitle.DestinationTrackNo  = MMPORT_TrackTitle.0
   MMPORT_TrackArtist.DestinationTrackNo = MMPORT_TrackArtist.0
   return

MMPORT_MoveEntryInTOCCacheHelper2:
   Parse Arg DestinationTrackNo, Stepping
   call SysFileTree MMPORT_TOCCache"*", TOCCache., "FO"

   SrcPos  = CurTrackNo+Stepping
   DstPos  = CurTrackNo
   Do Forever
      if DstPos=DestinationTrackNo Then
         Leave
      '@ren 'TOCCache.SrcPos' 'D2X(DstPos,8)'.*'
      SrcPos = SrcPos+Stepping
      DstPos = DstPos+Stepping
   End
   '@ren 'TOCCache.CurTrackNo' 'D2X(DestinationTrackNo,8)'.*'
   return

/* END-OF-TOC-MODIFICATION */

MMPORT_ExecTREE:
   '@'MMPORT_Directory'\CasioWMP.cmd TREE | RXQUEUE'
   call MMPORT_ProcessFeedback
   return

MMPORT_ExecFLSH:
   Parse Arg Parameter

   '@'MMPORT_Directory'\CasioWMP.cmd FLSH | RXQUEUE'
   call MMPORT_ProcessFeedback
   return

MMPORT_ExecUPLD:
   Parse Arg Parameter
   Parameter = STRIP(Parameter, "Both", '"')

   rc = Stream(Parameter, "c", "open read")
   if rc\="READY:" Then Do
      say "File not found"
      return
   End
   if MP3_ReadInfo(Parameter)=0 Then Do
      say "File is not an MP3"
      return
   End
   call Stream MP3_Parameter, "c", "close"
   say "Processing upload request (this may take some time)..."
   MMPORT_NewID = ""
   '@'MMPORT_Directory'\CasioWMP.cmd UPLD MP3 "'Parameter'" 'MP3_Bitrate' 'MP3_Samplerate' 'MP3_Mode' | RXQUEUE'
   if MMPORT_ProcessFeedback()=1 Then Do
      say "Successfully uploaded."
      If LENGTH(MP3_Title)=0 Then
         MP3_Title = FILESPEC("name", Parameter)
      say "TAG: Setting title to "MP3_Title
      MMPORT_TrackTitle.MMPORT_TrackCount = MP3_Title
      '@'MMPORT_Directory'\CasioWMP.cmd DSET 'MMPORT_NewID' TITLE "'MP3_Title'" | RXQUEUE'
      call MMPORT_ProcessFeedback
      if Length(MP3_Artist)>0 Then Do
         say "TAG: Setting artist to "MP3_Artist
         MMPORT_TrackArtist.MMPORT_TrackCount = MP3_Artist
         '@'MMPORT_Directory'\CasioWMP.cmd DSET 'MMPORT_NewID' ARTIST "'MP3_Artist'" | RXQUEUE'
         call MMPORT_ProcessFeedback
      End
      /* And include that (last) track in TOC-Cache (after upload of course) */
      call MMPORT_AddEntryToTOCCache
   End
   return

MMPORT_ExecDNLD:
   Parse Arg Parameter
   Parse Value Parameter With TrackNo " " FileName
   FileName = STRIP(FileName, "Both", '"')

   say "Processing download request (this may take some time)..."
   '@'MMPORT_Directory'\CasioWMP.cmd DNLD 'MMPORT_Tracks.TrackNo' "'Filename'" | RXQUEUE'
   if MMPORT_ProcessFeedback()=1 Then
      say "Successfully downloaded."
   return

MMPORT_ExecDSET:
   Parse Arg Parameter
   Parse Value Parameter With TrackID " " DetailType '"' String '"'

   TrackNo    = MMPORT_SearchTrack(TrackID)
   DetailType = TRANSLATE(DetailType)
   /* If DetailType known to us, update internal table */
   Select
    When DetailType="TITLE"  Then MMPORT_TrackTitle.TrackNo  = String
    When DetailType="ARTIST" Then MMPORT_TrackArtist.TrackNo = String
    Otherwise Nop
   End

   '@'MMPORT_Directory'\CasioWMP.cmd DSET 'TrackID' 'DetailType' "'String'" | RXQUEUE'
   call MMPORT_ProcessFeedback
   /* TOC got already updated by MMPORT-Enqueue, but we update again just to */
   /*  be sure */
   call MMPORT_WriteEntryToTOCCache TrackID
   return

MMPORT_ExecKILL:
   Parse Arg Parameter
   Parse Value Parameter With TrackID

   '@'MMPORT_Directory'\CasioWMP.cmd KILL 'TrackID' | RXQUEUE'
   call MMPORT_ProcessFeedback
   /* TOC got already updated by MMPORT-Enqueue, so only update local array */
   call MMPORT_DelEntryFromTOCCacheArray TrackID
   call MMPORT_DelEntryFromTOCCache TrackID
   return

MMPORT_ExecSRTA:
   Parse Arg Parameter
   Parse Value Parameter With TrackID " " RelativeTrackID

   '@'MMPORT_Directory'\CasioWMP.cmd SRTA 'TrackID' 'RelativeTrackID' | RXQUEUE'
   call MMPORT_ProcessFeedback
   call MMPORT_MoveBehindEntryInTOCCache TrackID, RelativeTrackID
   say "Order successfully changed."
   return

MMPORT_ExecSRTB:
   Parse Arg Parameter
   Parse Value Parameter With TrackID " " RelativeTrackID

   '@'MMPORT_Directory'\CasioWMP.cmd SRTB 'TrackID' 'RelativeTrackID' | RXQUEUE'
   call MMPORT_ProcessFeedback
   call MMPORT_MoveInFrontEntryInTOCCache TrackID, RelativeTrackID
   say "Order successfully changed."
   return

/* ========================================================================= */
/*  INCLUDEd C:\Rexx\filewait */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  FILE-WAIT Library - by Kiewitz in 2002                                   */
/* ------------------------------------------------------------------------- */

/* All of these routines will wait a maximum of 60 seconds for getting xs */
/*  Timeout will be 300 (60*5) */

/* Will wait for read access. Will return false, if file is not found or */
/*  killed during trying for access */
FILEWAIT_ForReadFile:
   Procedure
   Parse Arg Filename
   Do 800
      If Length(Stream(Filename,"c", "query exists"))=0 Then return 0
      If Stream(Filename,"c","open read")="READY:" Then return 1
      call XtraSleep 200
   End
   return 0

/* Will wait for read access on the specified file even if the file is not */
/*  found. If timeout -> Error-Message and script abort */
FILEWAIT_ForMustReadFile:
   Procedure
   Parse Arg Filename
   Do 800
      If Stream(Filename,"c","open read")="READY:" Then return 1
      call XtraSleep 200
   End
   say "Rexx: No MustRead-Access on file "Filename
   exit

/* Will wait for write access on the specified file. */
/*  If timeout -> Error-Message and Exit */
FILEWAIT_ForWriteFile:
   Procedure
   Parse Arg Filename
   Do 800
      If Stream(Filename,"c","open write")="READY:" Then return 1
      call XtraSleep 200
   End
   say "Rexx: No Write-Access on file "Filename
   exit

/* Will wait for read/write access on the specified file. */
/*  If timeout -> Error-Message and Exit */
FILEWAIT_ForReadWriteFile:
   Procedure
   Parse Arg Filename
   Do 800
      If Stream(Filename,"c","open")="READY:" Then return 1
      call XtraSleep 200
   End
   say "Rexx: No ReadWrite-Access on file "Filename
   exit

/* Will wait for killing specified file. */
/*  If timeout -> Error-Message and Exit */
FILEWAIT_ForKillFile:
   Procedure
   Parse Arg Filename
   Do 800
      rc = SysFileDelete(Filename)
      If rc\=5 & rc\=32 Then return 1
      call XtraSleep 200
   End
   say "Rexx: Kill denied on file "Filename
   exit

/* Will wait for killing and replacing specified file. */
/*  If timeout -> Error-Message from FILEWAIT_ForKillFile and Exit */
FILEWAIT_ForReplaceFile:
   Procedure
   Parse Arg OrgFile, ReplaceFile
   call FILEWAIT_ForKillFile OrgFile
   '@ren 'ReplaceFile' 'FileSpec("name",OrgFile)
   return 1

FILEWAIT_Close:
   Procedure
   Parse Arg Filename
   call Stream Filename, "c", "close"
   return

FILEWAIT_FileExists:
   Procedure
   Parse Arg Filename
   if length(stream(Filename,"c","query exists"))>0 Then return 1
   return 0

FILEWAIT_Seek:
   Procedure
   Parse Arg Filename, Offset
   call Stream Filename,"c","seek ="Offset
   return

FILEWAIT_GetSeek:
   Procedure
   Parse Arg Filename
   return Stream(Filename,"c","seek")

FILEWAIT_GetSize:
   Procedure
   Parse Arg Filename
   return Stream(Filename,"c","query size")

/* ========================================================================= */
/*  INCLUDEd C:\Rexx\error-handler */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  ERROR-HANDLER Library - by Kiewitz in 2002                               */
/* ------------------------------------------------------------------------- */

/* signal on syntax name ErrorHandler */
/* signal on halt name ErrorHalt */

ErrorHandler:
   say SIGL" +++   "SourceLine(SIGL)
   say "Error "rc" at line "SIGL": "ErrorText(rc)
  ErrorHalt:
   say " - Please note this message and press Strg-Break"
   Do Forever
      call SysSleep 30
   End
   exit

