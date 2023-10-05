'AVI Library

#INCLUDE ONCE "Vfw.inc"


GLOBAL AVILIB_hFile&        'handle of the last opened AVI file (only one AVI file can be open at the same time)
GLOBAL AVILIB_hVideoSteam&  'handle of the active video stream
GLOBAL AVILIB_hAudioSteam&  'handle of the active audio stream
GLOBAL AVILIB_hGetFrame&    'handle of the frame getter



'Open the AVI file and return a handle to it
FUNCTION OpenAviFile&(f$)
  LOCAL r&, hAviFile&
  LOCAL szFilename AS ASCIIZ*512

  CALL AVIFileInit
  szFilename = f$
  r& = AVIFileOpen(hAviFile&, szFilename, %OF_READ, BYVAL %NULL)
  IF r& <> %AVIERR_OK THEN
    hAviFile& = 0
  ELSE
    AVILIB_hFile& = hAviFile&
  END IF

  OpenAviFile& = hAviFile&
END FUNCTION



'Opens the first video stream of an AVI file that has been opened with OpenAviFile&()
FUNCTION GetAviVideoStream&(hAviFile&)
  LOCAL r&, hAviStream&

  r& = AVIFileGetStream(hAviFile&, hAviStream&, %streamtypeVIDEO, 0)
  IF r& <> %AVIERR_OK THEN
    hAviStream& = 0
  ELSE
    AVILIB_hVideoSteam& = hAviStream&
    AVILIB_hGetFrame& = AVIStreamGetFrameOpen(hAviStream&, BYVAL %NULL)
  END IF

  GetAviVideoStream& = IIF&(AVILIB_hGetFrame& = 0, 0, hAviStream&)
END FUNCTION



'Opens an audio stream of an AVI file that has been opened with OpenAviFile&()
FUNCTION GetAviAudioStream&(hAviFile&, languageNumber&)
  LOCAL r&, hAviStream&

  r& = AVIFileGetStream(hAviFile&, hAviStream&, %streamtypeAUDIO, languageNumber&)
  IF r& <> %AVIERR_OK THEN
    hAviStream& = 0
  ELSE
    AVILIB_hAudioSteam& = hAviStream&
  END IF

  GetAviAudioStream& = hAviStream&
END FUNCTION



'Returns the frame width and height, the number of frames and the rate of a video frame that has been opened with GetAviVideoStream&()
SUB GetAVIVideoStreamInfo(BYVAL hAviStream&, BYREF frameWidth&, BYREF frameHeight&, BYREF frameCount&, BYREF millisecsPerFrame&)
  LOCAL psi AS AVISTREAMINFOA_STRUCT

  CALL AVIStreamInfo(hAviStream&, psi, SIZEOF(psi))
  frameWidth& = psi.rcFrame.right - psi.rcFrame.left
  frameHeight& = psi.rcFrame.bottom - psi.rcFrame.top
  frameCount& = AVIStreamLength(hAviStream&)
  millisecsPerFrame& = AVIStreamSampleToTime(hAviStream&, frameCount&) / frameCount&
END SUB



'Returns the audio parameters of an audio stream that has been opened with GetAviAudioStream&()
SUB GetAVIAudioStreamInfo(BYVAL hAviStream&, BYREF audioFormat&, BYREF channelCount&, BYREF samplesPerSecond&, BYREF bitsPerSample&, BYREF streamLengthInBytes&)
  LOCAL r&, formatStructureSize&, streamlengthInSamples&
  LOCAL fmt AS PCMWAVEFORMAT

  formatStructureSize& = SIZEOF(fmt)
  r& = AVIStreamReadFormat(hAviStream&, 0, VARPTR(fmt), formatStructureSize&)
  IF r& <> %AVIERR_OK THEN
    audioFormat& = 0
    channelCount& = 0
    samplesPerSecond& = 0
    bitsPerSample& = 0
    streamLengthInBytes& = 0
  ELSE
    audioFormat& = fmt.wf.wFormatTag
    channelCount& = fmt.wf.nChannels
    samplesPerSecond& = fmt.wf.nSamplesPerSec
    bitsPerSample& = fmt.wBitsPerSample
    streamlengthInSamples& = AVIStreamLength(hAviStream&)
    streamLengthInBytes& = streamlengthInSamples&*fmt.wf.nBlockAlign
  END IF
END SUB



