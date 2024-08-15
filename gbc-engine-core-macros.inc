; UPDATE THIS LINE WHEN CHANGING THE REVISION NUMBER
DEF GBC_ENGINE_CORE_VERSION     EQUS "1.2.0"

; REQUIRED RGBDS VERSION
DEF REQUIRED_RGBDS_VERSION      EQUS "0.7.0"

; REQUIRE HARDWARE.INC VERSION
DEF REQUIRED_HARDWARE_VERSION   EQUS "4.9.1"


/*******************************************************************************
**
**  INTERNAL MACROS
**
*******************************************************************************/

/*
Call with the expected version string to ensure your code
is compatible with the INCLUDE'd version of `gbc-engine-core.inc`.

Usage: `gbc_engine_core_version` `<min_ver>`

Examples:
- `gbc_engine_core_version 1.2.3`
- `gbc_engine_core_version 1.2` (equivalent to 1.2.0)
- `gbc_engine_core_version 1` (equivalent to 1.0.0)
*/
MACRO gbc_engine_core_version
    REDEF CURRENT_VERSION EQUS STRRPL("{GBC_ENGINE_CORE_VERSION}", ".", ",")
    REDEF EXPECTED_VERSION EQUS STRRPL("\1", ".", ",")

    REDEF INTERNAL_CORE_CHECK EQUS """MACRO internal_core_check
    IF (\\1 != \\4) || (\\1 == \\4 && \\2 < \\5) || (\\1 == \\4 && \\2 == \\5 && \\3 < \\6)
      FAIL "gbc-engine-core.inc version \\4.\\5.\\6 is required, which is incompatible with current version \\1.\\2.\\3"
    ENDC\nENDM"""

    INTERNAL_CORE_CHECK

    internal_core_check {CURRENT_VERSION}, {EXPECTED_VERSION}, 0, 0, 0

    PURGE CURRENT_VERSION
    PURGE EXPECTED_VERSION
    PURGE INTERNAL_CORE_CHECK
    PURGE internal_core_check
ENDM

/*
Call with the expected version string to ensure your code
is compatible with current version of RGBDS binaries.

Usage: `check_rgbds_version` `<min_ver>`

Examples:
- `check_rgbds_version 1.2.3`
- `check_rgbds_version 1.2` (equivalent to 1.2.0)
- `check_rgbds_version 1` (equivalent to 1.0.0)
*/
MACRO check_rgbds_version
    IF (!DEF(__RGBDS_MAJOR__) || !DEF(__RGBDS_MINOR__) || !DEF(__RGBDS_PATCH__) || DEF(__RGBDS_RC__))
      FAIL "RGBDS version \1 is required, which is incompatible with current version"
    ENDC

    REDEF RGBDS_VERSION EQUS STRCAT("{d:__RGBDS_MAJOR__}", ".", "{d:__RGBDS_MINOR__}", ".","{d:__RGBDS_PATCH__}")

    REDEF CURRENT_VERSION EQUS STRRPL("{RGBDS_VERSION}", ".", ",")
    REDEF EXPECTED_VERSION EQUS STRRPL("\1", ".", ",")

    REDEF INTERNAL_RGBDS_CHECK EQUS """MACRO internal_rgbds_check
    IF (\\1 != \\4) || (\\1 == \\4 && \\2 < \\5) || (\\1 == \\4 && \\2 == \\5 && \\3 < \\6)
      FAIL "RGBDS version \\4.\\5.\\6 is required, which is incompatible with current version \\1.\\2.\\3"
    ENDC\nENDM"""

    INTERNAL_RGBDS_CHECK

    internal_rgbds_check {CURRENT_VERSION}, {EXPECTED_VERSION}, 0, 0, 0

    PURGE RGBDS_VERSION
    PURGE CURRENT_VERSION
    PURGE EXPECTED_VERSION
    PURGE INTERNAL_RGBDS_CHECK
    PURGE internal_rgbds_check
ENDM


