; UPDATE THIS LINE WHEN CHANGING THE REVISION NUMBER
DEF GBC_ENGINE_CORE_VERSION EQUS "0.2.0"

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
   DEF CUR_GBC_ENGINE_CORE_VER EQUS STRRPL("{GBC_ENGINE_CORE_VERSION}", ".", ",")
   DEF MIN_GBC_ENGINE_CORE_VER EQUS STRRPL("\1", ".", ",")
   
   check_core_version {CUR_GBC_ENGINE_CORE_VER}, {MIN_GBC_ENGINE_CORE_VER}, 0, 0, 0

   IF !DEF(MIN_GBC_ENGINE_CORE_VER)
      FAIL "gbc-engine-core version \1 is required, which is incompatible with current version {GBC_ENGINE_CORE_VERSION}"
   ENDC

   PURGE CUR_GBC_ENGINE_CORE_VER, MIN_GBC_ENGINE_CORE_VER, check_core_version
ENDM

; Used internally by `gbc_engine_core_version` to check whether trios of version components match.
;
; Usage:
; - `check_core_version` `expected major, minor, patch`, `current major, minor, patch`
MACRO check_core_version
   IF (\1) != (\4) || (\2) > (\5) || (\3) > (\6)
      PURGE MIN_GBC_ENGINE_CORE_VER
   ENDC
ENDM

IF (!DEF(__RGBDS_MAJOR__) || !DEF(__RGBDS_MINOR__) || !DEF(__RGBDS_PATCH__)) || \
   (__RGBDS_MAJOR__ <  0) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ <  7) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ == 7 && __RGBDS_PATCH__ <  0) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ == 7 && __RGBDS_PATCH__ == 0 && DEF(__RGBDS_RC__))
	FAIL "This version of gbc-engine-core.inc requires rgbds v0.7.0 or newer."
ENDC

; check to ensure hardware.inc has been included and defined
   rev_Check_hardware_inc 4.9.1

DEF TRUE    EQU 1
DEF FALSE   EQU 0

DEF VRAM_SIZE        EQU _SRAM - _VRAM
DEF TILE_BLOCK_SIZE  EQU (VRAM_SIZE - (_SRAM - _SCRN0)) / 3
DEF TILE_SIZE        EQU $10
DEF OFFSET           EQU 128

DEF SRAM_SIZE        EQU _RAM - _SRAM
DEF SRAM_BUFFER_SIZE EQU $100

DEF RAM_SIZE         EQU (_RAMBANK - _RAM) * 2
DEF RAM_STACK_SIZE   EQU $200

DEF HRAM_SIZE        EQU rIE - _HRAM
DEF hTempStack       EQU rIE - 1


SECTION "GBC-ENGINE-CORE", ROM0[$0000]

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
	jp _CrashHandler

DS $0008 - @, $FF

; RST Vector for `jp hl` with additional error handling
;
; Crashes if attempting to jump to RAM
; Used by `_FarCall`, and `_FarCallPreserve`
_CallHL::
   bit 7, h
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

; RST Vector to perform a soft reset
;
; This traps arbitrary code executions of the byte $FF,
; which is the recommended "default" padding value for empty ROM space.
; The opcode for rst $38 is $FF, so any attempts to execute code in
; such empty space will instead end up here.
_Crash::
   nop
   nop
   jp _CrashHandler

DS $0040 - @, $FF

_VBL:
   push hl
   push af
   ld hl, hVBLHandler
   jr _InterruptVBL

DS $0048 - @, $FF

_LCD:
   push hl
   push af
   ld hl, hLCDHandler
   jr _InterruptCommon

DS $0050 - @, $FF

_TIM:
   push hl
   push af
   ld hl, hTIMHandler
   jr _InterruptCommon

DS $0058 - @, $FF

_SIO:
   push hl
   push af
   ld hl, hSIOHandler
   jr _InterruptCommon

DS $0060 - @, $FF

_JOY:
   push hl
   push af
   ld hl, hJOYHandler
   jr _InterruptCommon

DS $0068 - @, $FF

; Default Interrupt Handler Function
;
; Initialize Interrupt Handler HRAM variables to this in _Start,
; and any other time in which you don't want an interrupt to do anything.
_DefaultInterruptHandler::
   ret

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
; Called from VBL Interrupt $0040
;
; Inputs:
;- hl = interrupt handler HRAM variable
_InterruptVBL:
   ld a, TRUE
   ldh [hIsVBlankFlag], a
   ; fallthrough

