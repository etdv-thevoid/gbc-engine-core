; UPDATE THIS LINE WHEN CHANGING THE REVISION NUMBER
DEF GBC_ENGINE_CORE_VERSION    EQUS "0.4.0"

; REQUIRED RGBDS VERSION
DEF REQUIRED_RGBDS_VERSION     EQUS "0.7.0"

; REQUIRE HARDWARE.INC VERSION
DEF REQUIRED_HARDWARE_VERSION  EQUS "4.9.1"


;###############################################################################
;
;     PUSH LINKER STACKS
;
;###############################################################################

PUSHC ; charmap
PUSHS ; section
PUSHO ; options


;###############################################################################
;
;     MACROS
;
;###############################################################################

; Call with the expected version string to ensure your code
; is compatible with the INCLUDE'd version of `gbc-engine-core.inc`.
;
; Usage: `gbc_engine_core_version` `<min_ver>`
;
; Examples:
; - `gbc_engine_core_version 1.2.3`
; - `gbc_engine_core_version 1.2` (equivalent to 1.2.0)
; - `gbc_engine_core_version 1` (equivalent to 1.0.0)
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

; Call with the expected version string to ensure your code
; is compatible with current version of RGBDS binaries.
;
; Usage: `check_rgbds_version` `<min_ver>`
;
; Examples:
; - `check_rgbds_version 1.2.3`
; - `check_rgbds_version 1.2` (equivalent to 1.2.0)
; - `check_rgbds_version 1` (equivalent to 1.0.0)
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


;###############################################################################
;
;     CONSTANTS
;
;###############################################################################

DEF TRUE    EQU 1
DEF FALSE   EQU 0

RSRESET
DEF CRASH_EXECUTING_FF     RB 1
DEF CRASH_NULL_POINTER     RB 1
DEF CRASH_EXECUTING_RAM    RB 1
DEF CRASH_RETURN_FROM_MAIN RB 1
DEF CRASH_STACK_OVERFLOW   RB 1
DEF CRASH_STACK_UNDERFLOW  RB 1
DEF CRASH_DIVIDE_BY_ZERO   RB 1

DEF NUMBER_OF_CRASH_CODES  RB 0

DEF CRASH_UNKNOWN_ERROR    RB 1

DEF CRASH_CODE_SIZE  EQU 4
DEF CRASH_TEXT_SIZE  EQU 16

DEF VRAM_SIZE        EQU _SRAM - _VRAM
DEF TILE_BLOCK_SIZE  EQU (VRAM_SIZE - (_SRAM - _SCRN0)) / 3
DEF TILE_SIZE        EQU $10
DEF OFFSET           EQU 128

DEF SRAM_SIZE        EQU _RAM - _SRAM
DEF SRAM_BUFFER_SIZE EQU $100

DEF RAM_SIZE         EQU (_RAMBANK - _RAM) * 2
DEF STACK_SIZE       EQU $100
DEF STACK_BUFFER     EQU $10

DEF HRAM_SIZE        EQU rIE - _HRAM
DEF hTempStack       EQU rIE - 1


;###############################################################################
;
;     VERSION CHECKS
;
;###############################################################################

; check to ensure hardware.inc has been included and meets minimum version
   rev_Check_hardware_inc {REQUIRED_HARDWARE_VERSION}

; check to ensure RGBDS binaries meet minimum version
   check_rgbds_version {REQUIRED_RGBDS_VERSION}


;###############################################################################
;
;     ASCII CHARMAP
;
;###############################################################################

   create_gb_engine_core_charmap


;###############################################################################
;
;     RST VECTORS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0000]

; RST Vector to perform soft reset
; 
; This traps jumps to $0000, which is a common "default" pointer.
; $FFFF is another one, but reads rIE as the instruction byte;
; after which the Program Counter wraps around to address $0000.
; Thus, we put two `nop`s that may serve as operands before soft-crashing.
; The operand(s) will always be 0, so even jumps will work fine. Nice!
_NULL::
	nop
	nop
   ld a, CRASH_NULL_POINTER
	jp _CrashHandler