/*******************************************************************************
**                                                                            **
**      VERSION CHECK                                                         **
**                                                                            **
*******************************************************************************/

; check to ensure hardware.inc has been included and meets minimum version
    rev_Check_hardware_inc {REQUIRED_HARDWARE_VERSION}

; check to ensure RGBDS binaries meet minimum version
    check_rgbds_version {REQUIRED_RGBDS_VERSION}


/*******************************************************************************
**                                                                            **
**      GFX MACROS                                                            **
**                                                                            **
*******************************************************************************/

/*
GBC palette entry using html rgb color codes.

For easy palette creation using rgb codes that can be color highlighted
by most modern code editors; allowing you to preview your color choices.

Usage:
- `rgb_palette #FFFFFF, #AAAAAA, #555555, #000000`

Outputs:
- `DW $7FFF, $5294, $294A, $0000`
*/
MACRO rgb_palette
    ASSERT (_NARG == 4), \
        "rgb_palette requires 4 color codes and parsed {_NARG}."

    REPT _NARG
        ASSERT (STRIN("\1", "#") == 1 && (STRLEN("\1") == 4 || STRLEN("\1") == 7)), \
            "rgb_palette requires codes formated as #xxx or #xxxxxx and instead parsed \1."
    
        REDEF CODE_LEN  EQU STRLEN("\1")
        REDEF CODE_STR EQUS STRRPL("\1", "#", "$")
        REDEF CODE_HEX  EQU {CODE_STR}

        IF CODE_LEN == 7
            REDEF RGB_RED EQU ((CODE_HEX & $FF0000) >> 16) / 8
            REDEF RGB_GRN EQU ((CODE_HEX & $00FF00) >>  8) / 8
            REDEF RGB_BLU EQU ((CODE_HEX & $0000FF) >>  0) / 8
        ELSE
            REDEF RGB_RED EQU (((CODE_HEX & $F00) >> 4) ^ ((CODE_HEX & $F00) >> 8)) / 8
            REDEF RGB_GRN EQU (((CODE_HEX & $0F0) >> 0) ^ ((CODE_HEX & $0F0) >> 4)) / 8
            REDEF RGB_BLU EQU (((CODE_HEX & $00F) << 4) ^ ((CODE_HEX & $00F) >> 0)) / 8
        ENDC

        DW ((RGB_RED) << 0) + ((RGB_GRN) << 5) + ((RGB_BLU) << 10)

        SHIFT 1
    ENDR

    PURGE CODE_LEN, CODE_STR, CODE_HEX, RGB_RED, RGB_GRN, RGB_BLU
ENDM


/*******************************************************************************
**                                                                            **
**      SOUND MACROS                                                          **
**                                                                            **
*******************************************************************************/

/*
Starts a new sound entry

Parameters:
- 1 = Sound Channel # (1 - 4)
- 2 = Length (Total number of individual sound entries)
*/
MACRO sound_entry_start
    IF !DEF(SOUND_ENTRY_START)
        DEF SOUND_ENTRY_START = 0
    ENDC

    IF (_NARG != 2)
        FAIL "`sound_entry_start` requires 2 parameters and parsed {d:_NARG}!"
    ELIF (\1 < 1 && \1 > 4)
        FAIL "Sound Channel must be a number 1 - 4 and instead parsed \1!"
    ELIF (\2 < 1)
        FAIL "Sound Length must be a number >= 1 and instead parsed \2!"
    ELIF (SOUND_ENTRY_START == 1)
        FAIL "Must stop previous sound entry before starting a new entry!"
    ENDC

    REDEF SOUND_ENTRY_START = 1
    REDEF CURRENT_SOUND_CHANNEL = \1
    REDEF CURRENT_SOUND_LENGTH = \2
    REDEF CURRENT_SOUND_LENGTH_COUNT = 0
        
    DB $FF & ($C0 | CURRENT_SOUND_CHANNEL)
    DB $FF & ((CURRENT_SOUND_LENGTH + 1))
ENDM