; Execute Interrupt Handler
;
; Inputs:
; - hl = interrupt handler HRAM variable
_InterruptCommon:
   ld a, [hl+]
   ld h, [hl]
   ld l, a

   push bc
   push de
   rst _CallHL
   pop  de
   pop  bc

   pop af
   pop hl
   reti

; Safely turns LCD off by waiting for VBlank
_ScreenOff::
   ldh a, [rLCDC]
   and a, LCDCF_ON
   ret z ; LCD already OFF
   ld b, SCRN_Y
   call _WaitForLY
   xor a, a
   ldh [rLCDC], a ; Shutdown LCD
   ret

; Wait for LY (Scanline)
; - b = LY to wait for
_WaitForLY::
   ld c, LOW(rLY)
.wait:
   ld a, [c]
   cp a, b
   jr c, .wait
   ret

; Wait for HBlank and/or VBlank mode
; (only modes where VRAM access is safe)
_WaitForScreenBlank::
   ldh a, [rSTAT]
   bit STATB_BUSY, a
   jr nz, _WaitForScreenBlank ; Not mode 0 or 1
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

; Wait for (e) VBlank Interrupts
; - e = frames to wait
_WaitFrames::
   call _WaitForVBLInterrupt
   dec e
   jr nz, _WaitFrames
   ret

; Polls joypad register and sets appropriate HRAM variables
;
; Returns:
; - a = hKeysHeld
;
; Trashes:
; - bc
_ScanKeys::
   ld c, LOW(rP1)          ; c = joypad register low byte
   ld a, P1F_GET_DPAD      ; get DPAD
   ld [c], a
REPT 6
   ld a, [c]               ; poll for a few cycles
ENDR
   or a, %11110000         ; set upper 4 bits
   swap a                  ; swap DPAD bits to upper nibble
   ld b, a                 ; b = current state (DPAD)

   ld a, P1F_GET_BTN       ; get BTN
   ld [c], a
REPT 6
   ld a, [c]               ; poll for a few cycles
ENDR
   or a, %11110000         ; set upper 4 bits
   xor a, b                ; Mix with DPAD bits, and invert all bits (such that pressed=1)
   ld b, a                 ; b = current state (DPAD + BTN)

   ld a, P1F_GET_NONE      ; reset joypad register
   ld [c], a

   ld a, b                 ; check if reset key combo is pressed
   and a, PADF_START | PADF_SELECT | PADF_B | PADF_A
   jr nz, .tryReset

.noReset:
   ldh a, [hKeysHeld]
   ld c, a                 ; c = old state

   ld a, b                 ; a = current state
   cpl                     ; a = not a
   and a, c                ; released = (NOT current) AND old
   ldh [hKeysReleased], a  

   ld a, c                 ; a = old state
   cpl                     ; a = not a
   and a, b                ; pressed = (NOT old) AND current
   ldh [hKeysPressed], a

   ld a, b                 ; a = current state
   ldh [hKeysHeld], a

   ret

.tryReset:
   ldh a, [hCanResetFlag]  ; only reset if TRUE
   and a
   jr z, .noReset          ; else skip
   jp _Reset

_____:
DS $0100 - @, $FF

_Header:
   nop
   jp _Start

   NINTENDO_LOGO

_MagicString:
   DS 16, $00
.end:

DS $0150 - @, $00

_Start:
   cp a, BOOTUP_A_CGB      ; Check Hardware model (GBC)
   jr nz, .notGBC
   ld a, TRUE
   ldh [hIsGBColorFlag], a

   ld a, b
   cp a, BOOTUP_B_AGB      ; Check Hardware model (GBA)
   jr nz, .notGBA
   ld a, TRUE
   ldh [hIsGBAdvanceFlag], a

.notGBC:
   xor a
   ldh [hIsGBColorFlag], a
.notGBA:
   xor a
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

   ld hl, _RAM             ; Clear RAM
   ld bc, RAM_SIZE
   ld d, $00
   call _MemSet

   ld sp, wStackTop        ; With RAM cleared, switch to real stack

   ld hl, _VRAM            ; Clear VRAM
   ld bc, VRAM_SIZE
   ld d, $00
   call _MemSet

   ld hl, _HRAM + 2        ; Clear HRAM (but not hardware model variables)
   ld b, HRAM_SIZE - 2
   ld a, $00
   call _MemSetFast

   call _InitOAM           ; With HRAM filled with $00, copy OAM Refresh function to HRAM
   call _RefreshOAM        ; With RAM filled with $00, this will clear also clear OAM

ASSERT BANK(_Main) == 0, """
   Function `_Main` should be in ROM0!
   gbc-engine-core hands off code execution to a function labeled `_Main` when done with initial setup.
   """
   call _Main              ; Real program starts here

