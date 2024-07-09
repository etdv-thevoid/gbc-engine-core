IF !DEF(__RGBDS_MAJOR__) || !DEF(__RGBDS_MINOR__) || !DEF(__RGBDS_PATCH__)
	FAIL "This version of gb-engine-core requires rgbds v0.7.0 or newer."
ENDC

IF (__RGBDS_MAJOR__ <  0) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ <  7) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ == 7 && __RGBDS_PATCH__ <  0) || \
   (__RGBDS_MAJOR__ == 0 && __RGBDS_MINOR__ == 7 && __RGBDS_PATCH__ == 0 && DEF(__RGBDS_RC__))
	FAIL "This version of gb-engine-core requires rgbds v0.7.0 or newer."
ENDC


DEF TRUE    EQU 1
DEF FALSE   EQU 0

DEF STACK_SIZE      EQU $200
DEF HRAM_STACK_SIZE EQU $20


SECTION "Game Boy Engine Core", ROM0[$0000]

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
; Used by `_InterruptCommon`, `_LongCall`, and `_LongCallArgs`
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

DS $0070 - @, $FF

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

DS $0078 - @, $FF

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

DS $0088 - @, $FF

_ScreenOff::
    ldh a, [rLCDC]
    and a, LCDCF_ON
    ret z ; LCD already OFF
    ld b, 144
    call _WaitLY
    xor a, a
    ldh [rLCDC], a ; Shutdown LCD
    ret

DS $0098 - @, $FF

; - b = ly to wait for
_WaitLY::
    ld c, LOW(rLY)
.loop:
    ld a, [c]
    cp a, b
    jr c, .loop
    ret

DS $00A0 - @, $FF

; Wait for next VBlank Period
_WaitVBL::
    ld a, FALSE
    ldh [hIsVBlankFlag], a
.loop:
    halt
    nop
    ldh a, [hIsVBlankFlag]
    and a
    jr z, .loop
    ret

DS $00B0 - @, $FF

_WaitScreenBlank::
    ldh a, [rSTAT]
    bit STATB_BUSY, a
    jr nz, _WaitScreenBlank ; Not mode 0 or 1
    ret

DS $00B8 - @, $FF

; - e = frames to wait
_WaitFrames::
    call _WaitVBL
    dec e
    jr nz, _WaitFrames
    ret

DS $00C0 - @, $FF

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
    ld a, [c]              ; poll for a few cycles
ENDR
    or a, %11110000         ; set upper 4 bits
    swap a                  ; swap DPAD bits to upper nibble
    ld b, a                 ; b = current state (DPAD)

    ld a, P1F_GET_BTN       ; get BTN
    ld [c], a
REPT 6
    ld a, [c]              ; poll for a few cycles
ENDR
    or a, %11110000         ; set upper 4 bits
    xor a, b                ; Mix with DPAD bits, and invert all bits (such that pressed=1)
    ld b, a                 ; b = current state (DPAD + BTN)

    ld a, P1F_GET_NONE      ; reset joypad register
    ld [c], a

    ld a, b                 ; check if reset key combo is pressed
    cp a, PADF_START | PADF_SELECT | PADF_B | PADF_A
    jr z, .tryReset

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
    ; fallthrough

_Reset:
    ldh a, [hInitRegisterB]
    ld b, a
    ldh a, [hInitRegisterA]
    ; fallthrough

ASSERT @ == $0100

_Header:
    nop
    jp _Start

; Reserve ROM space for rgbfix
DS $0150 - @, $00

ASSERT @ == $0150

_Start:
    di                      ; disble interrupts

    ld sp, hTempStackTop    ; Use this as stack for a while

    push af                 ; Save CPU type
    push bc

    xor a, a
    ldh [rNR52], a          ; Switch off sound

    call _SetRandomSeed

    ld hl, _RAM             ; Clear RAM
    ld bc, $2000
    ld d, $00
    call _MemSet

    pop bc                  ; Get CPU type
    pop af

    ld sp, wStackTop        ; Switch to real stack

    push af                 ; Save CPU Type
    push bc
    
    ld hl,_HRAM             ; Clear high RAM (and rIE)
    ld bc,$0080
    ld d,$00
    call _MemSet

    pop bc                  ; Get CPU type
    pop af

    ldh [hInitRegisterA], a ; Save CPU type into HRAM
    ld a, b
    ldh [hInitRegisterB], a

    ldh a, [hInitRegisterA]
    cp a, BOOTUP_A_CGB
    jr nz, .notGBC
    ld a, TRUE
    ldh [hIsGBCFlag], a