DS $0008 - @, $FF

; RST Vector for `jp hl` with additional error handling
;
; Crashes if attempting to jump to RAM
;
; By default, called by the 
; the interrupt handler executor, `_FarCall`, and `_FarCallPreserve`
_CallHL::
   bit 7, h
   ld a, CRASH_EXECUTING_RAM
   jp nz, _CrashHandler
   jp hl

DS $0010 - @, $FF

; RST Vector to set `hVBLHandler`
; 
; Inputs:
; - bc = function pointer
_SetVBLHandler::
   ld hl, hVBLHandler
   jr _SetInterruptHandler

DS $0018 - @, $FF

; RST Vector to set `hLCDHandler`
; 
; Inputs:
; - bc = function pointer
_SetLCDHandler::
   ld hl, hLCDHandler
   jr _SetInterruptHandler

DS $0020 - @, $FF

; RST Vector to set `hTIMHandler`
; 
; Inputs:
; - bc = function pointer
_SetTIMHandler::
   ld hl, hTIMHandler
   jr _SetInterruptHandler

DS $0028 - @, $FF

; RST Vector to set `hSIOHandler`
; 
; Inputs:
; - bc = function pointer
_SetSIOHandler::
   ld hl, hSIOHandler
   jr _SetInterruptHandler

DS $0030 - @, $FF

; RST Vector to set `hJOYHandler`
; 
; Inputs:
; - bc = function pointer
_SetJOYHandler::
   ld hl, hJOYHandler
   jr _SetInterruptHandler

DS $0038 - @, $FF

; RST Vector to perform a crash
;
; This traps arbitrary code executions of the byte $FF,
; which is the recommended "default" padding value for empty ROM space.
; The opcode for rst $38 is $FF, so any attempts to execute code in
; such empty space will instead end up here.
;
; It can also be called directly to force any crash code by setting
; `hCrashCode` before calling `rst _Crash`.
;
; Inputs:
; - `hCrashCode`
_Crash::
   ldh a, [hCrashCode]
   jp _CrashHandler

DS $0040 - @, $FF

ENDSECTION


;###############################################################################
;
;     INT VECTORS
;
;###############################################################################

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


;###############################################################################
;
;     INTERRUPT SETTER & EXECUTOR
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0[$0068]

; Sets Interrupt Handler
;
; Inputs:
; - hl = dest handler
; - bc = function pointer
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

; Execute Interrupt Handler
;
; Inputs:
; - hl = interrupt handler HRAM variable
_RunInterruptHandler:
   call _CheckForStackOverflow

   ld a, [hl+]
   ld h, [hl]
   ld l, a
   rst _CallHL

   call _CheckForStackUnderflow

   pop af
   pop bc
   pop de
   pop hl

   reti

ENDSECTION


;###############################################################################
;
;     WAIT FUNCTIONS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Turn LCD on, with parameters
;
; Inputs:
; - a = `rLCDC` parameters
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
   call _WaitForScanlineHigh  ; Wait for LY 144 (VBlank start)

   xor a, a
   ldh [rLCDC], a             ; Shutdown LCD
   
   ret

; Wait for b >= LY (Scanline)
;
; Inputs:
; - b = LY to wait for
;
; Trashes:
; - c
_WaitForScanlineHigh::
   ld c, LOW(rLY)
.wait:
   ld a, [c]
   cp a, b
   jr c, .wait
   ret

; Wait for b < LY (Scanline)
;
; Inputs:
; - b = LY to wait for
;
; Trashes:
; - c
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

ENDSECTION