ASSERT BANK(_CrashHandler) == 0, """
   Function `_CrashHandler` should be in ROM0!
   gbc-engine-core hands off code execution to a function labeled `_CrashHandler` whenever `rst _Crash` is executed.
   """
   jp _CrashHandler        ; Should never reach this point!

; Set GBC/GBA CPU Speed to DoubleSpeed
_SetCPUFast::
   ldh a, [hIsGBColorFlag]
   cp a, TRUE
   ret c
   ldh a, [rKEY1]
   and a, KEY1F_DBLSPEED
   jr nz, _SwitchCPUSpeed
   ret

; Set GBA/GBC CPU to SingleSpeed
_SetCPUSlow::
   ldh a, [hIsGBColorFlag]
   cp a, TRUE
   ret c
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

; Calls function `hl` in bank `a` by switching rom banks
; - a = bank where function is located
; - hl = function
;
; Arugments can be passed via `bc` and `de`
; - bc = args
; - de = args
_FarCall::
   ldh [hFarCallBank], a   ; Save destination bank

   ld a, [rROMB0]          ; Push current bank
   push af

   ldh a, [hFarCallBank]   ; Set bank
   ld [rROMB0], a

   rst _CallHL             ; Call function at `hl`

   pop af                  ; Pop bank and reset
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

; Load data from SRAM to address `de`
; Inputs:
; - hl = source address
; - bc = size
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
; - bc = size
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
   ld hl, _MagicString
   ld b, (_MagicString.end - _MagicString)
   ld de, sMagicString
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
   ld hl, _MagicString
   ld b, (_MagicString.end - _MagicString)
   ld de, sMagicString
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
_ChecksumString: ; `SAVE CHECKSUM:`
   DB 83, 65, 86, 69, 32, 67, 72, 69, 67, 75, 83, 85, 77, 58
.end:

; ASCII string saved to SRAM to mark checksum
_DataString: ; `DATA START:`
   DB 68, 65, 84, 65, 32, 83, 84, 65, 82, 84, 58
.end:

; Calculates a 2 byte checksum
;
; Inputs:
; - hl = start address
; - bc = size
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
   ld a, $FF
   ld c, $00
   scf ; set carry
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

; Generates a pseudorandom 32-bit integer in BCDE
; using the LCG formula from cc65 rand():
;
; x[i + 1] = (x[i] + 0xB3) * 0x01010101
;
; Returns:
; - A = state bits 31-24 (which have the best entropy)
; - B = state bits 31-24
; - C = state bits 23-16
; - D = state bits 15-08
; - E = state bits 07-00 (which have the worst entropy)
;
; Trashes:
; - HL
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
; - A = state bits 31-24 (which have the best entropy)
;
; Preserves:
; - BC
; - DE
; - HL
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
; - DE = seed
;
; Returns:
; - A = state bits 31-24 (which have the best entropy)
; - B = state bits 31-24
; - C = state bits 23-16
; - D = state bits 15-08
; - E = state bits 07-00 (which have the worst entropy)
;
; Trashes:
; - HL
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

; Sets an area of memory to a specified value.
;
; Inputs:
; - hl = start address
; - bc = size
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
; - hl = start address
; - bc = size
; - d = value
_VideoMemSet::
   call _WaitForScreenBlank
   ld a, d
   ld [hl+], a
   dec bc
   ld a, b
   or a, c
   jr nz, _VideoMemSet
   ret

; Sets a small area (< 256 bytes) of memory to a specified value.
;
; Inputs:
; - hl = start address
; - b = size
; - a = value
_MemSetFast::
   ld [hl+], a
   dec b
   jr nz, _MemSetFast
   ret

; Safely sets a small area (< 256 bytes) of VRAM memory to a specified value.
;
; Inputs:
; - hl = start address
; - b = size
; - a = value
_VideoMemSetFast::
   push af
   call _WaitForScreenBlank
   pop af
   ld [hl+], a
   dec b
   jr nz, _VideoMemSetFast
   ret

; Randomizes an area of memory.
;
; Inputs:
; - hl = start address
; - bc = size
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
; - hl = start address
; - bc = size
_VideoMemSetRandom::
   call _WaitForScreenBlank
   call _GetRandomByte
   ld [hl+], a
   dec bc
   ld a, b
   or a, c
   jr nz, _VideoMemSetRandom
   ret

; Copies bytes from one area of memory to another.
;
; Inputs:
; - hl = source address
; - bc = size
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
; - hl = source address
; - bc = size
; - de = dest address
_VideoMemCopy::
   call _WaitForScreenBlank
   ld a, [hl+]
   ld [de], a
   inc de
   dec bc
   ld a, b
   or a, c
   jr nz, _VideoMemCopy
   ret

