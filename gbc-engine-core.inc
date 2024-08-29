; UPDATE THIS LINE WHEN CHANGING THE REVISION NUMBER
DEF GBC_ENGINE_CORE_VERSION    EQUS "1.3.0"

; REQUIRED RGBDS VERSION
DEF REQUIRED_RGBDS_VERSION     EQUS "0.7.0"

; REQUIRE HARDWARE.INC VERSION
DEF REQUIRED_HARDWARE_VERSION  EQUS "4.9.1"


/*******************************************************************************
**
**  PUSH LINKER STACKS
**
*******************************************************************************/

PUSHC ; charmap
PUSHS ; section
PUSHO ; options


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

; Creates the ASCII charactermap used internally for the crash screen.
MACRO create_gb_engine_core_charmap
    REDEF CHARMAP_START EQU 32  ; ASCII code for (space)
    REDEF CHAR_LIST EQUS STRCAT(#""" !"#$%&'()*+,-./""", \
                                     #"""0123456789:;<=>?""", \
                                     #"""@ABCDEFGHIJKLMNO""", \
                                     #"""PQRSTUVWXYZ[\]^_""", \
                                     #"""`abcdefghijklmno""", \
                                     #"""pqrstuvwxyz{|}~©""")

    NEWCHARMAP gb_engine_core_charmap ; Create default ASCII charmap

    FOR CHAR, STRLEN("{CHAR_LIST}")
       CHARMAP STRSUB("{CHAR_LIST}", CHAR + 1, 1), CHAR + CHARMAP_START
    ENDR

    PURGE CHARMAP_START
    PURGE CHAR_LIST
ENDM


/*******************************************************************************
**
**  INTERNAL CONSTANTS
**
*******************************************************************************/

DEF TRUE    EQU 1
DEF FALSE   EQU 0

DEF ASSERT_MAIN EQUS #"""
Function `_Main` should be in ROM0!
gbc-engine-core hands off code execution to a function labeled `_Main` when done with initial setup.

The function can assume the following:
 - All ram areas are cleared
 - LCD is off
 - Interrupts are disabled
"""

DEF ASSERT_FONT_01 EQUS #"""
Tileset `_FontTiles` should be in ROM0!
gbc-engine-core requires a .1bpp set of font tiles in ASCII format for the crash screen.

Format:

 !"#$%&'()*+,-./
0123456789:;<=>?
@ABCDEFGHIJKLMNO
PQRSTUVWXYZ[\]^_
`abcdefghijklmno
pqrstuvwxyz{|}~
"""

DEF ASSERT_FONT_02 EQUS #"""
gbc-engine-core expects `_FontTiles` to have sublabel `.end:`, be formated as .1bpp, and be 96 tiles long.
"""

DEF BOOTUP_C_SGB            EQU $14

RSRESET
DEF CRASH_EXECUTING_FF      RB 1
DEF CRASH_NULL_POINTER      RB 1
DEF CRASH_EXECUTING_RAM     RB 1
DEF CRASH_RETURN_FROM_MAIN  RB 1
DEF CRASH_STACK_OVERFLOW    RB 1
DEF CRASH_STACK_UNDERFLOW   RB 1
DEF CRASH_DIVIDE_BY_ZERO    RB 1
DEF CRASH_HARDWARE          RB 1
DEF NUMBER_OF_CRASH_CODES   RB 0
DEF CRASH_UNKNOWN_ERROR     RB 1

DEF CRASH_CODE_SIZE         EQU 4
DEF CRASH_TEXT_SIZE         EQU 18

DEF VRAM_SIZE               EQU _SRAM - _VRAM
DEF TILE_BLOCK_SIZE         EQU (VRAM_SIZE - (_SRAM - _SCRN0)) / 3
DEF TILE_SIZE               EQU $10
DEF OFFSET                  EQU 128

DEF SRAM_SIZE               EQU _RAM - _SRAM
DEF SRAM_BUFFER_SIZE        EQU $100

DEF RAM_SIZE                EQU (_RAMBANK - _RAM) * 2
DEF STACK_SIZE              EQU $100
DEF STACK_BUFFER            EQU $10

DEF HRAM_SIZE               EQU rIE - _HRAM
DEF hTempStack              EQU rIE - 1

DEF DEFAULT_DMG_PALETTE     EQU %11100100
DEF DEFAULT_DMG_FADE_1      EQU %11100101
DEF DEFAULT_DMG_FADE_2      EQU %11101010
DEF DEFAULT_DMG_FADE_3      EQU %11111111

DEF INVERTED_DMG_PALETTE    EQU (~DEFAULT_DMG_PALETTE)
DEF INVERTED_DMG_FADE_1     EQU (~DEFAULT_DMG_FADE_1)
DEF INVERTED_DMG_FADE_2     EQU (~DEFAULT_DMG_FADE_2)
DEF INVERTED_DMG_FADE_3     EQU (~DEFAULT_DMG_FADE_3)

DEF RLE_MAGIC_NUMBER        EQU $30


/*******************************************************************************
**
**  VERSION CHECKS
**
*******************************************************************************/

; check to ensure hardware.inc has been included and meets minimum version
    rev_Check_hardware_inc {REQUIRED_HARDWARE_VERSION}

; check to ensure RGBDS binaries meet minimum version
    check_rgbds_version {REQUIRED_RGBDS_VERSION}


/*******************************************************************************
**
**  ASCII CHARMAP
**
*******************************************************************************/

    create_gb_engine_core_charmap


/*******************************************************************************
**
**  RST VECTORS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0000]

/*
RST Vector to catch null pointer values and cause a crash

This traps jumps to $0000, which is a common "default" pointer.
$FFFF is another one, but reads rIE as the instruction byte;
after which the Program Counter wraps around to address $0000.
Thus, we put two `nop`s that may serve as operands before soft-crashing.
The operand(s) will always be 0, so even jumps will work fine. Nice!
*/
_NULL::
    nop
    nop
    ld a, CRASH_NULL_POINTER
    jp _CrashHandler

DS $0008 - @, $FF

/*
RST Vector for `jp hl` with additional error handling

Crashes if attempting to jump to RAM.

Internally called by
- the custom interrupt handler
- `_FarCall`
- `_FarCallPreserve`
- `_JumpTable`
- `_JumpTableBank`
*/
_CallHL::
    bit 7, h
    ld a, CRASH_EXECUTING_RAM
    jp nz, _CrashHandler
    jp hl

DS $0010 - @, $FF

/*
RST Vector to set the VBlank interrupt handler

Inputs:
- bc = function pointer
*/
_SetVBLHandler::
    ld hl, hVBLHandler
    jr _SetInterruptHandler

DS $0018 - @, $FF

/*
RST Vector to set the LCD Stat interrupt handler

Inputs:
- bc = function pointer
*/
_SetLCDHandler::
    ld hl, hLCDHandler
    jr _SetInterruptHandler

DS $0020 - @, $FF

/*
RST Vector to set the Timer interrupt handler

Inputs:
- bc = function pointer
*/
_SetTIMHandler::
    ld hl, hTIMHandler
    jr _SetInterruptHandler

DS $0028 - @, $FF

/*
RST Vector to set the Serial interrupt handler

Inputs:
- bc = function pointer
*/
_SetSIOHandler::
    ld hl, hSIOHandler
    jr _SetInterruptHandler

DS $0030 - @, $FF

/*
RST Vector to set the Joypad interrupt handler

Inputs:
- bc = function pointer
*/
_SetJOYHandler::
    ld hl, hJOYHandler
    jr _SetInterruptHandler

DS $0038 - @, $FF

/*
RST Vector to force a crash

This traps arbitrary code executions of the byte $FF,
which is the recommended "default" padding value for empty ROM space.
The opcode for rst $38 is $FF, so any attempts to execute code in
such empty space will instead end up here.

It can also be called directly to force any crash code by setting
`hCrashCode` before calling `rst _Crash`.

Inputs:
- `hCrashCode`
*/
_Crash::
    ldh a, [hCrashCode]
    jp _CrashHandler

DS $0040 - @, $FF

ENDSECTION


/*******************************************************************************
**
**  INT VECTORS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0040]

_VBL:
    push hl
    push de
    push bc
    ld hl, hVBLHandler
    jr _InterruptVBL

DS $0048 - @, $FF

_LCD:
    push hl
    push de
    push bc
    ld hl, hLCDHandler
    jr _InterruptCommon

DS $0050 - @, $FF

_TIM:
    push hl
    push de
    push bc
    ld hl, hTIMHandler
    jr _InterruptCommon

DS $0058 - @, $FF

_SIO:
    push hl
    push de
    push bc
    ld hl, hSIOHandler
    jr _InterruptCommon

DS $0060 - @, $FF

_JOY:
    push hl
    push de
    push bc
    ld hl, hJOYHandler
    jr _InterruptCommon

DS $0068 - @, $FF

ENDSECTION


/*******************************************************************************
**
**  INTERRUPT FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0068]

/*
Sets Interrupt Handler

Inputs:
- hl = dest handler
- bc = function pointer
*/
_SetInterruptHandler:
    ld [hl], c
    inc hl
    ld [hl], b
    ret

; Set hIsVBlankFlag and execute Interrupt Handler
_InterruptVBL:
    push af
    ld a, TRUE
    ldh [hIsVBlankFlag], a
    jr _RunInterruptHandler

; Shared Interrupt jump point
_InterruptCommon:
    push af
   ; fallthrough

/*
Execute Interrupt Handler

Inputs:
- hl = interrupt handler HRAM variable
*/
_RunInterruptHandler:
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    rst _CallHL

    pop af
    pop bc
    pop de
    pop hl

    reti

ENDSECTION


/*******************************************************************************
**
**  WAIT FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Wait for b >= LY (Scanline)

Inputs:
- b = LY to wait for

Trashes:
- c
*/
_WaitForScanlineHigh::
    ld c, LOW(rLY)
.wait:
    ld a, [c]
    cp a, b
    jr c, .wait
    ret

/*
Wait for b < LY (Scanline)

Inputs:
- b = LY to wait for

Trashes:
- c
*/
_WaitForScanlineLow::
    ld c, LOW(rLY)
.wait:
    ld a, [c]
    cp a, b
    jr nc, .wait
    ret

; Wait for safe VRAM access (HBlank and/or VBlank mode)
_WaitForScreenBlank::
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, _WaitForScreenBlank
    ret

; Wait for next VBlank Interrupt
_WaitForVBLInterrupt::
    xor a
    ldh [hIsVBlankFlag], a
.wait:
    halt
    nop
    ldh a, [hIsVBlankFlag]
    and a
    jr z, .wait
    ret

/*
Wait for multiple VBlank Interrupts

Inputs:
- d = frames to wait
*/
_WaitForVBLInterruptMultiple::    
    inc e
    dec e
    ret z

    xor a
    ldh [hIsVBlankFlag], a
.wait:
    halt
    nop
    ldh a, [hIsVBlankFlag]
    and a
    jr z, .wait
    dec d
    jr nz, .wait
    ret

ENDSECTION


/*******************************************************************************
**
**  INPUT HANDLER FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Polls joypad register and sets appropriate HRAM variables. If soft reset is enabled,
a soft reset will be initialized if `Start + Select + A + B` are all held.

Returns:
- `hKeysHeld`
- `hKeysPressed`
- `hKeysReleased`

Trashes:
- af
- bc

See also:
- `_EnableSoftReset`
- `_DisableSoftReset`
*/
_ScanKeys::
    ld a, P1F_GET_DPAD
    call .getNibble         ; get DPAD nibble
    swap a                  ; swap bits to upper nibble
    ld b, a                 ; b = current state (DPAD)

    ld a, P1F_GET_BTN
    call .getNibble         ; get BTN nibble
    xor a, b                ; Mix with DPAD bits, and invert all bits (such that pressed=1)
    ld b, a                 ; b = current state (DPAD + BTN)

    ld a, P1F_GET_NONE      ; reset joypad register
    ldh [rP1], a

    ld a, b                 ; check if reset key combo is pressed
    cp a, PADF_START | PADF_SELECT | PADF_B | PADF_A
    jr z, .tryReset

.noReset:                  ; b = new state
    ldh a, [hKeysHeld]
    ld c, a                 ; c = old state

    cpl
    and a, b
    ldh [hKeysPressed], a   ; pressed = (NOT old) AND new

    ld a, b
    ldh [hKeysHeld], a      ; held = new

    cpl
    and a, c
    ldh [hKeysReleased], a  ; released = (NOT current) AND old

    ret

.getNibble:
    ldh [rP1], a
    call .burn              ; burn 10 cycles by calling known return
REPT 3
    ldh a, [rP1]            ; poll 3 times (9 cycles) to account for key bounce
ENDR
    or a, %11110000         ; set upper 4 bits
.burn:
    ret

.tryReset:
    ldh a, [hCanResetFlag]  ; only reset if TRUE
    and a
    jr z, .noReset          ; else skip
    jp _Reset

ENDSECTION


/*******************************************************************************
**
**  BUILD DATE
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

_BuildDate:
    DB __ISO_8601_UTC__
.end:

DS $0100 - @, $FF

ENDSECTION


/*******************************************************************************
**
**  CARTRIDGE HEADER
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0100]

_Header:
    nop
    jp _Start

    NINTENDO_LOGO

_MagicStringTitle:
    DS 11, $00
.end:

_MagicStringCode:
    DS 4, $00
.end:

_HeaderCompatibilityCode:
    DS 1, $00
.end:

DS $014C - @, $00

_MagicStringVersion:
    DS 1, $00
.end

DS $0150 - @, $00

ENDSECTION

/*******************************************************************************
**
**  START, RESET, & CRASH HANDLER FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0150]

; Initial boot routine. Determines hardware model from
; the initial values of the following registers:
;
; - a = Monochrome vs Color
; - b = Color vs Advance
; - c = Super Game Boy
; - d = CGB/AGB in MGB compat. mode
;
_Start:
    ld e, a                 ; Backup a into e (non-important startup register)

    xor a                   ; clear hardware flags
    ldh [hIsGBMonochromeFlag], a
    ldh [hIsGBSuperFlag], a
    ldh [hIsGBColorFlag], a
    ldh [hIsGBAdvanceFlag], a

    ld a, e                 ; Check for Hardware model MGB
    sub a, BOOTUP_A_CGB     ; Possible Hardware: MGB, SGB, CGB, AGB
    jr nz, .isMGB

    ld a, c                 ; Check for Hardware model SGB
    sub a, BOOTUP_C_SGB     ; Possible Hardware: SGB, CGB, AGB
    jr z, .isCGB

    ld a, d                 ; Check for Hardware model CGB/AGB in MGB compat. mode
    and a                   ; Possible Hardware: CGB, AGB
    jr z, .isMGB

    ld a, b                 ; Check for Hardware model (GBA)
    and a, BOOTUP_B_AGB     ; Possible Hardware: CGB, AGB
    jr nz, .isAGB
    jr .isCGB
   
.isMGB:
    ld a, TRUE
    ldh [hIsGBMonochromeFlag], a
    jr _Reset

.isSGB:
    ld a, TRUE              ; SGB has all the functionalities of MGB
    ldh [hIsGBMonochromeFlag], a
    ldh [hIsGBSuperFlag], a
    jr _Reset

.isCGB:
    ld a, TRUE
    ldh [hIsGBColorFlag], a
    jr _Reset

.isAGB:
    ld a, TRUE              ; AGB has all the functionalities of CGB
    ldh [hIsGBColorFlag], a
    ldh [hIsGBAdvanceFlag], a
    jr _Reset

_Reset:
    di                      ; disble interrupts

    ld sp, hTempStack       ; Use this as stack for a while

    xor a, a
    ldh [rNR52], a          ; Switch off sound

    ld [rROMB0], a          ; Initialize rom bank
    ldh [rVBK], a           ; Initialize vram bank
    ld [rRAMB], a           ; Initialize sram bank
    ldh [rSVBK], a          ; Initialize wram bank

    call _ScreenOff         ; Turn the screen off

    ld hl, _RAM             ; Calculate checksum of values in RAM
    ld bc, RAM_SIZE
    call _CalculateChecksum

    call _SetRandomSeed     ; Use value to set Random seed

    ld hl, _RAM
    ld bc, RAM_SIZE
    ld d, $00
    call _MemSet            ; Clear RAM

    ld sp, wStackTop        ; With RAM cleared, switch to real stack

    xor a
    ld hl, _VRAM
    ld bc, VRAM_SIZE
    ld d, $00
    call _VideoMemSet       ; Clear VRAM bank 0

    ldh a, [hIsGBColorFlag]
    and a
    jr z, .notColorEnabled

    ld a, 1
    ld hl, _VRAM
    ld bc, VRAM_SIZE
    ld d, $00
    call _VideoMemSet       ; Clear VRAM bank 1

    call _SetBackgroundPaletteBlack
    call _SetSpritePaletteBlack

.notColorEnabled:
    ld hl, _HRAM + (hRandomState.end - _HRAM)
    ld b, HRAM_SIZE - (hRandomState.end - _HRAM)
    ld a, $00
    call _MemSetFast        ; Clear HRAM (but not the variables we've already set)

    call _InitOAM           ; With HRAM filled with $00, copy OAM Refresh function to HRAM
    call _RefreshOAM        ; With RAM filled with $00, this will clear also clear OAM

    ld a, [_HeaderCompatibilityCode]
    cp a, CART_COMPATIBLE_GBC
    jr nz, .done

    ldh a, [hIsGBColorFlag]
    and a
    ld a, CRASH_HARDWARE
    jr z, _CrashHandler

.done:
ASSERT FATAL, BANK(_Main) == 0, "{s:ASSERT_MAIN}"
    call _Main              ; Real program starts here

    ld a, CRASH_RETURN_FROM_MAIN
    ; Should never reach this point!
    ; fallthrough to CrashHandler

_CrashHandler:
    di

    cp a, NUMBER_OF_CRASH_CODES
    jr c, .validCrashCode
    ld a, CRASH_UNKNOWN_ERROR

.validCrashCode:
    ldh [hCrashCode], a     ; Save crash code

    xor a, a
    ldh [rNR52], a          ; Switch off sound

    call _ScreenOff         ; Turn off screen

    xor a
    ld hl, _VRAM
    ld bc, VRAM_SIZE
    ld d, $00
    call _VideoMemSet       ; Clear VRAM bank 0

ASSERT FATAL, BANK(_FontTiles) == 0, "{s:ASSERT_FONT_01}"
ASSERT FATAL, ((_FontTiles.end - _FontTiles) * 2) == (vBLK21.end - vBLK21.32), "{s:ASSERT_FONT_02}"
    xor a
    ld hl, _FontTiles
    ld bc, (_FontTiles.end - _FontTiles)
    ld de, vBLK21.32
    call _VideoMemCopy1BitPerPixel

    xor a
    ld hl, _CrashTextTileMap
    ld bc, (_CrashTextTileMap.end - _CrashTextTileMap)
    ld de, vSCRN0
    call _VideoMemCopy

    xor a
    ld hl, _MagicStringTitle
    ld b, (_MagicStringCode.end - _MagicStringTitle)
    ld de, vSCRN0.y1x2
    call _VideoMemCopyFast

    ld hl, _MagicStringVersion
    ld a, [hl]
    add "0"
    ld d, a
    ld hl, vSCRN0.y1x17
    ld b, 1
    xor a
    call _VideoMemSetFast

    ldh a, [hCrashCode]
    ld b, CRASH_CODE_SIZE
    ld c, (NUMBER_OF_CRASH_CODES + 1)
    ld hl, _CrashCodes
    call _DataTable

    xor a
    ld b, CRASH_CODE_SIZE
    ld de, vSCRN0.y11x13
    call _VideoMemCopyFast

    ldh a, [hCrashCode]
    ld b, CRASH_TEXT_SIZE
    ld c, (NUMBER_OF_CRASH_CODES + 1)
    ld hl, _CrashText
    call _DataTable
    
    xor a
    ld b, CRASH_TEXT_SIZE
    ld de, vSCRN0.y13x1
    call _VideoMemCopyFast

    xor a
    ld hl, _BuildDate + 2
    ld b, (_BuildDate.end - (_BuildDate + 2))
    ld de, vSCRN0.y16x1
    call _VideoMemCopyFast

    ldh a, [hIsGBColorFlag]
    and a
    jr z, .notColorEnabled

    ld a, 1
    ld hl, _VRAM
    ld bc, VRAM_SIZE
    ld d, $00
    call _VideoMemSet       ; Clear VRAM bank 1

    call _SetBackgroundPaletteBlack
    call _SetSpritePaletteBlack

    xor a
    ld hl, _CrashPalette
    call _SetBackgroundPalette

.notColorEnabled:
    xor a
    ldh [rSCY], a
    ldh [rSCX], a

    ld a, LCDCF_ON | LCDCF_BGON
    ldh [rLCDC], a

    call _SetDMGPalettesDefault

.loop
    halt
    nop
    jr .loop
    ret

_CrashCodes:
    DB "{04X:CRASH_EXECUTING_FF}"
    DB "{04X:CRASH_NULL_POINTER}"
    DB "{04X:CRASH_EXECUTING_RAM}"
    DB "{04X:CRASH_RETURN_FROM_MAIN}"
    DB "{04X:CRASH_STACK_OVERFLOW}"
    DB "{04X:CRASH_STACK_UNDERFLOW}"
    DB "{04X:CRASH_DIVIDE_BY_ZERO}"
    DB "{04X:CRASH_HARDWARE}"
    DB "00FF"
.end:
ASSERT (_CrashCodes.end - _CrashCodes) == ((NUMBER_OF_CRASH_CODES + 1) * CRASH_CODE_SIZE)

_CrashText:
; col# 0                  19
;      |..................|
    DB "Executing $FF     "
    DB "NULL pointer      "
    DB "Executing RAM     "
    DB "Return from Main  "
    DB "Stack overflow    "
    DB "Stack underflow   "
    DB "Divide by zero    "
    DB "GBC-only cartridge"
    DB "Unknown error     "
.end:
ASSERT (_CrashText.end - _CrashText) == ((NUMBER_OF_CRASH_CODES + 1) * CRASH_TEXT_SIZE)

_CrashTextTileMap:
; col#  0                 19          31
;       |..................|...........|  ; row#
    DB "                    ............" ; — 0
    DB "                    ............" ; .
    DB "                    ............" ; .
    DB "       ERROR!       ............" ; .
    DB " ------------------ ............" ; .
    DB "                    ............" ; .
    DB " Please report this ............" ; .
    DB " crash, with a pic, ............" ; .
    DB " to the developer   ............" ; .
    DB " on GitHub.com :)   ............" ; .
    DB "                    ............" ; .
    DB " Error Code:        ............" ; .
    DB "                    ............" ; .
    DB "                    ............" ; .
    DB "                    ............" ; .
    DB " Build Date:        ............" ; .
    DB "                    ............" ; .
    DB "                    ............" ; — 17
REPT 14                                   ; .
    DB "................................" ; .
ENDR                                      ; — 31
.end:

_CrashPalette:
    DW (00 << 0) + (00 << 5) + (15 << 10)
    DW (00 << 0) + (00 << 5) + (15 << 10)
    DW (00 << 0) + (00 << 5) + (15 << 10)
    DW (31 << 0) + (31 << 5) + (31 << 10)
.end:

ENDSECTION


/*******************************************************************************
**
**  SOFT RESET SWITCH FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Enables Soft Reset
_EnableSoftReset::
    ld a, TRUE
    ldh [hCanResetFlag], a
    ret

; Disables Soft Reset
_DisableSoftReset::
    ld a, FALSE
    ldh [hCanResetFlag], a
    ret

ENDSECTION


/*******************************************************************************
**
**  STACK CHECK FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Verifies that `sp` is within the bounds of the stack and
causes a crash on detection of overflow.

Trashes:
- af
- hl
*/
_CheckForStackOverflow::
    ld hl, sp - STACK_BUFFER  ; account for full push/pops and/or a couple of further calls during
    ld a, h
    cp a, HIGH(wStackBottom)
    ld a, CRASH_STACK_OVERFLOW
    jp c, _CrashHandler
    ret

/*
Verifies that `sp` is within the bounds of the stack and
causes a crash on detection of underflow.

Trashes:
- af
- hl
*/
_CheckForStackUnderflow::
IF (STACK_SIZE % $100 == 0)
    ld hl, sp + STACK_BUFFER  ; account for full push/pops and/or a couple of further calls during
    ld a, h
    cp a, HIGH(wStackTop) + 1
    ld a, CRASH_STACK_UNDERFLOW
    jp nc, _CrashHandler
ELSE
    WARN "STACK_SIZE must be a multiple of $100 bytes for effecient stack underflow checking"
ENDC
    ret

ENDSECTION


/*******************************************************************************
**
**  HARDWARE CHECK FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Checks if running on MGB hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBMonochrome::
    ldh a, [hIsGBMonochromeFlag]
    and a
    ret

; Checks if running on SGB hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBSuper::
    ldh a, [hIsGBSuperFlag]
    and a
    ret

; Checks if running on CGB hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBColor::
    ldh a, [hIsGBColorFlag]
    and a
    ret

; Checks if running on AGB hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBAdvance::
    ldh a, [hIsGBAdvanceFlag]
    and a
    ret

ENDSECTION


/*******************************************************************************
**
**  CPU SPEED SWITCH FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Set GBC/GBA CPU Speed to DoubleSpeed
_SetCPUFast::
    ldh a, [hIsGBColorFlag]
    and a
    ret z
    ldh a, [rKEY1]
    and a, KEY1F_DBLSPEED
    jr nz, _SwitchCPUSpeed
    ret

; Set GBA/GBC CPU to SingleSpeed
_SetCPUSlow::
    ldh a, [hIsGBColorFlag]
    and a
    ret z
    ldh a, [rKEY1]
    and a, KEY1F_DBLSPEED
    jr z, _SwitchCPUSpeed
    ret

; Switches CPU Speed
_SwitchCPUSpeed:
    ldh a, [rIE]
    ld b, a ; save IE
    xor a, a
    ldh [rIE], a
    ld  a, P1F_GET_NONE
    ldh [rP1], a
    ld  a, KEY1F_PREPARE
    ldh [rKEY1],a

    stop

    ld a, b
    ldh [rIE], a ; restore IE
    ret

ENDSECTION


/*******************************************************************************
**
**  BANK SWITCH & FARCALL FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Switches to bank `a`
;
; Trashes:
; - af
; - hl
_BankSwitchNew::
    ldh [hDestinationBank], a   ; Save destination bank

    pop hl                      ; Move return address position in stack
    add sp, -2
    push hl
    add sp, +4

    ldh a, [hCurrentBank]       ; Get current bank
    push af                     ; Push current bank to stack
    add sp, -2

    ldh a, [hDestinationBank]   ; Set current bank
    ldh [hCurrentBank], a
    ld [rROMB0], a

    ret

; Returns to previous bank
;
; Trashes:
; - af
; - hl
_BankSwitchPrevious::
    pop hl                      ; Pop return address position in stack
    pop af                      ; Pop return bank from stack and reset

    ldh [hCurrentBank], a
    ld [rROMB0], a

    push hl                     ; Push return address back to stack
    ret

; Calls function `hl` in bank `a` by switching rom banks
; - a = bank where function is located
; - hl = function
;
; Arugments can be passed via `bc` and `de`
; - bc = args
; - de = args
_FarCall::
    ldh [hDestinationBank], a   ; Save destination bank

    ldh a, [hCurrentBank]       ; Get current bank
    push af                     ; Push current bank to stack

    ldh a, [hDestinationBank]   ; Set destination bank
    ldh [hCurrentBank], a
    ld [rROMB0], a

    rst _CallHL                 ; Call function at `hl` in destination bank

    pop af                      ; Pop current bank from stack and reset
    ldh [hCurrentBank], a
    ld [rROMB0], a

    ret

; Calls function `hl` in bank `a` by switching rom banks.
; - a = bank where function is located
; - hl = function
;
; Preserves registers `bc` and `de`
_FarCallPreserve::
    push de
    push bc
    call _FarCall
    pop bc
    pop de
    ret

ENDSECTION


/*******************************************************************************
**
**  TABLE FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Loads pointer table from address in `hl` and returns address number `a`.

Crashes if `a` is greater than or equal to table entry count `c`.

Inputs:
- a = pointer table entry
- c = number of table entries
- hl = pointer table address

Returns:
- hl = pointer number `a`

Trashes:
- de

Pointer Table Format:
```
ExamplePointerTable:
    DW _Address1
    DW _Address2
    DW _Address3
    DW _NULL
```
*/
_PointerTable::
    cp a, c
    jp nc, _NULL
    ld d, 0
    ld e, a
    add hl, de
    add hl, de
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    ret

/*
Loads jump table from address in `hl` and jumps to address number `a`.

Crashes if `a` is greater than or equal to table entry count `c`.

Inputs:
- a = jump table entry
- c = number of table entries
- hl = jump table address

Trashes:
- de

Jump Table Format:

```
ExampleJumpTable:
    DW _Address1
    DW _Address2
    DW _Address3
    DW _NULL
```
*/
_JumpTable::
    cp a, c
    jp nc, _NULL
    ld d, 0
    ld e, a
    add hl, de
    add hl, de
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    rst _CallHL
    ret

/*
Loads pointer table (with banks) from address in `hl` and returns address number `a`.

Crashes if `a` is greater than or equal to table entry count `c`.

Inputs:
- a = pointer table entry
- c = number of table entries
- hl = pointer table address

Returns:
- a = bank of pointer
- hl = pointer number `a`

Trashes:
- de

Pointer Table Format:
```
_ExamplePointerTable:
    DB BANK(_Address1)
    DW _Address1
    DB BANK(_Address2)
    DW _Address2
    DB BANK(_Address3)
    DW _Address3
    DB BANK(_NULL)
    DW _NULL
```
*/
_PointerTableBank::
    cp a, c
    jp nc, _NULL
    ld d, 0
    ld e, a
    add hl, de
    add hl, de
    add hl, de
    ld a, [hl+]
    ld d, a
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    ld a, d
    ret

/*
Loads jump table (with banks) from address in `hl` and jumps to address number `a`.

Crashes if `a` is greater than or equal to table entry count `c`.

Inputs:
- a = jump table entry
- c = number of table entries
- hl = jump table address

Trashes:
- de

Jump Table Format:

```
ExampleJumpTable:
    DB BANK(_Address1)
    DW _Address1
    DB BANK(_Address2)
    DW _Address2
    DB BANK(_Address3)
    DW _Address3
    DB BANK(_NULL)
    DW _NULL
```
*/
_JumpTableBank::
    cp a, c
    jp nc, _NULL
    ld d, 0
    ld e, a
    add hl, de
    add hl, de
    add hl, de
    ld a, [hl+]
    ld d, a
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    ld a, d
    jp _FarCall

/*
Loads pointer to entry `a` in data table at address `hl` that is
`b` bytes in width and returns new pointer in `hl`.

Crashes if `a` is greater than or equal to table entry count `c`.

Inputs:
- a = data table entry
- b = table byte width
- c = number of table entries
- hl = data table address

Returns:
- hl = pointer to data entry

Trashes:
- de

Data Table Format:
```
ExampleDataTable:
    DB "Example Data 1"
    DB "Example Data 2"
    DB "Example Data 3"
    DW _NULL
```
*/
_DataTable::
    cp a, c
    jp nc, _NULL
    ld d, 0
    ld e, a

.loop:
    add hl, de
    dec b
    jr nz, .loop

    ret

ENDSECTION


/*******************************************************************************
**
**  SRAM HANDLER FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Load data from SRAM to address `de`
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_LoadFromSRAM::
    ld a, CART_SRAM_ENABLE
    ld [rRAMG], a

    push hl
    push de
    push bc
    call _VerifySRAMIntegrity
    pop bc
    pop de
    pop hl

    call _MemCopy

    ld a, CART_SRAM_DISABLE
    ld [rRAMG], a
    ret

; Save data to SRAM to address `hl`
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_SaveToSRAM::
    ld a, CART_SRAM_ENABLE
    ld [rRAMG], a

    call _MemCopy
    call _SaveSRAMIntegrityData

    ld a, CART_SRAM_DISABLE
    ld [rRAMG], a
    ret

_VerifySRAMIntegrity::
   ; Check magic string
    ld hl, _MagicStringTitle
    ld b, (_MagicStringCode.end - _MagicStringTitle)
    ld de, sMagicStringTitle
    call _MemCompare
    jr nz, .checkFail

   ; Check checksum string
    ld hl, _ChecksumString
    ld b, (_ChecksumString.end - _ChecksumString)
    ld de, sChecksumString
    call _MemCompare
    jr nz, .checkFail
   
   ; Check checksum string
    ld hl, _DataString
    ld b, (_DataString.end - _DataString)
    ld de, sDataString
    call _MemCompare
    jr nz, .checkFail

   ; Check SRAM checksum
    ld hl, sDataStart
    ld bc, (_RAM - sDataStart)
    call _CalculateChecksum

    ld hl, sChecksumValue
    ld a, [hl+]
    cp a, d
    jr nz, .checkFail
    ld a, [hl]
    cp a, e
    ret z

.checkFail
   ; Clear SRAM
    ld hl, _SRAM
    ld bc, SRAM_SIZE
    ld d, $00
    call _MemSet
   ; fallthrough

_SaveSRAMIntegrityData:
    ld hl, _MagicStringTitle
    ld b, (_MagicStringCode.end - _MagicStringTitle)
    ld de, sMagicStringTitle
    call _MemCopyFast

    ld hl, _ChecksumString
    ld b, (_ChecksumString.end - _ChecksumString)
    ld de, sChecksumString
    call _MemCopyFast

    ld hl, _DataString
    ld b, (_DataString.end - _DataString)
    ld de, sDataString
    call _MemCopyFast

    ld hl, sDataStart
    ld bc, (_RAM - sDataStart)
    call _CalculateChecksum

    ld hl, sChecksumValue
    ld [hl], d
    inc hl
    ld [hl], e

    ret

; ASCII string saved to SRAM to mark checksum
_ChecksumString:
    DB "SAVE CHECKSUM:"
.end:

; ASCII string saved to SRAM to mark checksum
_DataString:
    DB "DATA START:"
.end:

ENDSECTION


/*******************************************************************************
**
**  CHECKSUM FUNCTION
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Calculates a 2 byte checksum
;
; Inputs:
; - hl = start address
; - bc = byte count
;
; Returns:
; - de = checksum value
;
; Checksum is calculated as follows (BSD checksum):
;
; ```
; u16 sum = 0
; u8 * data = &start;
; for i = 0 to size
;    sum = (sum >> 1) | (sum << 15)
;    sum += data[i]
; ```
_CalculateChecksum::
    ld de, $0000 ; Checksum accumulator

.loop:
    ld a, e     ; save lowest bit of de
    srl d
    rr e        ; de = (u16)de >> 1
    rrca        ; a.7 = a.0
    and a, %10000000
    or a, d
    ld d, a     ; de = ( (u16)de >> 1 ) | (e.0 << 15)

    ld a, [hl+] ; a = read byte

    add a, e
    ld e, a
    ld a, d
    adc a, 0
    ld d, a     ; de += (u16)a

    dec bc
    ld a, b
    or a, c
    jr nz, .loop

    ret

ENDSECTION


/*******************************************************************************
**
**  MATH FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Super fast, unrolled, unsigned 8-bit multiplication.
;
; [6/6] + (7 * [5/6]) + [5/8] = [46/56] Cycles
;
; Inputs:
; - a = operand
; - c = operand
;
; Returns:
; - hl = a * c
_Multiply::
    ld b, $00      ; 2
    ld h, a        ; 1
    ld l, b        ; 1
    add hl, hl     ; 2   = 6/6 cycles
REPT 7 ; bits 7-1
    jr nc, :+      ; 3/2
    add hl, bc     ; 2
:   add hl, hl     ; 2   = 5/6 cycles
ENDR   ; bit 0
    ret nc         ; 5/2
    add hl, bc     ; 2
    ret            ; 4   = 5/8 cycles

; Signed 8-bit multiplication.
;
; Inputs:
; - a = operand (signed)
; - c = operand
;
; Returns:
; - hl = a * c
_MultiplySigned::
    ld e, a
    bit 7, e
    jr nz, .negative
    call _Multiply
    ret

.negative:
    cpl
    inc a
    call _Multiply
    ld a, h
    cpl
    ld h, a
    ld a, l
    cpl
    ld l, a
    inc hl
    ret

; Unsigned 8-bit division.
;
; Inputs:
; - a = operand
; - b = operand
;
; Returns:
; - c = a / b
; - a = a % b
_Divide::
    inc b
    dec b
    jr z, .divideByZero
    ld c, $FF
.continue:
    inc c
    sub a, b
    jr nc, .continue
    add a, b
    and a, a
    ret

.divideByZero:
    ld a, CRASH_DIVIDE_BY_ZERO
    jp _CrashHandler
    ret

; Signed 8-bit division.
;
; Inputs:
; - a = operand (signed)
; - b = operand (signed)
;
; Returns:
; - c = a / b
; - a = a % b
_DivideSigned::
    ld e, $00   ; bit 0 of e = result sign (0/1 = +/-)
    bit 7, a
    jr z, .positiveDividend
    inc e
    cpl         ; change sign
    inc a
.positiveDividend:
    bit 7, b
    jr z, .positiveDivisor
    ld c, a
    ld a, b
    cpl
    inc a
    ld b, a     ; change sign
    inc e
.positiveDivisor:
    call _Divide
    ret c       ; if division by 0, exit now

    bit 0, e
    ret z       ; exit if both signs are the same

    ld b, a
    ld a, c
    cpl
    inc a
    ld c, a
    ld a, b
    ret

ENDSECTION


/*******************************************************************************
**
**  RNG FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Sets random state bits BCDE = DEDE
; 
; Inputs:
; - de = seed
;
; Returns:
; - a = state bits 31-24 (which have the best entropy)
; - b = state bits 31-24
; - c = state bits 23-16
; - d = state bits 15-08
; - e = state bits 07-00 (which have the worst entropy)
;
; Trashes:
; - hl
_SetRandomSeed:
    ld hl, hRandomState
    ld a, d
    ld [hl+], a
    ld a, e
    ld [hl+], a
    ld a, d
    ld [hl+], a
    ld a, e
    ld [hl], a
    ; fallthrough

; Generates a pseudorandom 32-bit integer in BCDE
; using the LCG formula from cc65 rand():
;
; x[i + 1] = (x[i] + $B3) * $01010101
;
; Returns:
; - a = state bits 31-24 (which have the best entropy)
; - b = state bits 31-24
; - c = state bits 23-16
; - d = state bits 15-08
; - e = state bits 07-00 (which have the worst entropy)
;
; Trashes:
; - hl
_GetRandom::
    scf 
    ccf 

    ld hl, hRandomState
    ld a, [hl]
    
    add a, $B3
    ld [hl+], a
    ld e, a
    
    adc a, [hl]
    ld [hl+], a
    ld d, a
    
    adc a, [hl]
    ld [hl+], a
    xor a, e
    ld c, a
    
    adc a, [hl]
    ld [hl], a
    xor a, d
    ld b, a
    
    ret

; Generates a pseudorandom 8-bit integer in A
;
; See: `_GetRandom`
;
; Returns:
; - a = state bits 31-24 (which have the best entropy)
;
; Preserves:
; - bc
; - de
; - hl
_GetRandomByte::
    push bc
    push de
    push hl
    call _GetRandom
    pop hl
    pop de
    pop bc
    ret

ENDSECTION


/*******************************************************************************
**
**  MEMORY FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Sets an area of memory to a specified value.
;
; Inputs:
; - hl = start address
; - bc = byte count
; - d = value
_MemSet::
    ld a, d
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, _MemSet
    ret


; Sets a small area (< 256 bytes) of memory to a specified value.
;
; Inputs:
; - hl = start address
; - b = bytes
; - a = value
_MemSetFast::
    ld [hl+], a
    dec b
    jr nz, _MemSetFast
    ret


; Randomizes an area of memory.
;
; Inputs:
; - hl = start address
; - bc = byte count
_MemSetRandom::
    call _GetRandomByte
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, _MemSetRandom
    ret


; Copies bytes from one area of memory to another.
;
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_MemCopy::
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or a, c
    jr nz, _MemCopy
    ret


; Copies bytes from one small area (< 256 bytes) of memory to another.
;
; Inputs:
; - hl = source address
; - b = bytes
; - de = dest address
_MemCopyFast::
    ld a, [hl+]
    ld [de], a
    inc de
    dec b
    jr nz, _MemCopyFast
    ret


; Decompress and copy RLE formatted data from one area of memory to another.
;
; Algorithm based on:
; `RLE coding for Nintendo GBA/DS`
; `Copyright (C) 2011 CUE`
;
; Inputs:
; - hl = source address
; - de = dest adress
;
; Returns:
; - bc = byte count of decompressed data
_MemCopyDecompressRLE::
    ld a, [hl+]             ; check RLE magic number
    cp a, RLE_MAGIC_NUMBER
    ret nz

    ld a, [hl+]             ; check recorded decompressed byte count
    ld c, a
    ld a, [hl+]
    ld b, a
    ld a, [hl+]             ; limit byte count to 16-bits
    and a, a
    ret nz

    push bc                 ; save byte count for later

.loop:                      ; loop through each block of RLE compressed data
    push bc

    ld a, [hl+]             ; get block header
    bit 7, a
    jr nz, .compressedBlock

.uncompressedBlock:         ; uncompressed blocks are N+1 bytes
    inc a
    ld c, a                 ; size of block for copy loop byte count
    ld b, a                 ; size of block for later subtration from total

.copy:
    ld a, [hl+]
    ld [de], a
    inc de
    dec c
    jr nz, .copy
    
    jr .continue

.compressedBlock:           ; compressed blocks are N+3 bytes
    and a, $7F
    add a, 3
    ld c, a
    ld b, a                 ; record size of block for subtration from total

    ld a, [hl+]
.decompress:
    ld [de], a
    inc de
    dec c
    jr nz, .decompress

.continue:
    ld a, b                 ; b holds the size of the last block

    pop bc

    cpl                     ; we need to sub/sbc a from bc without fudging with
    inc a                   ; hl & de, so we cpl a and use add/adc instead
    add a, c
    ld c, a

    ld a, $FF
    adc a, 0
    add a, b
    ld b, a

    or a, c
    jr nz, .loop

    pop bc                  ; return total byte count of decompressed data

    ret


; Compares bytes from one area of memory to another.
;
; Inputs:
; - hl = source address
; - b = bytes
; - de = dest address
;
; Returns:
; - z = equal
; - nz = not equal
_MemCompare::
    ld c, $00
.loop:
    ld a, [de]
    inc de
    sub a, [hl]
    inc hl
    or a, c
    ld c, a
    dec b
    jr nz, .loop

    ld a, c
    and a, a
    ret


ENDSECTION


/*******************************************************************************
**
**  VIDEO MEMORY FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Safely sets an area of VRAM memory to a specified value.
;
; Inputs:
; - a = VRAM bank
; - hl = start address
; - bc = byte count
; - d = value
_VideoMemSet::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, d
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, .loop
    ret


; Safely sets a small area (< 256 bytes) of VRAM memory to a specified value.
;
; Inputs:
; - a = VRAM bank
; - hl = start address
; - b = bytes
; - d = value
_VideoMemSetFast::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, d
    ld [hl+], a
    dec b
    jr nz, .loop
    ret


; Safely randomizes an area of VRAM memory.
;
; Inputs:
; - a = VRAM bank
; - hl = start address
; - bc = byte count
_VideoMemSetRandom::
    ldh [rVBK], a
.loop1:
    call _GetRandomByte
.loop2:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop2
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, .loop1
    ret


; Safely copies bytes from one area of memory to/from VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - bc = byte count
; - de = dest address
_VideoMemCopy::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or a, c
    jr nz, .loop
    ret


; Safely copies bytes from one small area (< 256 bytes) of memory to/from VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - de = dest address
_VideoMemCopyFast::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl+]
    ld [de], a
    inc de
    dec b
    jr nz, .loop
    ret


; Safely compares bytes from one area of memory to/from VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - de = dest address
;
; Returns:
; - z = equal
; - nz = not equal
_VideoMemCompare::
    ldh [rVBK], a
    ld c, $00
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [de]
    inc de
    sub a, [hl]
    inc hl
    or a, c
    ld c, a
    dec b
    jr nz, .loop

    ld a, c
    and a, a
    ret



; Safely copies bytes from one area of memory to/from VRAM.
;
; Note:
; - For copying .1bpp formated graphics data
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - bc = byte count
; - de = dest address
_VideoMemCopy1BitPerPixel::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl+]
    ld [de], a
    inc de
    ld [de], a
    inc de
    dec bc
    ld a, b
    or a, c
    jr nz, .loop
    ret


; Safely copies bytes from one small area (< 256 bytes) of memory to/from VRAM.
;
; Note:
; - For copying .1bpp formated graphics data
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - de = dest address
_VideoMemCopyFast1BitPerPixel::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl+]
    ld [de], a
    inc de
    ld [de], a
    inc de
    dec b
    jr nz, .loop
    ret


; Safely copies bytes from one area of memory to/from VRAM while incrementally stepping.
;
; Note:
; - For copying tilemap data column by column instead of by row
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - c = increment
; - de = dest address
_VideoMemCopyColumnWrap::
    ldh [rVBK], a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl]
    ld [de], a
    
    xor a
    ld a, e
    add a, SCRN_VX_B
    ld e, a
    ld a, d
    adc a, 0
    and a, $9B
    ld d, a     ; increase dest (8 cycles)


    ld a, b
    ld b, $00
    add hl, bc
    ld b, a     ; increase source (6 cycles)

    dec b
    jr nz, .loop
    ret


; Safely copies bytes from one small area (< 256 bytes) of memory to/from VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - de = dest address
;
; Trashes:
; - c
_VideoMemCopyRowWrap::
    ldh [rVBK], a
    ld a, e
    and a, (SCRN_VX_B - 1)
    ld c, a
.loop:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .loop
    ld a, [hl+]
    ld [de], a
    inc de
    inc c
    bit 5, c
    jr z, .skip
    xor a
    ld c, a
    ld a, e
    sub a, SCRN_VX_B
    ld e, a
    ld a, d
    sbc a, 0
    ld d, a
.skip:
    ld a, d
    and a, $9B
    ld d, a
    dec b
    jr nz, .loop
    ret


ENDSECTION


/*******************************************************************************
**
**  SCREEN FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Safely turn on screen with optional parameters

Inputs:
- a = `rLCDC` flags
*/
_ScreenOn::
    or a, LCDCF_ON
    ldh [rLCDC], a
    ret

; Safely turns LCD off by waiting for VBlank
_ScreenOff::
    ldh a, [rLCDC]
    and a, LCDCF_ON
    ret z                      ; LCD already OFF

    ld b, SCRN_Y
    ld c, LOW(rLY)
.wait:
    ld a, [c]
    cp a, b
    jr c, .wait

    xor a, a
    ldh [rLCDC], a             ; Shutdown LCD
   
    ret

ENDSECTION


/*******************************************************************************
**
**  MONOCHROME PALETTE FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

/*
Sets all Monochrome DMG Palettes to the default Black on White values
*/
_SetDMGPalettesDefault::
    ld a, DEFAULT_DMG_PALETTE
    ld b, DEFAULT_DMG_PALETTE
    ld c, INVERTED_DMG_PALETTE
    jr _SetDMGPalettesCustom

/*
Sets all Monochrome DMG Palettes to inverted White on Black values
*/
_SetDMGPalettesInverted::
    ld a, INVERTED_DMG_PALETTE
    ld b, INVERTED_DMG_PALETTE
    ld c, DEFAULT_DMG_PALETTE
    ; fallthrough

/*
Sets all Monochrome DMG Palettes

Inputs:
- a = `rBGP` palette
- b = `rOBP0` palette
- c = `rOBP1` palette
*/
_SetDMGPalettesCustom::
    ldh [rBGP], a
    ld a, b
    ldh [rOBP0], a
    ld a, c
    ldh [rOBP1], a
    ret

/*
Fades all Monochrome DMG Palettes out, either to all black or all white,
over a set number of frames.

Determines which direction to fade based on the initial palette.

Requires VBlank interrupts to be enabled.

Inputs:
- a = number of frames to wait between each transition step.

Trashes:
- de
*/
_FadeDMGPalettesOut::
    ld e, a
    ldh a, [rBGP]
    cp a, DEFAULT_DMG_PALETTE
    jr z, .fadeOutDefault
    cp a, INVERTED_DMG_PALETTE
    jr z, .fadeOutInverted
    ret

.fadeOutDefault:
    ld a, DEFAULT_DMG_PALETTE
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_FADE_1
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_FADE_2
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_FADE_3
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    jp _WaitForVBLInterruptMultiple

.fadeOutInverted:
    ld a, INVERTED_DMG_PALETTE
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_FADE_1
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_FADE_2
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_FADE_3
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    jp _WaitForVBLInterruptMultiple


/*
Fades all Monochrome DMG Palettes in, either from all black or all white,
over a set number of frames.

Determines which direction to fade based on the initial palette.

Requires VBlank interrupts to be enabled.

Inputs:
- a = number of frames to wait between each transition step.

Trashes:
- de
*/
_FadeDMGPalettesIn::
    ld e, a
    ldh a, [rBGP]
    cp a, DEFAULT_DMG_FADE_3
    jr z, .fadeInDefault
    cp a, INVERTED_DMG_FADE_3
    jr z, .fadeInInverted
    ret

.fadeInDefault:
    ld a, DEFAULT_DMG_FADE_3
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_FADE_2
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_FADE_1
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, DEFAULT_DMG_PALETTE
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    jp _WaitForVBLInterruptMultiple

.fadeInInverted:
    ld a, INVERTED_DMG_FADE_3
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_FADE_2
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_FADE_1
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    call _WaitForVBLInterruptMultiple
    
    ld a, INVERTED_DMG_PALETTE
    ldh [rBGP], a
    ldh [rOBP0], a
    cpl
    ldh [rOBP1], a

    ld d, e
    jp _WaitForVBLInterruptMultiple

ENDSECTION


/*******************************************************************************
**
**  BACKGROUND & WINDOW LAYER FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Get address of tile # `a` in `vBLK01`
;
; Input:
; - a = tile #
;
; Returns:
; - hl = source address ( `vBLK01.#` )
; - de = base address ( `vBLK01` )
_GetBLK01TileAddress::
    ld de, vBLK01
    jr _GetTileAddress

; Get address of tile # `a` in `vBLK21`
;
; Input:
; - a = tile #
;
; Returns:
; - hl = source address ( `vBLK21.#` )
; - de = base address ( `vBLK21` )
_GetBLK21TileAddress::
    add a, OFFSET
    ld de, vBLK21
    ; fallthrough

_GetTileAddress:
    ld h, $00
    ld l, a
REPT 4         ; 2^4 = 16
    add hl, hl ; index * 16
ENDR
    add hl, de
    ret


; Convert grid (row, column) yx position `bc` to pixel coordinates
;
; Input:
; - b = y (in tile rows)
; - c = x (in tile columns)
;
; Returns:
; - b = y (in pixels)
; - c = x (in pixels)
_ConvertGridPositionToPixelPosition:
REPT 3          ; 2^3 = 8
    sla b       ; y * 8
ENDR
REPT 3          ; 2^3 = 8
    sla c       ; y * 8
ENDR
    ret


; Convert pixel yx position `bc` to grid (row, column) coordinates
;
; Input:
; - b = y (in pixels)
; - c = x (in pixels)
;
; Returns:
; - b = y (in tile rows)
; - c = x (in tile columns)
_ConvertPixelPositionToGridPosition:
REPT 3          ; 2^3 = 8
    srl b       ; y / 8
ENDR
REPT 3          ; 2^3 = 8
    srl c       ; y / 8
ENDR
    ret


; Get `vSCRN0` tile # to at yx position `bc`
;
; Input:
; - b = y (in tile rows)
; - c = x (in tile columns)
;
; Returns:
; - a = Tile #
; - hl = source address ( `vSCRN0.y#x#` )
; - de = base address ( `vSCRN0` )
_GetSCRN0BackgroundTile::
    ld de, vSCRN0
    jr _SetBackgroundTile

; Get `vSCRN1` tile # to at yx position `bc`
;
; Input:
; - b = y (in tile rows)
; - c = x (in tile columns)
;
; Returns:
; - a = Tile #
; - hl = source address ( `vSCRN1.y#x#` )
; - de = base address ( `vSCRN1` )
_GetSCRN1BackgroundTile::
    ld de, vSCRN1
    ; fallthrough

_GetBackgroundTile:
    ld h, $00
    ld l, b     ; hl = y
REPT 5          ; 2^5 = 32
    add hl, hl  ; y * 32
ENDR
    ld e, c     ; de = base + x
    add hl, de  ; hl = (base + x) + (y * 32)
    
.wait:          ; wait until VRAM is accessible
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .wait

    ld a, [hl]
    ret


; Set `vSCRN0` tile # to `a` at yx `bc`
;
; Input:
; - a = tile #
; - b = y (in tile rows)
; - c = x (in tile columns)
;
; Returns:
; - hl = source address ( `vSCRN0.y#x#` )
; - de = base address ( `vSCRN0` )
_SetSCRN0BackgroundTile::
    ld de, vSCRN0
    jr _SetBackgroundTile

; Set `vSCRN1` tile # to `a` at yx `bc`
;
; Input:
; - a = tile #
; - b = y (in tile rows)
; - c = x (in tile columns)
;
; Returns:
; - hl = source address ( `vSCRN1.y#x#` )
; - de = base address ( `vSCRN1` )
_SetSCRN1BackgroundTile::
    ld de, vSCRN1
    ; fallthrough

_SetBackgroundTile:
    ld e, b     ; de = base + x
    ld h, $00
    ld l, c     ; hl = y
REPT 5          ; 2^5 = 32
    add hl, hl  ; y * 32
ENDR
    add hl, de  ; hl = (base + x) + (y * 32)
    push af

.wait:          ; wait until VRAM is accessible
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, .wait

    pop af
    ld [hl], a
    ret


ENDSECTION


/*******************************************************************************
**
**  SPRITE LAYER FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Get address of sprite # `a` in `wShadowOAM`
;
; Input:
; - a = sprite #
;
; Returns:
; - hl = source address ( `wShadowOAM.#` )
; - de = base address ( `wShadowOAM` )
_GetSpriteAddress::
    ld h, $00
    ld l, a
REPT 2         ; 2^2 = 4
    add hl, hl  ; # * 4
ENDR
    ld de, wShadowOAM
    add hl, de
    ret

; Set xy position of Sprite
;
; Inputs:
; - b = x
; - c = y
; - hl = `wShadowOAM.#`
_SetSpriteXY::
    ld [hl], c
    inc hl
    ld [hl], b
    ret

; Set the tile of Sprite
;
; Inputs:
; - a = tile
; - hl = `wShadowOAM.#`
_SetSpriteTile::
    inc hl
    inc hl
    ld [hl], a
    ret

; Set the parameters of Sprite
;
; Inputs:
; - a = params
; - hl = `wShadowOAM.#`
_SetSpriteParams::
    inc hl
    inc hl
    inc hl
    ld [hl], a
    ret

ENDSECTION


/*******************************************************************************
**
**  COLOR PALETTE FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Sets all Background Palettes to full white
_SetBackgroundPaletteWhite::
    ld hl, _WhiteoutPalettes
    jr _SetBackgroundPaletteAll

; Sets all Background Palettes to full black
_SetBackgroundPaletteBlack::
    ld hl,_BlackoutPalettes
    ; fallthrough

; Sets all Background Palettes
;
; Inputs:
; - hl = pointer to data for all 8 palettes
_SetBackgroundPaletteAll::
    xor a
    call _SetBackgroundPalette
    ld a, 1
    call _SetBackgroundPalette
    ld a, 2
    call _SetBackgroundPalette
    ld a, 3
    call _SetBackgroundPalette
    ld a, 4
    call _SetBackgroundPalette
    ld a, 5
    call _SetBackgroundPalette
    ld a, 6
    call _SetBackgroundPalette
    ld a, 7
    call _SetBackgroundPalette
    ret

; Set Background Palette
;
; Inputs:
; - a = palette number
; - hl = pointer to data
_SetBackgroundPalette::
    swap a      ; \  multiply
    rrca        ; /  palette by 8
    set BCPSB_AUTOINC, a    ; auto increment
    ldh [rBCPS], a
REPT 8
:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, :-

    ld a, [hl+]
    ldh [rBCPD], a
ENDR
    ret

; Sets all Sprite Palettes to full white
_SetSpritePaletteWhite::
    ld hl, _WhiteoutPalettes
    jr _SetSpritePaletteAll

; Sets all Sprite Palettes to full black
_SetSpritePaletteBlack::
    ld hl,_BlackoutPalettes
    ; fallthrough

; Sets all Sprite Palettes
;
; Inputs:
; - hl = pointer to data for all 8 palettes
_SetSpritePaletteAll::
    xor a
    call _SetSpritePalette
    ld a, 1
    call _SetSpritePalette
    ld a, 2
    call _SetSpritePalette
    ld a, 3
    call _SetSpritePalette
    ld a, 4
    call _SetSpritePalette
    ld a, 5
    call _SetSpritePalette
    ld a, 6
    call _SetSpritePalette
    ld a, 7
    call _SetSpritePalette
    ret

; Set Sprite Palette
;
; Inputs:
;- a = palette number
;- hl = pointer to data
_SetSpritePalette::
    swap a      ; \  multiply
    rrca        ; /  palette by 8
    set OCPSB_AUTOINC, a    ; auto increment
    ldh [rOCPS], a
REPT 8
:
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, :-

    ld a, [hl+]
    ldh [rOCPD], a
ENDR
    ret

_WhiteoutPalettes:
REPT 8
    DW (31 << 0) + (31 << 5) + (31 << 10)
    DW (31 << 0) + (31 << 5) + (31 << 10)
    DW (31 << 0) + (31 << 5) + (31 << 10)
    DW (31 << 0) + (31 << 5) + (31 << 10)
ENDR
.end:

_BlackoutPalettes:
REPT 8
    DW (00 << 0) + (00 << 5) + (00 << 10)
    DW (00 << 0) + (00 << 5) + (00 << 10)
    DW (00 << 0) + (00 << 5) + (00 << 10)
    DW (00 << 0) + (00 << 5) + (00 << 10)
ENDR
.end:

ENDSECTION


/*******************************************************************************
**
**  OAM REFRESH FUNCTIONS
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Initializes OAM Refresh Function in HRAM
_InitOAM:
    ld b, (_OAMRefreshFunction.end - _OAMRefreshFunction)
    ld hl, _OAMRefreshFunction
    ld de, hOAMRefreshFunction
    call _MemCopyFast
    ret

; Refresh OAM by copying data from `wShadowOAM`
;
; Trashes
; - af
_RefreshOAM::
    jp hOAMRefreshFunction

; OAM Refresh function for DMA transfer
_OAMRefreshFunction:
LOAD "GBC-ENGINE-CORE - OAM REFRESH", HRAM, ALIGN[4]

; Copy of OAM Refresh function for DMA transfer in HRAM
hOAMRefreshFunction:
    ld a, HIGH(wShadowOAM)
    ldh [rDMA], a   ; start DMA transfer (starts right after instruction)
    ld a, 40        ; delay for a total of 4×40 = 160 M-cycles
.delay:
    dec a           ; 1 M-cycle
    jr nz, .delay   ; 3 M-cycles
    ret z           ; Conditional `ret` is 1 M-cycle slower, which avoids
                    ; reading from the stack on the last M-cycle of DMA.
ENDL
.end:

ENDSECTION


/*******************************************************************************
**                                                                            **
**      SOUND FUNCTIONS                                                       **
**                                                                            **
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0
    
/*
Initializes the sound driver

Input:
- a = total number of sounds in Sound Pointer Table
- bc = address of Sound Pointer Table
*/
_InitSound::
    ldh [hSoundTotalNumber], a

    ld a, AUDENA_OFF    ; Turn master audio OFF
    ldh [rNR52], a

    ld a, TACF_STOP     ; Turn Timer OFF
    ldh [rTAC], a

    ld hl, hSoundPointerTableAddress
    ld [hl], c
    inc hl
    ld [hl], b          ; Save address of Sound table

    ld hl, _TriangleWaveform
    ld b, $10           ; Copy waveform to WaveRAM
    ld de, _AUD3WAVERAM
    call _MemCopyFast
    
    ldh a, [rIE]        ; Disable Timer interrupt
    and a, (~IEF_TIMER & $FF)
    ldh [rIE], a

    ldh a, [rIF]        ; Clear Timer interrupt flag
    and a, (~IEF_TIMER & $FF)
    ldh [rIF], a

    xor a               ; Start Timer with a 64 Hz trigger
    ldh [rTIMA], a
    ld a, $C0
    ldh [rTMA], a
    ld a, TACF_START|TACF_4KHZ
    ldh [rTAC], a

    ld a, AUDENA_ON     ; Turn master audio ON
    ldh [rNR52], a

    ld a, $77           ; Enable max mixer volume for L / R
    ldh [rNR50], a

    ld a, $FF           ; Enable all channels L / all channels R
    ldh [rNR51], a

    ld a, $01           ; Flag sound as initialized
    ldh [hSoundInitialized], a

    ret

_TriangleWaveform:
    DB $01, $23, $45, $67, $89, $AB, $CD, $EF
    DB $FE, $DC, $BA, $98, $76, $54, $32, $10
.end:

ASSERT (_TriangleWaveform.end - _TriangleWaveform) == $10

/*
Plays a new sound

Inputs:
    - a = Sound number from the Sound Pointer Table defined on `_InitSound`
*/
_PlaySound::
    ldh [hSoundType], a

    ldh a, [hSoundInitialized]
    or a
    ret z

    ld hl, hSoundPointerTableAddress
    ld a, [hl+]
    ld h, [hl]
    ld l, a
    ldh a, [hSoundTotalNumber]
    ld c, a
    ldh a, [hSoundType]
    call _PointerTable

    ld a, [hl+]

    cp $C1
    jr z, _PlayCh1
    cp $C2
    jr z, _PlayCh2
    cp $C3
    jr z, _PlayCh3
    cp $C4
    jr z, _PlayCh4

    ret

_PlayCh1:
    ld a, [hl+]
    ldh [hSoundCh1Length], a

    ld de, hSoundCh1DataTableAddress
    ld a, l
    ld [de], a
    inc de
    ld a, h
    ld [de], a

    xor a
    ldh [hSoundCh1Count], a
    ldh [hSoundCh1Duration], a
    
    jr _PlayEnable

_PlayCh2:
    ld a, [hl+]
    ldh [hSoundCh2Length], a

    ld de, hSoundCh2DataTableAddress
    ld a, l
    ld [de], a
    inc de
    ld a, h
    ld [de], a

    xor a
    ldh [hSoundCh2Count], a
    ldh [hSoundCh2Duration], a
    
    jr _PlayEnable

_PlayCh3:
    ld a, [hl+]
    ldh [hSoundCh3Length], a

    ld de, hSoundCh3DataTableAddress
    ld a, l
    ld [de], a
    inc de
    ld a, h
    ld [de], a
    
    xor a
    ldh [hSoundCh3Count], a
    ldh [hSoundCh3Duration], a
    
    jr _PlayEnable

_PlayCh4:
    ld a, [hl+]
    ldh [hSoundCh4Length], a

    ld de, hSoundCh4DataTableAddress
    ld a, l
    ld [de], a
    inc de
    ld a, h
    ld [de], a

    xor a
    ldh [hSoundCh4Count], a
    ldh [hSoundCh4Duration], a
    
    ; fallthrough

_PlayEnable:
    ldh a, [rIE]    ; Enable Timer interrupt
    or a, IEF_TIMER
    ldh [rIE], a

    ret

/*
Updates the sound driver
*/
_UpdateSound::
    ldh a, [hSoundInitialized]
    or a
    ret z
    ; fallthrough

_UpdateCh1:
    ldh a, [hSoundCh1Duration]
    or a
    jr z, .update

    dec a
    ldh [hSoundCh1Duration], a
    jr _UpdateCh2

.update:
    ldh a, [hSoundCh1Length]
    ld c, a
    ldh a, [hSoundCh1Count]
    cp a, c
    jr nc, _UpdateCh2

    ld hl, hSoundCh1DataTableAddress
    ld a, [hl+]
    ld h, [hl]
    ld l, a

    ldh a, [hSoundCh1Length]
    ld c, a
    ldh a, [hSoundCh1Count]
    ld b, $08
    call _DataTable

    ld a, [hl+]
    ldh [hSoundCh1Duration], a

    ld b, 5
    ld de, rNR10
    call _MemCopyFast

    ld hl, hSoundCh1Count
    inc [hl]

    ; fallthrough

_UpdateCh2:
    ldh a, [hSoundCh2Duration]
    or a
    jr z, .update

    dec a
    ldh [hSoundCh2Duration], a
    jr _UpdateCh3

.update:
    ldh a, [hSoundCh2Length]
    ld c, a
    ldh a, [hSoundCh2Count]
    cp a, c
    jr nc, _UpdateCh3

    ld hl, hSoundCh2DataTableAddress
    ld a, [hl+]
    ld h, [hl]
    ld l, a

    ldh a, [hSoundCh2Length]
    ld c, a
    ldh a, [hSoundCh2Count]
    ld b, $08
    call _DataTable

    ld a, [hl+]
    ldh [hSoundCh2Duration], a

    ld b, 4
    ld de, rNR21
    call _MemCopyFast

    ld hl, hSoundCh2Count
    inc [hl]

    ; fallthrough

_UpdateCh3:
    ldh a, [hSoundCh3Duration]
    or a
    jr z, .update

    dec a
    ldh [hSoundCh3Duration], a
    jr _UpdateCh4

.update:
    ldh a, [hSoundCh3Length]
    ld c, a
    ldh a, [hSoundCh3Count]
    cp a, c
    jr nc, _UpdateCh4

    ld hl, hSoundCh3DataTableAddress
    ld a, [hl+]
    ld h, [hl]
    ld l, a

    ldh a, [hSoundCh3Length]
    ld c, a
    ldh a, [hSoundCh3Count]
    ld b, $08
    call _DataTable

    ld a, [hl+]
    ldh [hSoundCh3Duration], a

    ld b, 5
    ld de, rNR30
    call _MemCopyFast

    ld hl, hSoundCh3Count
    inc [hl]

    ; fallthrough

_UpdateCh4:
    ldh a, [hSoundCh4Duration]
    or a
    jr z, .update

    dec a
    ldh [hSoundCh4Duration], a
    jr _UpdateDisable

.update:
    ldh a, [hSoundCh4Length]
    ld c, a
    ldh a, [hSoundCh4Count]
    cp a, c
    jr nc, _UpdateDisable

    ld hl, hSoundCh4DataTableAddress
    ld a, [hl+]
    ld h, [hl]
    ld l, a

    ldh a, [hSoundCh4Length]
    ld c, a
    ldh a, [hSoundCh4Count]
    ld b, $08
    call _DataTable

    ld a, [hl+]
    ldh [hSoundCh4Duration], a

    ld b, 4
    ld de, rNR41
    call _MemCopyFast

    ld hl, hSoundCh4Count
    inc [hl]

    ; fallthrough

_UpdateDisable:
    ldh a, [hSoundCh1Length]
    ld c, a
    ldh a, [hSoundCh1Count]
    cp a, c
    ret c
    
    ldh a, [hSoundCh2Length]
    ld c, a
    ldh a, [hSoundCh2Count]
    cp a, c
    ret c
    
    ldh a, [hSoundCh3Length]
    ld c, a
    ldh a, [hSoundCh3Count]
    cp a, c
    ret c
    
    ldh a, [hSoundCh4Length]
    ld c, a
    ldh a, [hSoundCh4Count]
    cp a, c
    ret c
    
    ldh a, [rIE]
    and a, (~IEF_TIMER & $FF)
    ldh [rIE], a

    ldh a, [rIF]
    and a, (~IEF_TIMER & $FF)
    ldh [rIF], a

    xor a
    ldh [rTIMA], a

    ret


/*
Wait until no sounds are playing
*/
_WaitSound::
    ldh a, [hSoundInitialized]
    or a
    ret z
    ; fallthrough

_WaitCh1:
    ldh a, [hSoundCh1Length]
    ld c, a
    ldh a, [hSoundCh1Count]
    cp a, c
    jr c, _WaitCh1
    ; fallthrough

_WaitCh2:
    ldh a, [hSoundCh2Length]
    ld c, a
    ldh a, [hSoundCh2Count]
    cp a, c
    jr c, _WaitCh2
    ; fallthrough

_WaitCh3:
    ldh a, [hSoundCh3Length]
    ld c, a
    ldh a, [hSoundCh3Count]
    cp a, c
    jr c, _WaitCh3
    ; fallthrough

_WaitCh4:
    ldh a, [hSoundCh4Length]
    ld c, a
    ldh a, [hSoundCh4Count]
    cp a, c
    jr c, _WaitCh4
    ; fallthrough

_WaitReturn:
    ret

ENDSECTION


/*******************************************************************************
**
**  VRAM
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE - VRAM", VRAM[_VRAM]

UNION

; VRAM Tile Block 0 + 1
; 
; Used with `rLCDC` Tile Addressing mode `LCDCF_BLK01`
;
; Write to a specific tile address using `vBLK01.#`,
; where # is `0 <= # <= 255`
vBLK01::
FOR TILE, (TILE_BLOCK_SIZE / TILE_SIZE)
.{d:TILE}::
    DS TILE_SIZE
ENDR

FOR TILE, OFFSET, ((TILE_BLOCK_SIZE / TILE_SIZE) + OFFSET)
.{d:TILE}::
    DS TILE_SIZE
ENDR
.end::

    DS TILE_BLOCK_SIZE

NEXTU

    DS TILE_BLOCK_SIZE

; VRAM Tile Block 2 + 1
; 
; Used with `rLCDC` Tile Addressing mode `LCDCF_BLK21`
;
; Write to a specific tile address using `vBLK21.#`,
; where # is `0 <= # <= 255`
vBLK21::
FOR TILE, OFFSET, ((TILE_BLOCK_SIZE / TILE_SIZE) + OFFSET)
.{d:TILE}::
    DS TILE_SIZE
ENDR

FOR TILE, (TILE_BLOCK_SIZE / TILE_SIZE)
.{d:TILE}::
    DS TILE_SIZE
ENDR
.end::

ENDU

ASSERT @ == _SCRN0

ENDSECTION


/*******************************************************************************
**
**  VRAM SCRN0
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE - VRAM", VRAM[_SCRN0]

; VRAM Screen 0 Tilemap
;
; Write to a specific tile address using `vSCRN0.y#x#`,
; where both y# and x# are `0 <= # <= 31`
vSCRN0::
FOR COL, SCRN_VY_B
FOR ROW, SCRN_VX_B
.y{d:COL}x{d:ROW}::
    DS 1
ENDR
ENDR
.end::

ASSERT @ == _SCRN1

ENDSECTION


/*******************************************************************************
**
**  VRAM SCRN1
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE - VRAM", VRAM[_SCRN1]

; VRAM Screen 1 Tilemap
;
; Write to a specific tile address using `vSCRN1.y#x#`,
; where both y# and x# are `0 <= # <= 31`
vSCRN1::
FOR COL, SCRN_VY_B
FOR ROW, SCRN_VX_B
.y{d:COL}x{d:ROW}::
    DS 1
ENDR
ENDR
.end::

ASSERT @ == _SRAM

ENDSECTION


/*******************************************************************************
**
**  SRAM
**
*******************************************************************************/

SECTION "GBC-ENGINE-CORE - SRAM", SRAM[_SRAM]

; 256 byte buffer at the start of SRAM
sBuffer:
    DS $100

; Copy of the Magic String used to verify save integrity
sMagicStringTitle:
    DS (_MagicStringTitle.end - _MagicStringTitle)

sMagicStringCode:
    DS (_MagicStringCode.end - _MagicStringCode)

sMagicStringVersion:
    DS (_MagicStringVersion.end - _MagicStringVersion)

    DS 16 ; padding

sChecksumString:
    DS (_ChecksumString.end - _ChecksumString)

; Checksum of SRAM used to verify save integrity
sChecksumValue:
    DS 2

    DS 16 ; padding

sDataString:
    DS (_DataString.end - _DataString)

    DS 5 ; padding

sDataStart:

ENDSECTION


/*******************************************************************************
**
**  WRAM
**
*******************************************************************************/

SECTION FRAGMENT "GBC-ENGINE-CORE - WRAM", WRAM0[_RAM]

wStackBottom:
    DS STACK_SIZE - 2
wStackTop:
    DS 2

DS ALIGN[8]

; DMA will be used to copy this to OAM
;
; Write to a specific sprite address using `wShadowOAM.#`,
; where `0 <= # <= OAM_COUNT`
;
; Write to specific attribute using `wShadowOAM.# + offset`
;
; Offsets:
; - `OAMA_Y`
; - `OAMA_X`
; - `OAMA_TILEID`
; - `OAMA_FLAGS`
wShadowOAM::
FOR SPRITE, OAM_COUNT
.{d:SPRITE}::
    DS sizeof_OAM_ATTRS
ENDR
.end::

ENDSECTION


/*******************************************************************************
**
**  OAM RAM
**
*******************************************************************************/

SECTION "GBC-ENGINE-CORE - OAM", OAM[_OAMRAM]

; WARNING:
;
; Writing directly to OAM can often cause undesired behavior.
; Instead use `wShadowOAM` and call `_RefreshOAM` during the VBlank period.
;
; Write to a specific sprite address using `oSprite.#`,
; where `0 <= # <= OAM_COUNT`
;
; Write to specific attribute using `oSprite.# + offset`
;
; Offsets:
; - `OAMA_Y`
; - `OAMA_X`
; - `OAMA_TILEID`
; - `OAMA_FLAGS`
oSprite::
FOR SPRITE, OAM_COUNT
.{d:SPRITE}::
    DS sizeof_OAM_ATTRS
ENDR
.end::

ENDSECTION


/*******************************************************************************
**
**  HRAM
**
*******************************************************************************/

SECTION "GBC-ENGINE-CORE - HRAM", HRAM[_HRAM]

hIsGBMonochromeFlag:
    DS 1
hIsGBSuperFlag:
    DS 1
hIsGBColorFlag:
    DS 1
hIsGBAdvanceFlag:
    DS 1

hRandomState:
    DS 4
.end:

hCrashCode::
    DS 1

hIsVBlankFlag:
    DS 1

hVBLHandler:
    DS 2
hLCDHandler:
    DS 2
hTIMHandler:
    DS 2
hSIOHandler:
    DS 2
hJOYHandler:
    DS 2

hCanResetFlag:
    DS 1

hKeysHeld::
    DS 1
hKeysPressed::
    DS 1
hKeysReleased::
    DS 1

hCurrentBank:
    DS 1
hDestinationBank:
    DS 1

hSoundInitialized:
    DS 1

hSoundPointerTableAddress:
    DS 2

hSoundTotalNumber:
    DS 1

hSoundType:
    DS 1

hSoundCh1DataTableAddress:
    DS 2

hSoundCh1Length:
    DS 1

hSoundCh1Count:
    DS 1

hSoundCh1Duration:
    DS 1

hSoundCh2DataTableAddress:
    DS 2

hSoundCh2Length:
    DS 1

hSoundCh2Count:
    DS 1

hSoundCh2Duration:
    DS 1

hSoundCh3DataTableAddress:
    DS 2

hSoundCh3Length:
    DS 1

hSoundCh3Count:
    DS 1

hSoundCh3Duration:
    DS 1

hSoundCh4DataTableAddress:
    DS 2

hSoundCh4Length:
    DS 1

hSoundCh4Count:
    DS 1

hSoundCh4Duration:
    DS 1

ENDSECTION


/*******************************************************************************
**
**  PURGE INTERNAL MACROS
**
*******************************************************************************/

PURGE check_rgbds_version, create_gb_engine_core_charmap


/*******************************************************************************
**
**  PURGE INTERNAL CONSTANTS
**
*******************************************************************************/

PURGE REQUIRED_RGBDS_VERSION, REQUIRED_HARDWARE_VERSION, \
      ASSERT_MAIN, ASSERT_FONT_01, ASSERT_FONT_02, \
      BOOTUP_C_SGB, \
      CRASH_CODE_SIZE, CRASH_TEXT_SIZE, \
      VRAM_SIZE, TILE_BLOCK_SIZE, TILE_SIZE, OFFSET, \
      SRAM_SIZE, SRAM_BUFFER_SIZE, \
      RAM_SIZE, STACK_SIZE, STACK_BUFFER, \
      HRAM_SIZE, hTempStack, \
      DEFAULT_DMG_FADE_1, DEFAULT_DMG_FADE_2, DEFAULT_DMG_FADE_3, \
      INVERTED_DMG_FADE_1, INVERTED_DMG_FADE_2, INVERTED_DMG_FADE_3, \
      RLE_MAGIC_NUMBER


/*******************************************************************************
**
**  POP LINKER STACKS
**
*******************************************************************************/

POPC ; charmap
POPS ; section
POPO ; options