'Reads sample data from an audio stream that has been opened with GetAviAudioStream&()
'Specify numberOfSamples& = -1 to read the entire stream
FUNCTION ReadAVIAudioStream$(BYVAL hAviStream&, BYVAL startSample&, BYVAL numberOfSamples&)
  LOCAL r&, requiredBufferSize&, bytesRead&, samplesRead&
  LOCAL audioFormat&, channelCount&, samplesPerSecond&, bitsPerSample&, streamLengthInBytes&, bytesPerSample&
  LOCAL buffer$

  'calculate required buffer size
  CALL GetAVIAudioStreamInfo(hAviStream&, audioFormat&, channelCount&, samplesPerSecond&, bitsPerSample&, streamLengthInBytes&)
  bytesPerSample& = bitsPerSample&/8
  IF numberOfSamples& = -1 THEN
    requiredBufferSize& = streamLengthInBytes&-startSample&*bytesPerSample&
    numberOfSamples& = requiredBufferSize&/bytesPerSample&
  ELSE
    requiredBufferSize& = numberOfSamples&*bytesPerSample&
  END IF
  buffer$ = STRING$(requiredBufferSize&, 0)

  r& = AVIStreamRead(hAviStream&, startSample&, numberOfSamples&, STRPTR(buffer$), requiredBufferSize&, bytesRead&, samplesRead&)
  IF r& <> %AVIERR_OK THEN EXIT FUNCTION
  IF bytesRead& < LEN(buffer$) THEN buffer$ = LEFT$(buffer$, bytesRead&)

  ReadAVIAudioStream$ = buffer$
END FUNCTION



'Returns the frame that should be displayed at a given millisecond
FUNCTION GetAVIVideoFrameNumberForMillisecond&(millisec&)
  GetAVIVideoFrameNumberForMillisecond& = AVIStreamTimeToSample(AVILIB_hVideoSteam&, millisec&)
END FUNCTION



'Returns the RGB pixel data of a video frame for a stream that has been opened with GetAviVideoStream&()
FUNCTION GetAVIFramePixelData$(frameNumber&)
  LOCAL pbitmap&, ppixeldatasrc&, ppalettedatasrc&, bitmapWidth&, bitmapHeight&, pdest&, i&
  LOCAL bmheader AS BITMAPINFOHEADER
  LOCAL a$

  pbitmap& = AVIStreamGetFrame(AVILIB_hGetFrame&, frameNumber&)
  IF pbitmap& = 0 THEN EXIT FUNCTION
  POKE$ VARPTR(bmheader), PEEK$(pbitmap&, SIZEOF(BITMAPINFOHEADER))
  bitmapWidth& = bmheader.biWidth
  bitmapHeight& = bmheader.biHeight
  ppixeldatasrc& = pbitmap&+bmheader.biSize+bmheader.biClrUsed*SIZEOF(RGBQUAD)
  ppalettedatasrc& = pbitmap&+bmheader.biSize

  a$ = STRING$(bitmapWidth&*bitmapHeight&*4, 255)
  SELECT CASE bmheader.biBitCount
  CASE 8: CALL ConvertIndexedColorsToBGRAandFlip(ppixeldatasrc&, STRPTR(a$), bitmapWidth&, bitmapHeight&, ppalettedatasrc&)
  CASE 24: CALL ConvertRGBtoBGRAandFlip(ppixeldatasrc&, STRPTR(a$), bitmapWidth&, bitmapHeight&)
  END SELECT

  GetAVIFramePixelData$ = a$
END FUNCTION