.notGBC:
    ldh a, [hInitRegisterB]
    cp a, BOOTUP_B_AGB
    jr nz, .notGBA
    ld a, TRUE
    ldh [hIsGBAFlag], a

.notGBA:
    call _ScreenOff

    ld hl, _VRAM        ; Clear VRAM
    ld bc, $2000
    ld d, $00
    call _VideoMemSet

    call _InitOAM       ; Copy OAM refresh function to high ram
    call _RefreshOAM    ; We filled RAM with $00, so this will clear OAM

    call _BankHandlerInit

    ; Real program starts here
    call _Main

    ; Should never reach this point!

_CrashHandler:
    jp _Reset

; Set CPU Speed to DoubleSpeed
_SetCPUFast::
    ldh a, [rKEY1]
    bit 7, a
    jr z, _SwitchCPUSpeed
    ret

; Set CPU to SingleSpeed
_SetCPUSlow::
    ldh a, [rKEY1]
    bit 7, a
    jr nz, _SwitchCPUSpeed
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

_BankHandlerInit:
    xor a, a
    ld [hBankStackPointer], a
    ld b, 1
    call _BankSet   ; select rom bank 1
    ret

_BankPop:
    ld hl, hBankStackPointer
    dec [hl]
    ld hl, hBankStack
    ld d, $00
    ld a, [hBankStackPointer]
    ld e, a
    add hl, de      ; hl now holds the pointer to the bank we want to change to
    ld a, [hl]      ; and a the bank we want to change to
    ld [rROMB0], a  ; select rom bank
    ret

_BankPush:
    ld hl,hBankStackPointer
    inc [hl]
    ret

; - b = bank to change to
_BankSet:
    ld hl, hBankStack
    ld d, $00
    ld a, [hBankStackPointer]
    ld e, a
    add hl, de
    ld a, b         ; hl = pointer to stack, a = bank to change to
    ld [hl], a
    ld [rROMB0], a  ; select rom bank
    ret

; - b = bank to change to
_BankPushSet:
    ld hl,hBankStackPointer
    inc [hl]
    ld hl, hBankStack
    ld d, $00
    ld a, [hBankStackPointer]
    ld e, a
    add hl, de
    ld a, b         ; hl = pointer to stack, a = bank to change to
    ld [hl], a
    ld [rROMB0], a  ; select rom bank
    ret

; - hl = function
; - b = bank where it is located
_LongCall::
    push hl
    call _BankPushSet
    pop hl
    rst _CallHL
    call _BankPop
    ret

; - hl = function
; - a = bank where it is located
;
; can use bc and de for passing arguments
_LongCallArgs::
    push bc
    push de
    push hl
    ld b, a
    call _BankPushSet
    pop hl
    pop de
    pop bc
    rst _CallHL
    call _BankPop
    ret

;- hl = returned value
;- a,c = initial values
;
; super fast unrolled multiplication
; 4 + 7 * [7/6] + [9/6] = [62/52] Cycles, including return :)
_Multiply::
    ld hl, $0000    ; 3   -> 4
    ld b, l         ; 1
    rla ; bit 7     ; 1
    jr nc, .skip0   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip0:
    add hl, hl      ; 2
    rla ; bit 6     ; 1
    jr nc, .skip1   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip1:
    add hl, hl      ; 2
    rla ; bit 5     ; 1
    jr nc, .skip2   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip2:
    add hl, hl      ; 2
    rla ; bit 4     ; 1
    jr nc, .skip3   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip3:
    add hl, hl      ; 2
    rla ; bit 3     ; 1
    jr nc, .skip4   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip4:
    add hl, hl      ; 2
    rla ; bit 2     ; 1
    jr nc, .skip5   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip5:
    add hl, hl      ; 2
    rla ; bit 1     ; 1
    jr nc, .skip6   ; 3/2 -> 7/6
    add hl, bc      ; 2