;###############################################################################
;
;     SCAN KEYS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Polls joypad register and sets appropriate HRAM variables
;
; Returns:
; - `hKeysHeld`
; - `hKeysPressed`
; - `hKeysReleased`
;
; Trashes:
; - af
; - bc
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
   and a, PADF_START | PADF_SELECT | PADF_B | PADF_A
   jr nz, .tryReset

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


;###############################################################################
;
;     HARDWARE CHECKS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Checks if running on GBC hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBColor::
   ldh a, [hIsGBColorFlag]
   and a
   ret

; Checks if running on GBA hardware
;
; Returns:
; - nz = TRUE
; - z = FALSE
_IsGBAdvance::
   ldh a, [hIsGBAdvanceFlag]
   and a
   ret

ENDSECTION


;###############################################################################
;
;     EMPTY SPACE
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

:  DS $0100 - @, $FF

ENDSECTION


;###############################################################################
;
;     CARTRIDGE HEADER
;
;###############################################################################

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

DS $014C - @, $00

_MagicStringVersion:
   DS 1, $00
.end

DS $0150 - @, $00

   DB "BUILD DATE "
_BuildDate::
   DB __ISO_8601_UTC__
.end::

DS $0170 - @, $00

ENDSECTION


;###############################################################################
;
;     STACK POINTER CHECKS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Verifies that `sp` is within the bounds of the stack and
; causes a crash on stack detection of overflow.
;
; By default, called by 
; the interrupt handler executor, `_FarCall`, and `_FarCallPreserve` 
;
; Trashes:
; - af
_CheckForStackOverflow::
   push hl

   ld hl, sp - STACK_BUFFER  ; account for full push/pops and/or a couple of further calls during
	ld a, h
	cp a, HIGH(wStackBottom)
	ld a, CRASH_STACK_OVERFLOW
	jp c, _CrashHandler

   pop hl
   ret

; Verifies that `sp` is within the bounds of the stack and
; causes a crash on stack detection of underflow.
;
; By default, called by 
; the interrupt handler executor, `_FarCall`, and `_FarCallPreserve` 
;
; Trashes:
; - af
_CheckForStackUnderflow::
IF (STACK_SIZE % $100 == 0)
   push hl

   ld hl, sp + STACK_BUFFER  ; account for full push/pops and/or a couple of further calls during
	ld a, h
   cp a, HIGH(wStackTop) + 1
	ld a, CRASH_STACK_UNDERFLOW
	jp nc, _CrashHandler

   pop hl
ELSE
   WARN "STACK_SIZE must be a multiple of $100 bytes for effecient stack underflow checking"
ENDC
   ret

ENDSECTION


;###############################################################################
;
;     CPU SPEED SWITCHES
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Set GBC/GBA CPU Speed to DoubleSpeed
_SetCPUFast::
   call _IsGBColor
   ret z
   ldh a, [rKEY1]
   and a, KEY1F_DBLSPEED
   jr nz, _SwitchCPUSpeed
   ret

; Set GBA/GBC CPU to SingleSpeed
_SetCPUSlow::
   call _IsGBColor
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


;###############################################################################
;
;     FARCALLS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Calls function `hl` in bank `a` by switching rom banks
; - a = bank where function is located
; - hl = function
;
; Arugments can be passed via `bc` and `de`
; - bc = args
; - de = args
_FarCall::
   ldh [hFarCallBank], a   ; Save destination bank

   ldh a, [hCurrentBank]   ; Get current bank
   push af                 ; Push current bank to stack

   ldh a, [hFarCallBank]   ; Set destination bank
   ldh [hCurrentBank], a
   ld [rROMB0], a

   call _CheckForStackOverflow

   rst _CallHL             ; Call function at `hl` in destination bank

   call _CheckForStackOverflow

   pop af                  ; Pop current bank from stack and reset
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


;###############################################################################
;
;     CHECKSUM
;
;###############################################################################

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


;###############################################################################
;
;     EMPTY SPACE
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

:  DS $0200 - @, $FF

ENDSECTION