; Copies bytes from one small area (< 256 bytes) of memory to another.
;
; Inputs:
; - hl = source address
; - b = size
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
; - hl = source address
; - b = size
; - de = dest address
_VideoMemCopyFast::
   call _WaitForScreenBlank
   ld a, [hl+]
   ld [de], a
   inc de
   dec b
   jr nz, _VideoMemCopyFast
   ret

; Copies bytes from one area of memory to another while incrementally stepping.
;
; Inputs:
; - hl = source address
; - b = size
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
; - hl = source address
; - b = size
; - c = source increment
; - de = dest address
_VideoMemCopyInc::
   call _WaitForScreenBlank
   ld a, [hl]
   ld [de], a
   inc de      ; increase dest
   ld a, b     ; save b
   ld b, $00
   add hl, bc  ; increase source
   ld b, a     ; restore b
   dec b
   jr nz, _VideoMemCopyInc
   ret

; Compares bytes from one area of memory to another.
;
; Inputs:
; - hl = source address
; - b = size
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
; - hl = source address
; - b = size
; - de = dest address
;
; Returns:
; - z = equal
; - nz = not equal
_VideoMemCompare::
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

;- b = x
;- c = y
;- hl = `wShadowOAM.#`
_SetSpriteXY::
   ld [hl], c
   inc hl
   ld [hl], b
   ret

;- a = tile
;- hl = `wShadowOAM.#`
_SetSpriteTile::
   inc hl
   inc hl
   ld [hl], a
   ret

;- a = params
;- hl = `wShadowOAM.#`
_SetSpriteParams::
   inc hl
   inc hl
   inc hl
   ld [hl], a
   ret

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


SECTION FRAGMENT "GBC-ENGINE-CORE - VRAM", VRAM[_SCRN0]

; VRAM Screen 0 Tilemap
;
; Write to a specific tile address using `vSCRN0.y#x#`,
; where both y# and x# are `0 <= # <= 31`
vSCRN0::
.start::
FOR COL, SCRN_VY_B
FOR ROW, SCRN_VX_B
.y{d:COL}x{d:ROW}::
   DS 1
ENDR
ENDR
.end::

ASSERT @ == _SCRN1

ENDSECTION


SECTION FRAGMENT "GBC-ENGINE-CORE - VRAM", VRAM[_SCRN1]

; VRAM Screen 1 Tilemap
;
; Write to a specific tile address using `vSCRN1.y#x#`,
; where both y# and x# are `0 <= # <= 31`
vSCRN1::
.start::
FOR COL, SCRN_VY_B
FOR ROW, SCRN_VX_B
.y{d:COL}x{d:ROW}::
   DS 1
ENDR
ENDR
.end::

ASSERT @ == _SRAM

ENDSECTION


SECTION "GBC-ENGINE-CORE - SRAM", SRAM[_SRAM]

; 256 byte buffer at the start of SRAM
sBuffer:
   DS $100

; Copy of the Magic String used to verify save integrity
sMagicString:
   DS (_MagicString.end - _MagicString)

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


SECTION FRAGMENT "GBC-ENGINE-CORE - WRAM", WRAM0[_RAM]

wStack:     DS RAM_STACK_SIZE
wStackTop:

ENDSECTION


SECTION FRAGMENT "GBC-ENGINE-CORE - WRAM", WRAM0, ALIGN[8]

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

ENDSECTION


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

ENDSECTION


SECTION "GBC-ENGINE-CORE - HRAM", HRAM[_HRAM]

hIsGBColorFlag::     DS 1
hIsGBAdvanceFlag::   DS 1


hIsVBlankFlag: DS 1

hVBLHandler:   DS 2
hLCDHandler:   DS 2
hTIMHandler:   DS 2
hSIOHandler:   DS 2
hJOYHandler:   DS 2


hCanResetFlag::   DS 1

hKeysHeld::       DS 1
hKeysPressed::    DS 1
hKeysReleased::   DS 1


hFarCallBank:     DS 1

hRandomState:     DS 4

ENDSECTION


PURGE VRAM_SIZE        
PURGE TILE_BLOCK_SIZE  
PURGE TILE_SIZE
PURGE OFFSET   

PURGE SRAM_SIZE        
PURGE SRAM_BUFFER_SIZE 

PURGE RAM_SIZE         
PURGE RAM_STACK_SIZE   

PURGE HRAM_SIZE 
PURGE hTempStack