'Converts 8-Bit indexed color pixel data to BGRA and flips the image vertically
SUB ConvertIndexedColorsToBGRAandFlip(pSrc&, pDest&, bitmapWidth&, bitmapHeight&, pPalette&)
  'Note: All parameters are pointers to the actual values (even when declared with BYVAL)
  #REGISTER NONE

  LOCAL rowcounter&

  'save CPU registers
  ! PUSH EAX
  ! PUSH EBX
  ! PUSH ECX
  ! PUSH EDX
  ! PUSH ESI
  ! PUSH EDI

  'load source index and set destination index to last row
  ! MOV ESI, pSrc&
  ! MOV ESI, [ESI]
  ! MOV EDI, pDest&
  ! MOV EDI, [EDI]
  ! MOV EAX, bitmapWidth&
  ! MOV EAX, [EAX]
  ! MOV EBX, bitmapHeight&
  ! MOV EBX, [EBX]
  ! MOV rowcounter&, EBX
  ! DEC EBX
  ! MUL EBX
  ! SHL EAX, 2
  ! ADD EDI, EAX
  ! MOV EDX, bitmapWidth&
  ! MOV EDX, [EDX]
  ! MOV ECX, EDX

  'load palette
  ! MOV EBX, pPalette&
  ! MOV EBX, [EBX]

  IndexedColorsToBGRA_NextPixel:
  'load color index, read its BGR value from the palette and set alpha=255
  ! XOR EAX, EAX
  ! LODSB
  ! MOV EAX, [EBX+EAX*4]
  ! OR EAX, &HFF000000

  'store BGRA value in destination
  ! STOSD
  ! LOOP IndexedColorsToBGRA_NextPixel

  'move destination index to beginning of previous row (by subtracting 2x bitmap-width * SIZEOF(DWORD)
  ! MOV ECX, EDX
  ! MOV EAX, EDX
  ! SHL EAX, 3
  ! SUB EDI, EAX
  ! DEC rowcounter&
  ! JNZ IndexedColorsToBGRA_NextPixel

  'restore CPU registers
  ! POP EDI
  ! POP ESI
  ! POP EDX
  ! POP ECX
  ! POP EBX
  ! POP EAX
END SUB


'Converts RGB pixel data to BGRA and flips the image vertically
SUB ConvertRGBtoBGRAandFlip(pSrc&, pDest&, bitmapWidth&, bitmapHeight&)
  'Note: All parameters are pointers to the actual values (even when declared with BYVAL)
  #REGISTER NONE

  'save CPU registers
  ! PUSH EAX
  ! PUSH EBX
  ! PUSH ECX
  ! PUSH EDX
  ! PUSH ESI
  ! PUSH EDI

  'load source index and set destination index to last row
  ! MOV ESI, pSrc&
  ! MOV ESI, [ESI]
  ! MOV EDI, pDest&
  ! MOV EDI, [EDI]
  ! MOV EAX, bitmapWidth&
  ! MOV EAX, [EAX]
  ! MOV EBX, bitmapHeight&
  ! MOV EBX, [EBX]
  ! DEC EBX
  ! MUL EBX
  ! SHL EAX, 2
  ! ADD EDI, EAX
  ! INC EBX
  ! MOV EDX, bitmapWidth&
  ! MOV EDX, [EDX]
  ! MOV ECX, EDX

  RBGtoBGRA_NextPixel:
  'load red, green, and blue value in reverse order and set alpha=255
  ! LODSB
  ! ROR EAX, 8
  ! LODSB
  ! ROR EAX, 8
  ! LODSB
  ! ROR EAX, 8
  ! MOV AL, 255
  ! ROR EAX, 8

  'store BGRA value in destination
  ! STOSD
  ! LOOP RBGtoBGRA_NextPixel

  'move destination index to beginning of previous row (by subtracting 2x bitmap-width * SIZEOF(DWORD)
  ! MOV ECX, EDX
  ! MOV EAX, EDX
  ! SHL EAX, 3
  ! SUB EDI, EAX
  ! DEC EBX
  ! JNZ RBGtoBGRA_NextPixel

  'restore CPU registers
  ! POP EDI
  ! POP ESI
  ! POP EDX
  ! POP ECX
  ! POP EBX
  ! POP EAX
END SUB



'Closes an AVI file opened with OpenAviFile&() and releases all handles
SUB CloseAVIFile
  IF AVILIB_hGetFrame& <> 0 THEN CALL AVIStreamGetFrameClose(AVILIB_hGetFrame&)
  IF AVILIB_hVideoSteam& <> 0 THEN CALL AVIStreamRelease(AVILIB_hVideoSteam&)
  IF AVILIB_hAudioSteam& <> 0 THEN CALL AVIStreamRelease(AVILIB_hAudioSteam&)
  IF AVILIB_hFile& <> 0 THEN CALL AVIFileRelease(AVILIB_hFile&)

  AVILIB_hFile& = 0
  AVILIB_hAudioSteam& = 0
  AVILIB_hVideoSteam& = 0
  AVILIB_hGetFrame& = 0
END SUB



'Shuts down the AVI library
SUB ShutDownAVI
  CALL AVIFileExit
END SUB