.skip6:
    add hl, hl      ; 2
    rla ; bit 0     ; 1
    ret nc          ; 5/2 -> 9/6
    add hl, bc      ; 2
    ret             ; 4

IF    0 ; Old version

    ld      hl,$0000
    ld      b,h
.nextBit:
    bit     0,a
    jr      z,.noAdd
    add     hl,bc
.noAdd:
    sla     c
    rl      b ; bc <<= 1
    srl     a ; a >>= 1
    jr      nz,.nextBit

    ret

ENDC

;- hl = returned value
;- a,c = values (a is signed)
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

;- a / b -> c
;- a % b -> a
_Divide::
    inc b
    dec b
    jr z, .divideByZero ; check if divisor is 0
    ld c, $FF ; -1
.continue:
    inc c
    sub a, b ; if a > b then a -= b , c ++
    jr nc, .continue ; if a > b continue
    add a, b ; fix remainder
    and a, a ; clear carry
    ret

.divideByZero
    ld a, $FF
    ld c, $00
    scf ; set carry
    ret

;- a / b -> c
;- a % b -> a
_DivideSigned::
    ld e, $00 ; bit 0 of e = result sign (0/1 = +/-)
    bit 7, a
    jr z, .positiveDividend
    inc e
    cpl     ; change sign
    inc a
.positiveDividend:
    bit 7, b
    jr z, .positiveDivisor
    ld c, a
    ld a, b
    cpl
    inc a
    ld b, a ; change sign
    inc e
.positiveDivisor:
    call _Divide
    ret c   ; if division by 0, exit now

    bit 0, e
    ret z   ; exit if both signs are the same

    ld b, a ; save modulo
    ld a, c ; change sign
    cpl
    inc a
    ld c, a
    ld a, b ; get modulo
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


; Sets random state bits BCDE = AAAA, where A is the sum of all values in WRAM
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
    ld hl, _RAM
    ld bc, $2000
    ld e, $00
.loop:
    ld a, e
    add a, [hl]
    ld e, a
    inc hl
    dec bc
    ld a, b
    or a, c
    jr nz, .loop
    ld a, e
    ld hl, hRandomState
    ld [hl+], a
    ld [hl+], a
    ld [hl+], a
    ld [hl], a
    jp _GetRandom

;- bc = size
;- d = value
;- hl = start address
_MemSet::
    ld a, d
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, _MemSet
    ret

; - bc = size
; - d = value
; - hl = dest address
_VideoMemSet::
    call _WaitScreenBlank
    ld [hl], d
    inc hl
    dec bc
    ld a, b
    or a, c
    jr nz, _VideoMemSet
    ret

;- b = size
;- a = value
;- hl = start address
_MemSetFast::
    ld [hl+], a
    dec b
    jr nz, _MemSetFast
    ret

;- bc = size
;- hl = start address
_MemSetRandom::
    push hl
    call _GetRandom
    pop hl
    ld [hl+], a
    dec bc
    ld a, b
    or a, c
    jr nz, _MemSetRandom
    ret

;- bc = size
;- hl = source address
;- de = dest address
;
; hl and de must be incremented at the end of this
_MemCopy::
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or a, c
    jr nz, _MemCopy
    ret

; - bc = size
; - hl = source address
; - de = dest address
_VideoMemCopy::
    call _WaitScreenBlank
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or a, c
    jr nz, _VideoMemCopy
    ret

;- b = size
;- hl = source address
;- de = dest address
;
; hl and de must be incremented at the end of this
_MemCopyFast::
    ld a, [hl+]
    ld [de], a
    inc de
    dec b
    jr nz, _MemCopyFast
    ret

;- b = size
;- c = increase_src
;- hl = src
;- de = dst
;
; hl and de should be incremented at the end of this
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

; - bc = tiles
; - de = start index
; - hl = source
_VideoMemCopyTiles::
    push hl
    ld h, d
    ld l, e
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl ; index * 16
    ld de, _VRAM
    add hl, de ; dest + base
    ld d, h
    ld e, l
    pop hl

    ; de = dest
    ; hl = src
.copyTile:
    push bc
    ld bc, $0010
    call _VideoMemCopy
    pop bc
    dec bc
    ld a, b
    or c
    jr nz, .copyTile
    ret

