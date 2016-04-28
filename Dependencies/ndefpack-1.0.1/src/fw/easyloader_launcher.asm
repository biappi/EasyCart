;
; EasyLoader - An menu for the EasyFlash;
;              boot code, under EasyLoader/tools/
;              banking-test, under EasySDK/examples/
;
; This is a mixture of easyloader_launcher and banking-test.
; Modifications:
; - translation of easyloader_launcher from KickAsm to ACME
; - merging the key test code from banking-test
;
; The original easyloader_launcher:
; (c) 2009-2012 ALeX Kazik
;
; The original banking-test:
; (c) 2009-2010 Thomas 'skoe' Giesel
;
; Modifications by:
; (c) 2011-2012 Hannu Nuotio
;
; This software is provided 'as-is', without any express or implied
; warranty.  In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.
;
; ALeX Kazik alex@kazik.de
;
; Original sources available at:
;   https://bitbucket.org/skoe/easyflash
;

EASYFLASH_BANK    = $de00
EASYFLASH_CONTROL = $de02
EASYFLASH_16K     = $07
EASYFLASH_KILL    = $04

* = $ffaa

; This code runs in Ultimax mode after reset, so this memory becomes
; visible at $E000..$FFFF first and must contain a reset vector
start:
    ; === the reset vector points here ===
    sei
    ldx #$ff
    txs
    cld

    ; enable VIC (e.g. RAM refresh)
    lda #8
    sta $d016

    ; write to RAM to make sure it starts up correctly (=> RAM datasheets)
-   sta $0100,x
    dex
    bne -

    ; copy the final start-up code to RAM (bottom of CPU stack)
    ldx #(jump_start_end - jump_start)
-   lda jump_start,x
    sta $0100,x
    dex
    bpl -

    ; load the right starting bank for the mode
!if OCEAN = 1 {
    ldx #1
} else {
    ldx #0
}
    jmp $0100

jump_start:
!pseudopc $0100 {
    ; === this code is copied to the stack area, does some inits ===
    ; === scans the keyboard and kills the cartridge or          ===
    ; === starts the main application                            ===
    lda #EASYFLASH_16K
    sta EASYFLASH_CONTROL

    ; store bank
    stx $de00

    ; Check if one of the magic kill keys is pressed
    ; This should be done in the same way on any EasyFlash cartridge!

    ; Prepare the CIA to scan the keyboard
    lda #$7f
    sta $dc00   ; pull down row 7 (DPA)

    ldx #$ff
    stx $dc02   ; DDRA $ff = output (X is still $ff from copy loop)
    inx
    stx $dc03   ; DDRB $00 = input

    ; Read the keys pressed on this row
    lda $dc01   ; read coloumns (DPB)

    ; Restore CIA registers to the state after (hard) reset
    stx $dc02   ; DDRA input again
    stx $dc00   ; Now row pulled down

    ; Check if one of the magic kill keys was pressed
    and #$e0    ; only leave "Run/Stop", "Q" and "C="
    cmp #$e0
    bne kill    ; branch if one of these keys is pressed

    ; jump into the cartridge
!if OCEAN = 1 {
    jmp ($a000)
} else {
    jmp ($8000)
}

kill:
    lda #EASYFLASH_KILL
    sta EASYFLASH_CONTROL
    jmp ($fffc) ; reset

} ; !pseudopc $0100
jump_start_end:

!if * > $fffa {
!error "code goes over vectors!"
}

    ; fill it up to $FFFA to put the vectors there
!align $ffff, $fffa, $ff

!word reti      ; NMI
!word start     ; RESET

    ; we don't need the IRQ vector and can put RTI here to save space :)
reti:
    rti
    !byte 0xff