;###############################################################################
;
;     START, RESET, & CRASH HANDLER
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

_Start:
   cp a, BOOTUP_A_CGB      ; Check Hardware model (GBC)
   ld a, FALSE
   jr nz, .notGBC
   ld a, TRUE
   ldh [hIsGBColorFlag], a

   ld a, b
   cp a, BOOTUP_B_AGB      ; Check Hardware model (GBA)
   ld a, FALSE
   jr nz, .notGBA
   ld a, TRUE
   ; fallthrough

.notGBC:
   ldh [hIsGBColorFlag], a
.notGBA:
   ldh [hIsGBAdvanceFlag], a
   ; fallthrough

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

   call _IsGBColor
   jr z, .notGBC

   ld a, 1
   ld hl, _VRAM
   ld bc, VRAM_SIZE
   ld d, $00
   call _VideoMemSet       ; Clear VRAM bank 1

   call _SetBackgroundPaletteBlack
   call _SetSpritePaletteBlack

.notGBC:
   ld hl, _HRAM + (hRandomState.end - _HRAM)
   ld b, HRAM_SIZE - (hRandomState.end - _HRAM)
   ld a, $00
   call _MemSetFast        ; Clear HRAM (but not the variables we've already set)

   call _InitOAM           ; With HRAM filled with $00, copy OAM Refresh function to HRAM
   call _RefreshOAM        ; With RAM filled with $00, this will clear also clear OAM

ASSERT BANK(_Main) == 0, #"""
Function `_Main` should be in ROM0!
gbc-engine-core hands off code execution to a function labeled `_Main` when done with initial setup.

The function can assume the following:
 - All ram areas are cleared
 - LCD is off
 - Interrupts are disabled
"""
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

   call _ScreenOff         ; Turn off screen

   xor a
   ld hl, _VRAM
   ld bc, VRAM_SIZE
   ld d, $00
   call _VideoMemSet       ; Clear VRAM bank 0

ASSERT BANK(_FontTiles) == 0, #"""
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
ASSERT ((_FontTiles.end - _FontTiles) * 2) == (vBLK01.128 - vBLK01.32), #"""
gbc-engine-core expects `_FontTiles` to have `.end`, to be 96 tiles long, and to be formated as .1bpp
"""
   xor a
   ld hl, _FontTiles
   ld bc, (_FontTiles.end - _FontTiles)
   ld de, vBLK01.32
   call _VideoMemCopyMonochrome

   xor a
   ld hl, _CrashTextTileMap
   ld bc, (_CrashTextTileMap.end - _CrashTextTileMap)
   ld de, vSCRN1
   call _VideoMemCopy

   xor a
   ld hl, _MagicStringTitle
   ld b, (_MagicStringCode.end - _MagicStringTitle)
   ld de, vSCRN1.y1x2
   call _VideoMemCopyFast

   ld hl, _MagicStringVersion
   ld a, [hl]
   add "0"
   ld hl, vSCRN1.y1x17
   ld b, 1
   xor a
   call _VideoMemSetFast

   call _GetCrashCode
   xor a
   ld de, vSCRN1.y11x13
   call _VideoMemCopyFast

   call _GetCrashText
   ld de, vSCRN1.y13x1
   xor a
   call _VideoMemCopyFast

   xor a
   ld hl, _BuildDate + 2
   ld b, (_BuildDate.end - (_BuildDate + 2))
   ld de, vSCRN1.y16x1
   call _VideoMemCopyFast

   call _IsGBColor
   jr z, .notGBC

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

.notGBC:
   ; Reset scroll registers
   xor a
   ldh [rSCY], a
   ldh [rSCX], a

   ; Turn on LCD
   ld a, LCDCF_BLK01 | LCDCF_BG9C00 | LCDCF_BGON
   call _ScreenOn
      
   ; During the first (blank) frame, initialize display registers
   ld a, %11100100
   call _SetDMGPalettes

.loop
   halt
   nop
   jr .loop
   ret

_GetCrashCode:
   ldh a, [hCrashCode]
   ld de, _CrashCodes
   ld h, $00
   ld l, a
REPT 2         ; 2^2 = 4
   add hl, hl ; index * 4
ENDR
   add hl, de
   ld b, CRASH_CODE_SIZE
   ret

_CrashCodes:
   DB "{04X:CRASH_EXECUTING_FF}"
   DB "{04X:CRASH_NULL_POINTER}"
   DB "{04X:CRASH_EXECUTING_RAM}"
   DB "{04X:CRASH_RETURN_FROM_MAIN}"
   DB "{04X:CRASH_STACK_OVERFLOW}"
   DB "{04X:CRASH_STACK_UNDERFLOW}"
   DB "{04X:CRASH_DIVIDE_BY_ZERO}"
   DB "00FF"
.end:
   ASSERT (_CrashCodes.end - _CrashCodes) == ((NUMBER_OF_CRASH_CODES + 1) * CRASH_CODE_SIZE)

_GetCrashText:
   ldh a, [hCrashCode]
   ld de, _CrashText
   ld h, $00
   ld l, a
REPT 4         ; 2^4 = 16
   add hl, hl ; index * 16
ENDR
   add hl, de
   ld b, CRASH_TEXT_SIZE
   ret

_CrashText:
; col# 0             15
;      |..............|
   DB "Executing $FF   "
   DB "NULL pointer    "
   DB "Executing RAM   "
   DB "Return from Main"
   DB "Stack overflow  "
   DB "Stack underflow "
   DB "Divide by zero  "
   DB "Unknown error   "
.end:
   ASSERT (_CrashText.end - _CrashText) == ((NUMBER_OF_CRASH_CODES + 1) * CRASH_TEXT_SIZE)

_CrashTextTileMap:
; col# 0                 19          31
;      |..................|...........|  ; row#
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
REPT 14                                  ; .
   DB "................................" ; .
ENDR                                     ; — 31
.end:

_CrashPalette:
   DW (00 << 0) + (00 << 5) + (15 << 10)
   DW (00 << 0) + (00 << 5) + (15 << 10)
   DW (00 << 0) + (00 << 5) + (15 << 10)
   DW (31 << 0) + (31 << 5) + (31 << 10)
.end:

ENDSECTION


;###############################################################################
;
;     SRAM HANDLERS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Load data from SRAM to address `de`
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_LoadFromSRAM::
   call _EnableSRAM

   push hl
   push de
   push bc
   call _VerifySRAMIntegrity
   pop bc
   pop de
   pop hl

   call _MemCopy

   call _DisableSRAM
   ret

; Save data to SRAM to address `hl`
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_SaveToSRAM::
   call _EnableSRAM

   call _MemCopy
   call _SaveSRAMIntegrityData

   call _DisableSRAM
   ret

_EnableSRAM:
   ld a, CART_SRAM_ENABLE
   ld [rRAMG], a
   ret

_DisableSRAM:
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


;###############################################################################
;
;     MULTIPLY & DIVIDE
;
;###############################################################################

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
:  add hl, hl     ; 2   = 5/6 cycles
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


;###############################################################################
;
;     RANDOM NUMBER GENERATOR
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

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
   ld c, a
   adc a, [hl]
   ld [hl], a
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
   jp _GetRandom

ENDSECTION


;###############################################################################
;
;     MEM SET
;
;###############################################################################

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
   call _WaitForScreenBlank
   ld a, d
   ld [hl+], a
   dec bc
   ld a, b
   or a, c
   jr nz, .loop
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
   call _WaitForScreenBlank
   ld a, d
   ld [hl+], a
   dec b
   jr nz, .loop
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

; Safely randomizes an area of VRAM memory.
;
; Inputs:
; - a = VRAM bank
; - hl = start address
; - bc = byte count
_VideoMemSetRandom::
   ldh [rVBK], a
.loop:
   call _WaitForScreenBlank
   call _GetRandomByte
   ld [hl+], a
   dec bc
   ld a, b
   or a, c
   jr nz, .loop
   ret

ENDSECTION


;###############################################################################
;
;     MEM COPY
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

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

; Safely copies bytes from one area of memory to VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - bc = byte count
; - de = dest address
_VideoMemCopy::
   ldh [rVBK], a
.loop:
   call _WaitForScreenBlank
   ld a, [hl+]
   ld [de], a
   inc de
   dec bc
   ld a, b
   or a, c
   jr nz, .loop
   ret

; Copies bytes from one area of memory to another.
;
; Used to copy .1bpp formated tile data
;
; Inputs:
; - hl = source address
; - bc = byte count
; - de = dest address
_MemCopyMonochrome::
   ld a, [hl+]
   ld [de], a
   inc de
   ld [de], a
   inc de
   dec bc
   ld a, b
   or a, c
   jr nz, _MemCopyMonochrome
   ret

; Safely copies bytes from one area of memory to VRAM.
;
; Used to copy .1bpp formated tile data
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - bc = byte count
; - de = dest address
_VideoMemCopyMonochrome::
   ldh [rVBK], a
.loop:
   call _WaitForScreenBlank
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

; Safely copies bytes from one small area (< 256 bytes) of memory to VRAM.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - de = dest address
_VideoMemCopyFast::
   ldh [rVBK], a
.loop:
   call _WaitForScreenBlank
   ld a, [hl+]
   ld [de], a
   inc de
   dec b
   jr nz, .loop
   ret

; Copies bytes from one area of memory to another while incrementally stepping.
;
; Inputs:
; - hl = source address
; - b = bytes
; - c = source increment
; - de = dest address
_MemCopyInc::
   ld a, [hl]
   ld [de], a
   inc de      ; increase dest
   ld a, b     ; save b
   ld b, $00
   add hl, bc  ; increase source
   ld b, a     ; restore b
   dec b
   jr nz, _MemCopyInc
   ret

; Safely copies bytes from one area of memory to VRAM while incrementally stepping.
;
; Inputs:
; - a = VRAM bank
; - hl = source address
; - b = bytes
; - c = source increment
; - de = dest address
_VideoMemCopyInc::
   ldh [rVBK], a
.loop:
   call _WaitForScreenBlank
   ld a, [hl]
   ld [de], a
   inc de      ; increase dest
   ld a, b     ; save b
   ld b, $00
   add hl, bc  ; increase source
   ld b, a     ; restore b
   dec b
   jr nz, .loop
   ret

ENDSECTION


;###############################################################################
;
;     MEM COMPARE
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

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

; Safely compares bytes from one area of memory to VRAM.
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
   call _WaitForScreenBlank
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


;###############################################################################
;
;     BACKGROUND FUNCTIONS
;
;###############################################################################

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
   jr _GetTileAddress

_GetTileAddress:
   ld h, $00
   ld l, a
REPT 4         ; 2^4 = 16
   add hl, hl ; index * 16
ENDR
   add hl, de
   ret

; Set `vSCRN0` tile # to `a` at xy `bc`
;
; Input:
; - a = tile #
; - b = x
; - c = y
;
; Returns:
; - hl = source address ( `vSCRN0.y#x#` )
; - de = base address ( `vSCRN0` )
_SetSCRN0BackgroundTile::
   ld de, vSCRN0
   jr _SetBackgroundTile

; Set `vSCRN1` tile # to `a` at xy `bc`
;
; Input:
; - a = tile #
; - b = x
; - c = y
;
; Returns:
; - hl = source address ( `vSCRN1.y#x#` )
; - de = base address ( `vSCRN1` )
_SetSCRN1BackgroundTile::
   ld de, vSCRN1
   jr _SetBackgroundTile

_SetBackgroundTile:
   ld e, b     ; de = base + x
   ld h, $00
   ld l, c     ; hl = y
REPT 5         ; 2^5 = 32
   add hl, hl  ; y * 32
ENDR
   add hl, de  ; hl = (base + x) + (y * 32)
   ld [hl], a
   ret

ENDSECTION


;###############################################################################
;
;     SPRITE FUNCTIONS
;
;###############################################################################

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


;###############################################################################
;
;     PALETTE FUNCTIONS
;
;###############################################################################

SECTION FRAGMENT "GBC-ENGINE-CORE", ROM0

; Sets DMG Palettes
;
; - a = `rBGP` palette
; - a = `rOBP0` palette
; - a ~ `rOBP1` palette
_SetDMGPalettes::
	ldh [rBGP], a
   ldh [rOBP0], a
   cpl
   ldh [rOBP1], a
   ret

; Sets all Background Palettes to full white
_SetBackgroundPaletteWhite::
   ld hl, _WhiteoutPalettes
   jr _SetBackgroundPaletteAll

; Sets all Background Palettes to full black
_SetBackgroundPaletteBlack::
   ld hl,_BlackoutPalettes
   ;fallthrough

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
   ;fallthrough

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


;###############################################################################
;
;     OAM REFRESH
;
;###############################################################################

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
   ldh [rDMA], a  ; start DMA transfer (starts right after instruction)
   ld a, 40       ; delay for a total of 4×40 = 160 M-cycles
.delay:
   dec a          ; 1 M-cycle
   jr nz, .delay  ; 3 M-cycles
   ret z          ; Conditional `ret` is 1 M-cycle slower, which avoids
                  ; reading from the stack on the last M-cycle of DMA.
ENDL
.end:

ENDSECTION


;###############################################################################
;
;     VRAM - TILE BLOCKS
;
;###############################################################################

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


;###############################################################################
;
;     VRAM - SCRN0
;
;###############################################################################

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


;###############################################################################
;
;     VRAM - SCRN1
;
;###############################################################################

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


;###############################################################################
;
;     SRAM
;
;###############################################################################

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


;###############################################################################
;
;     WRAM
;
;###############################################################################

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


;###############################################################################
;
;     OAM RAM
;
;###############################################################################

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


;###############################################################################
;
;     HRAM
;
;###############################################################################

SECTION "GBC-ENGINE-CORE - HRAM", HRAM[_HRAM]

hIsGBColorFlag:   DS 1
hIsGBAdvanceFlag: DS 1

hRandomState:
   DS 4
.end:

hCrashCode::      DS 1

hIsVBlankFlag:    DS 1

hVBLHandler:      DS 2
hLCDHandler:      DS 2
hTIMHandler:      DS 2
hSIOHandler:      DS 2
hJOYHandler:      DS 2

hCanResetFlag::   DS 1

hKeysHeld::       DS 1
hKeysPressed::    DS 1
hKeysReleased::   DS 1

hCurrentBank:     DS 1
hFarCallBank:     DS 1

ENDSECTION


;###############################################################################
;
;     PURGE MACROS
;
;###############################################################################

PURGE create_gb_engine_core_charmap


;###############################################################################
;
;     PURGE VARIABLES
;
;###############################################################################

PURGE REQUIRED_RGBDS_VERSION, REQUIRED_HARDWARE_VERSION, \
      CRASH_CODE_SIZE, CRASH_TEXT_SIZE, \
      VRAM_SIZE, TILE_BLOCK_SIZE, TILE_SIZE, OFFSET, \
      SRAM_SIZE, SRAM_BUFFER_SIZE, \
      RAM_SIZE, STACK_SIZE, STACK_BUFFER, \
      HRAM_SIZE, hTempStack


;###############################################################################
;
;     POP LINKER STACKS
;
;###############################################################################

POPC ; charmap
POPS ; section
POPO ; options