;- a = palette number
;- hl = pointer to data
_BackgroundSetPalette::
    swap a      ; \  multiply
    rrca        ; /  palette by 8
    set 7, a    ; auto increment
    ldh [rBCPS], a
REPT 4
    ld a, [hl+]
    ldh [rBCPD], a
    ld a, [hl+]
    ldh [rBCPD], a
ENDR
    ret

;- b = x
;- c = y
;- a = tile index
_BackgroundSetTileWrap::
    ld l, a
    ld h, 31
    ld a, b
    and a, h
    ld b, a
    ld a, c
    and a, h
    ld c, a
    ld a, l

    ; Fall through

;- b = x
;- c = y
;- a = tile index
_BackgroundSetTile::
    ld de, _VRAM
    ld e, b     ; de = base + x
    ld l, c
    ld h, $00   ; hl = y
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl  ; y *  32
    add hl, de  ; hl = base + x + (y * 32)
    ld [hl], a
    ret

;- l = sprite
;- return = hl
;- destroys de
_SpriteGetBasePointer::
    ld h, $00
    add hl, hl
    add hl, hl ; spr number *= 4
    ld de, wOAMCopy
    add hl, de
    ret

;- b = x
;- c = y
;- l = sprite number
_SpriteSetXY::
    call _SpriteGetBasePointer
    ld [hl], c
    inc hl
    ld [hl], b
    ret

;- a = tile
;- l = sprite number
_SpriteSetTile::
    call _SpriteGetBasePointer
    inc hl
    inc hl
    ld [hl], a
    ret

;- a = params
;- l = sprite number
_SpriteSetParams::
    call _SpriteGetBasePointer
    inc hl
    inc hl
    inc hl
    ld [hl], a
    ret

;- a = palette number
;- hl = pointer to data
_SpriteSetPalette::
    swap a      ; \  multiply
    rrca        ; /  palette by 8
    set 7, a    ; auto increment
    ldh [rOCPS], a
REPT 4
    ld a, [hl+]
    ldh [rOCPD], a
    ld a, [hl+]
    ldh [rOCPD], a
ENDR
    ret

_RefreshOAM::
    ld a, wOAMCopy >> 8
    jp hOAMRefreshFunction

_RefreshOAMCustom::
    jp hOAMRefreshFunction

_InitOAM::
    ld b, _OAMRefreshFunction.end - _OAMRefreshFunction
    ld hl, _OAMRefreshFunction
    ld de, hOAMRefreshFunction
    call _MemCopyFast
    ret

_OAMRefreshFunction:
LOAD "OAM Refresh Function", HRAM[_HRAM]

; OAM Refresh function for DMA transfer
hOAMRefreshFunction:
    ldh [rDMA], a
    ld a, $28      ;delay 200ms
.delay:
    dec a
    jr nz, .delay
    ret

ENDL
.end:

ENDSECTION


SECTION "Game Boy Engine Core Variables", HRAM

hInitRegisterA: DS 1
hInitRegisterB: DS 1
hIsGBCFlag::    DS 1
hIsGBAFlag::    DS 1

hIsVBlankFlag:  DS 1

hVBLHandler:    DS 2
hLCDHandler:    DS 2
hTIMHandler:    DS 2
hSIOHandler:    DS 2
hJOYHandler:    DS 2

hCanResetFlag:: DS 1

hKeysHeld::     DS 1
hKeysPressed::  DS 1
hKeysReleased:: DS 1

hRandomState:   DS 4

ENDSECTION


SECTION "HRAM Stack", HRAM[rIE-HRAM_STACK_SIZE-1]

UNION
hBankStack:         DS HRAM_STACK_SIZE
hBankStackPointer:  DS 1
NEXTU
hTempStack:         DS HRAM_STACK_SIZE
hTempStackTop:      DS 1
ENDU

ENDSECTION


SECTION "OAM Copy", WRAM0[_RAM]

; DMA will be used to copy this to OAM
wOAMCopy:   DS $A0

ENDSECTION


SECTION "Stack", WRAM0[_RAMBANK-STACK_SIZE]

wStack:     DS STACK_SIZE
wStackTop:

ENDSECTION