/*
Stops the current sound entry
*/
MACRO sound_entry_stop
    IF (_NARG != 0)
        FAIL "`sound_entry_stop` requires 0 parameters and parsed {d:_NARG}!"
    ELIF !DEF(SOUND_ENTRY_START) || !DEF(CURRENT_SOUND_CHANNEL) || !DEF(CURRENT_SOUND_LENGTH) || !DEF(CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Must start a sound entry before stopping an entry!"
    ELIF (SOUND_ENTRY_START != 1)
        FAIL "Must start a sound entry before stopping an entry!"
    ELIF (CURRENT_SOUND_LENGTH != CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Declared sound length of {d:CURRENT_SOUND_LENGTH} does not match number of entries {d:CURRENT_SOUND_LENGTH_COUNT}"
    ENDC

    REDEF SOUND_ENTRY_START = 0

    DB $01
    DB $00
    DB $00
    DB $00
    DB $00
    DB $00
    DB $00
    DB $00
ENDM

/*
Adds a sound entry using channel 1

Parameters:
- 1 = Duration (# of 64Hz ticks to play this sound entry)
- `rNR10`
  - 2 = Sweep Pace (0 - 7)
  - 3 = Sweep Direction (0 - 1)
  - 4 = Sweep Step (0 - 7)
- `rNR11`
  - 5 = Wave Duty (0 - 3)
  - 6 = Length Timer (0 - 31)
- `rNR12`
  - 7 = Initial Volume (0 - 15)
  - 8 = Envelope Direction (0 - 1)
  - 9 = Envelope Pace (0 - 7)
- `rNR14`
  - 10 = Length Enable (0 - 1)
- `rNR13` & `rNR14`
  - 11 = Frequency (0 - 2047)
*/
MACRO sound_entry_ch1
    IF (_NARG != 11)
        FAIL "`sound_entry_ch1` requires 11 parameters and parsed {d:_NARG}!"
    ELIF !DEF(SOUND_ENTRY_START) || !DEF(CURRENT_SOUND_CHANNEL) || !DEF(CURRENT_SOUND_LENGTH) || !DEF(CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (SOUND_ENTRY_START != 1)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (CURRENT_SOUND_CHANNEL != 1)
        FAIL "Cannot add a Ch1 entry inside another channel!"
    ENDC

    REDEF CURRENT_SOUND_LENGTH_COUNT += 1

    DB $FF & ((\1))
    DB $FF & ((\2 << 4) | (\3 << 3) | (\4 << 0))
    DB $FF & ((\5 << 6) | (\6 << 0))
    DB $FF & ((\7 << 4) | (\8 << 3) | (\9 << 0))
    DB $FF & (LOW(\<11>))
    DB $FF & ((%10000000) | (\<10> << 6) | (HIGH(\<11>) << 0))
    DB $00
    DB $00
ENDM

/*
Adds a sound entry using channel 2

Parameters:
- 1 = Duration (# of 64Hz ticks to play this sound entry)
- `rNR21`
  - 2 = Wave Duty (0 - 3)
  - 3 = Length Timer (0 - 31)
- `rNR22`
  - 4 = Initial Volume (0 - 15)
  - 5 = Envelope Direction (0 - 1)
  - 6 = Envelope Pace (0 - 7)
- `rNR24`
  - 7 = Length Enable (0 - 1)
- `rNR23` & `rNR24`
  - 8 = Frequency (0 - 2047)
*/
MACRO sound_entry_ch2
    IF (_NARG != 8)
        FAIL "`sound_entry_ch2` requires 8 parameters and parsed {d:_NARG}!"
    ELIF !DEF(SOUND_ENTRY_START) || !DEF(CURRENT_SOUND_CHANNEL) || !DEF(CURRENT_SOUND_LENGTH) || !DEF(CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (SOUND_ENTRY_START != 1)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (CURRENT_SOUND_CHANNEL != 2)
        FAIL "Cannot add a Ch2 entry inside another channel!"
    ENDC

    REDEF CURRENT_SOUND_LENGTH_COUNT += 1
    
    DB $FF & (\1)
    DB $FF & ((\2 << 6) | (\3 << 0))
    DB $FF & ((\4 << 4) | (\5 << 3) | (\6 << 0))
    DB $FF & (LOW(\8))
    DB $FF & ((%10000000) | (\7 << 6) | (HIGH(\8) << 0))
    DB $00
    DB $00
    DB $00
ENDM

/*
Adds a sound entry using channel 3

Parameters:
- 1 = Duration (# of 64Hz ticks to play this sound entry)
- `rNR31`
  - 2 = Length Timer (0 - 255)
- `rNR32`
  - 3 = Volume Level (0 - 3)
- `rNR34`
  - 4 = Length Enable (0 - 1)
- `rNR33` & `rNR34`
  - 5 = Frequency (0 - 2047)
*/
MACRO sound_entry_ch3
    IF (_NARG != 5)
        FAIL "`sound_entry_ch3` requires 5 parameters and parsed {d:_NARG}!"
    ELIF !DEF(SOUND_ENTRY_START) || !DEF(CURRENT_SOUND_CHANNEL) || !DEF(CURRENT_SOUND_LENGTH) || !DEF(CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (SOUND_ENTRY_START != 1)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (CURRENT_SOUND_CHANNEL != 3)
        FAIL "Cannot add a Ch3 entry inside another channel!"
    ENDC

    REDEF CURRENT_SOUND_LENGTH_COUNT += 1

    DB $FF & ((\1))
    DB $FF & ((%10000000))
    DB $FF & ((\2))
    DB $FF & ((\3 << 5))
    DB $FF & (LOW(\5))
    DB $FF & ((%10000000) | (\4 << 6) | (HIGH(\5) << 0))
    DB $00
    DB $00
ENDM

/*
Adds a sound entry using channel 4

Parameters:
- 1 = Duration (# of 64Hz ticks to play this sound entry)
- `rNR41`
  - 2 = Length Timer (0 - 63)
- `rNR42`
  - 3 = Initial Volume (0 - 15)
  - 4 = Envelope Direction (0 - 1)
  - 5 = Envelope Pace (0 - 7)
- `rNR43`
  - 6 = Clock Shift (0 - 15)
  - 7 = LFSR Width (0 - 1)
  - 8 = Clock Divider (0 - 7)
- `rNR44`
  - 9 = Length Enable (0 - 1)
*/
MACRO sound_entry_ch4
    IF (_NARG != 9)
        FAIL "`sound_entry_ch4` requires 9 parameters and parsed {d:_NARG}!"
    ELIF !DEF(SOUND_ENTRY_START) || !DEF(CURRENT_SOUND_CHANNEL) || !DEF(CURRENT_SOUND_LENGTH) || !DEF(CURRENT_SOUND_LENGTH_COUNT)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (SOUND_ENTRY_START != 1)
        FAIL "Must start a sound entry before adding entry data!"
    ELIF (CURRENT_SOUND_CHANNEL != 4)
        FAIL "Cannot add a Ch4 entry inside another channel!"
    ENDC

    REDEF CURRENT_SOUND_LENGTH_COUNT += 1

    DB $FF & ((\1))
    DB $FF & ((\2))
    DB $FF & ((\3 << 4) | (\4 << 3) | (\5 << 0))
    DB $FF & ((\6 << 4) | (\7 << 3) | (\8 << 0))
    DB $FF & ((%10000000) | (\9 << 6))
    DB $00
    DB $00
    DB $00
ENDM


/*******************************************************************************
**                                                                            **
**      PURGE INTERNAL CONSTANTS                                              **
**                                                                            **
*******************************************************************************/

PURGE REQUIRED_RGBDS_VERSION, REQUIRED_HARDWARE_VERSION


/*******************************************************************************
**                                                                            **
**      PURGE INTERNAL MACROS                                                 **
**                                                                            **
*******************************************************************************/

PURGE check_rgbds_version
