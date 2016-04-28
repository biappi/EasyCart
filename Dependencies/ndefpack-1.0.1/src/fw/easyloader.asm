;
; EasyLoader - An menu for the EasyFlash
;
; This is not the original version. Modifications:
; - translation from KickAsm to ACME
; - some typos in the comments corrected
; - unused space is filled with $ff instead of $00
; - some redundant instructions removed
; - minor optimizations (jmp -> bXX and such)
; - added small delay to cart startup to allow joystick fire release
;
; The original version:
; (c) 2009-2012 ALeX Kazik
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
; Original source available at:
;   https://bitbucket.org/skoe/easyflash
;

; --- some definitions

!initmem $ff    ; fill unused space with $ff

!ct pet     ; set default char encoding to PETSCII

; colors

COLOR_BLACK = 0
COLOR_WHITE = 1
COLOR_CYAN = 3
COLOR_LIGHT_BLUE = 14

; screen

SCREEN = $0400
SCREEN_BASIC = $0400
COLORRAM = $d800
CHARSET_RAM = $2000

;

KEYBUF = $277
KEYBUF_NUM = $c6


; --- Macros

; --- "helper/tools.asm"

!macro copy_to_df00 .start, .len {
    ; TODO .assert "copy_to_df00: len too big", len.getValue() <= 251, true
    lda #<(.start - 1)
    ldx #>(.start - 1)
    ldy #.len
    jsr F_COPY_TO_DF00
}

; - EasyFlash I/O register

MODE_RAM = $04
MODE_16k = $07
MODE_8k  = $06
MODE_ULT = $05

IO_MODE = $de02


; --- "const.asm"

O_DIR_NAME = 0
; !! TYPE, BANK, OFFSET, SIZE must be in same order as in EFS
O_DIR_TYPE = 24
; !! BANK, OFFSET, SIZE, LOADADDR, UNAME must be together (in any order)
O_DIR_BANK = 25
O_DIR_PAD = 26
O_DIR_OFFSET = 27
O_DIR_SIZE = 29
; !! TYPE, BANK, OFFSET, SIZE must be in same order as in EFS
O_DIR_LOADADDR = 32
O_DIR_UNAME = 34
; !! BANK, OFFSET, SIZE, LOADADDR, UNAME must be together (in any order)
V_DIR_SIZE = 50

O_DIR_MODULE_MODE = O_DIR_OFFSET ; OFFET will be reused

O_EFS_NAME = 0
O_EFS_TYPE = 16
O_EFS_BANK = 17
O_EFS_PAD = 18
O_EFS_OFFSET = 19
O_EFS_SIZE = 21
V_EFS_SIZE = 24

O_EFST_MASK = $1f
O_EFST_FILE = $01
O_EFST_FILE_LO = $02
O_EFST_FILE_HI = $03
O_EFST_8KCRT = $10
O_EFST_16KCRT = $11
O_EFST_16KULTCRT = $12
O_EFST_8KULTCRT = $13
O_EFST_END = $1f

; keys

V_KEY_NO = 0
V_KEY_DEL = 1
V_KEY_INS = 2
V_KEY_RETURN = 3
V_KEY_CLEFT = 4
V_KEY_CRIGHT = 5
; regular f-keys
;  V_KEY_F1 = 6
V_KEY_F2 = 7
V_KEY_F3 = 8
V_KEY_F4 = 9
; V_KEY_F5 = 10
; V_KEY_F6 = 11
; V_KEY_F7 = 12
; V_KEY_F8 = 13
V_KEY_CUP = 14
V_KEY_CDOWN = 15
; V_KEY_HOME = 16
V_KEY_CLR = 17
V_KEY_CTRL = 18
V_KEY_SCTRL = 19
V_KEY_COMD = 20
V_KEY_SCOMD = 21
; V_KEY_RUN = 22
; V_KEY_STOP = 23

V_JOYPRESS_FIRE_UP = $01
V_JOYPRESS_FIRE_DOWN = $02
V_JOYPRESS_FIRE_LEFT = $03
V_JOYPRESS_FIRE_RIGHT = $04

; f-keys for movement
V_KEY_F1 = V_KEY_RETURN
V_KEY_F5 = V_KEY_CUP
V_KEY_F6 = V_KEY_CLEFT
V_KEY_F7 = V_KEY_CDOWN
V_KEY_F8 = V_KEY_CRIGHT
V_KEY_HOME = V_KEY_CLR
V_KEY_RUN = V_KEY_CLR
V_KEY_STOP = V_KEY_CLR


; --- "vars.asm"

; ZEROPAGE VARS

!zone zpvars {

; colors.asm

COL_SRC_LO = $02
COL_SRC_HI = $03
COL_DST_LO = $04
COL_DST_HI = $05

; draw.asm

P_DRAW_START = $06
P_DRAW_LAST_START = $07
P_DRAW_OFFSET= $08
P_DRAW_LINES_DIR = $09
ZP_DRAW_PTR = $0a ; $0b
P_DRAW_SLIDER_SIZE = $0c
P_DRAW_SLIDER_FAC = $0d ; $0e

; somewhere

ZP_ENTRY = $0f ; $10

; scan.asm

P_NUM_DIR_ENTRIES = $11
P_BUFFER = $12
P_DIR_BUFFER = $12
ZP_SCAN_SIZETEXT = $44 ; $12+V_DIR_SIZE
ZP_EFS_ENTRY = $46 ; $47

; input.asm

ZP_INPUT_KEYTABLE = $48 ; $49
ZP_INPUT_LAST_CHAR = $4a
ZP_INPUT_MATRIX = $4b

; menu.asm

P_SCREENSAVER_COUNTER = $4c ; $4d
P_SCREENSAVER_BANK = $4e
P_SCREENSAVER_OFS = $4f

; tools.asm

P_GEN_BUFFER = $50 ; $51 $52
P_BINBCD_IN = $53 ; $54
P_BINBCD_OUT = $55 ; $56 $57
P_LED_STATE = $58

; search.asm

V_SEARCH_MAX_CHAR = 9

P_SEARCH_POS = $59
P_SEARCH_ACTIVE = $5a

} ; !zone zpvars


; OTHER VARS

; free space $2800 until P_DIR

; scan.asm

P_DIR = $4e00 ; until $7fff (256 * V_DIR_SIZE)


; --- "build/screen.asm"

!src "easyloader_screendefs.asm"


; --- THE START OF THE MODULE

* = EASYLOADER_STARTADDRESS
!word F_START ; jump address

; ROUTINES

!zone Subs {

; --- "entries/draw.asm"

F_DRAW:

KEEP_CLEAR = 5
PAGE_SCROLL = 15

P_BOLD_LINE = P_BINBCD_IN

    ; check for correct values

    ; :if P_NUM_DIR_ENTRIES ; LT ; #23 ; ELSE ; multi_page
    lda P_NUM_DIR_ENTRIES
    cmp #23
    bcs multi_page

    ; ONE PAGE!

    ; we're always on page 0 (single page mode)
    lda #0
    sta P_DRAW_START
    lda P_DRAW_OFFSET
    bpl +
    ; offset < 0 -> make 0 (first entry)
    lda #0
    sta P_DRAW_OFFSET
    beq ++
+   ; offset >= 0
    ; :if A ; GE ; P_NUM_DIR_ENTRIES ; ENDIF ; !endif+
    cmp P_NUM_DIR_ENTRIES
    bcc ++
    ; offset > #enties -> make last entry
    lda P_NUM_DIR_ENTRIES
    sta P_DRAW_OFFSET
    dec P_DRAW_OFFSET
++
    jmp end_check

multi_page:
-   ; while P_DRAW_OFFSET < 0
    lda P_DRAW_OFFSET
    bpl +
    ; P_DRAW_OFFSET+=23
    clc
    adc #23
    sta P_DRAW_OFFSET
    ; P_DRAW_START-=23
    lda P_DRAW_START
    sec
    sbc #PAGE_SCROLL
    sta P_DRAW_START
    bcs -
    ; if P_DRAW_START < 0
    lda #0
    sta P_DRAW_START
    sta P_DRAW_OFFSET
+

-   ; while P_DRAW_OFFSET >= 23
    ; lda P_DRAW_OFFSET   ; already in A
    ; :if A ; LT ; #23 ; !end_while+
    cmp #23
    bcc +
    ; P_DRAW_OFFSET-=23
    sec
    sbc #PAGE_SCROLL
    sta P_DRAW_OFFSET
    ; P_DRAW_START+=23
    lda P_DRAW_START
    clc
    adc #PAGE_SCROLL
    sta P_DRAW_START
    bcc -
    ; if P_DRAW_START > 255
    lda P_DRAW_LAST_START
    sta P_DRAW_START
    lda #22
    sta P_DRAW_OFFSET
+

    ; if P_DRAW_START > last start
    ; :if P_DRAW_START ; GT ; P_DRAW_LAST_START ; ENDIF ; !endif+
    ; :sub P_DRAW_LAST_START ; P_DRAW_START ; P_DRAW_START
    sec
    lda P_DRAW_LAST_START
    sbc P_DRAW_START
    bcs ++
    sta P_DRAW_START
    ; :sub P_DRAW_OFFSET ; P_DRAW_START ; A
    sec
    lda P_DRAW_OFFSET
    sbc P_DRAW_START
    ; :if A ; GE ; #23 ; ENDIF ; !endif2+
    cmp #23
    bcc +
    lda #22
+   ; !endif2:
    sta P_DRAW_OFFSET
    lda P_DRAW_LAST_START
    sta P_DRAW_START
++ ; !endif:

-   ; :if P_DRAW_OFFSET ; GE ; #KEEP_CLEAR ; !end_while+
    lda P_DRAW_OFFSET
    cmp #KEEP_CLEAR
    bcs +
    ; :if P_DRAW_START ; EQ ; #0 ; !end_while+
    lda P_DRAW_START
    beq +
    inc P_DRAW_OFFSET
    dec P_DRAW_START
    jmp -
+
-   ; :if P_DRAW_OFFSET ; LT ; #23-KEEP_CLEAR ; !end_while+
    lda P_DRAW_OFFSET
    cmp #23-KEEP_CLEAR
    bcc +
    ; :if P_DRAW_START ; EQ ; P_DRAW_LAST_START ; !end_while+
    lda P_DRAW_START
    cmp P_DRAW_LAST_START
    beq +
    dec P_DRAW_OFFSET
    inc P_DRAW_START
    jmp -
+   ; !end_while:

end_check:

    ; :mov16 #P_DIR ; ZP_ENTRY
    lda #<P_DIR
    sta ZP_ENTRY
    lda #>P_DIR
    sta ZP_ENTRY+1

    ; :mov16 #$0400+40+1 ; ZP_DRAW_PTR
    lda #<($0400+40+1)
    sta ZP_DRAW_PTR
    lda #>($0400+40+1)
    sta ZP_DRAW_PTR+1

    lda P_DRAW_OFFSET
    sta P_BOLD_LINE

    ldx P_DRAW_START
    beq ++
-   ; :add16_8 ZP_ENTRY ; #V_DIR_SIZE
    clc
    lda ZP_ENTRY
    adc #V_DIR_SIZE
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+   dex
    bne -
++

    lda P_NUM_DIR_ENTRIES
    bne +   ; !skip+
    ; zero entries
    ldx #22
    jmp draw_empty
+
    ; :sub A ; P_DRAW_START
    sec
    sbc P_DRAW_START
    ; :if A ; Gx ; #23 ; ENDIF ; !endif+
    cmp #23
    bcc +
    lda #23
+   ; !endif:
    sta P_DRAW_LINES_DIR
    tax
    dex

--  ; !bigloop:
    ; draw a line
    ldy #O_DIR_TYPE-1

    lda P_BOLD_LINE
    beq +   ; !bold+

-   ; !loop:
    lda (ZP_ENTRY),y
    sta (ZP_DRAW_PTR),y
    dey
    bpl -  ; !loop-
    bmi ++ ; !next+

+   ; !bold:
-   ; !loop:
    lda (ZP_ENTRY),y
    eor #$80
    sta (ZP_DRAW_PTR),y
    dey
    bpl - ; !loop-

++ ; !next:
    dec P_BOLD_LINE

    ; :add16_8 ZP_ENTRY ; #V_DIR_SIZE
    clc
    lda ZP_ENTRY
    adc #V_DIR_SIZE
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+
    ; :add16_8 ZP_DRAW_PTR ; #40
    clc
    lda ZP_DRAW_PTR
    adc #40
    sta ZP_DRAW_PTR
    bcc +
    inc ZP_DRAW_PTR+1
+

    dex
    bpl -- ; !bigloop-

    ; draw empty
    lda #23
    ; :sub A ; P_DRAW_LINES_DIR
    sec
    sbc P_DRAW_LINES_DIR
    beq FDRAW_finish
    tax
    dex

draw_empty:
-- ; !bigloop:
    ; draw a line
    ldy #O_DIR_TYPE-1
    lda #32
-   sta (ZP_DRAW_PTR), y
    dey
    bpl -

    ; :add16_8 ZP_DRAW_PTR ; #40
    clc
    lda ZP_DRAW_PTR
    adc #40
    sta ZP_DRAW_PTR
    bcc +
    inc ZP_DRAW_PTR+1
+
    dex
    bpl -- ;!bigloop-

FDRAW_finish:
    ; SLIDER!

before_slider = P_BUFFER+6
after_slider = P_BUFFER+7

    ; init ptr to screen+colmem
    ; :mov16 #$400+40+25 ; ZP_ENTRY
    lda #<(SCREEN + 40+25)
    sta ZP_ENTRY
    lda #>(SCREEN + 40+25)
    sta ZP_ENTRY+1
    ;:mov16 #$d800+40+25 ; COL_SRC_LO
    lda #<(COLORRAM + 40+25)
    sta COL_SRC_LO
    lda #>(COLORRAM + 40+25)
    sta COL_SRC_LO+1

    ; check whether to draw a slider
    lda P_DRAW_SLIDER_SIZE
    bne has_slider

    ; no slider!
    ; :mov #23 ; after_slider
    lda #23
    sta after_slider
    ldy #0
    jmp FDRAW_last_part

has_slider:
    ; calc position of slider
    ;.const src1 = P_BUFFER+0
    ;.const src2 = P_BUFFER+2
    ;.const dst = P_BUFFER+4

    ; :mov16 P_DRAW_SLIDER_FAC ; src1
    lda P_DRAW_SLIDER_FAC
    sta P_BUFFER+0
    lda P_DRAW_SLIDER_FAC+1
    sta P_BUFFER+1

    ; :mov P_DRAW_START ; src2
    lda P_DRAW_START
    sta P_BUFFER+2

    ; :mul16_8 src1 ; src2 ; dst
    lda #$00
    tay
    beq ++
--  clc
    adc P_BUFFER+0
    tax
    tya
    adc P_BUFFER+1
    tay
    txa
-   asl P_BUFFER+0
    rol P_BUFFER+1
++  lsr P_BUFFER+2
    bcs --
    bne -
    sta P_BUFFER+4
    sty P_BUFFER+5

    ldx P_BUFFER+5  ; dst+1
    lda P_BUFFER+4  ; dst+0
    bpl +
    inx
+   stx before_slider

    ; middle = size
    lda #23
    sec
    sbc P_DRAW_SLIDER_SIZE
    sbc before_slider
    sta after_slider

    ldy #0

    ; draw 'before_slider'

    ldx before_slider
    beq middle_part

-   ; :mov #$84 ; (ZP_ENTRY), y
    ; :mov #color_slider_off ; (COL_SRC_LO), y
    lda #$84
    sta (ZP_ENTRY),y
    lda #color_slider_off
    sta (COL_SRC_LO),y
    jsr f_next_line
    dex
    bne -

middle_part:
    ; draw top thing
    ; :mov #$03 ; (ZP_ENTRY), y
    ; :mov #color_slider_on ; (COL_SRC_LO), y
    lda #3
    sta (ZP_ENTRY),y
    lda #color_slider_on
    sta (COL_SRC_LO),y
    jsr f_next_line

    ; draw middle thing (if needed)
    ldx P_DRAW_SLIDER_SIZE
    dex
    dex
    beq +
-   ; :mov #$04 ; (ZP_ENTRY), y
    ; :mov #color_slider_on ; (COL_SRC_LO), y
    lda #4
    sta (ZP_ENTRY),y
    lda #color_slider_on
    sta (COL_SRC_LO),y
    jsr f_next_line
    dex
    bne -
+
    ; draw bottom thing
    ; :mov #$05 ; (ZP_ENTRY), y
    ; :mov #color_slider_on ; (COL_SRC_LO), y
    lda #5
    sta (ZP_ENTRY),y
    lda #color_slider_on
    sta (COL_SRC_LO),y
    jsr f_next_line

FDRAW_last_part:
    ; draw 'after_slider'
    ldx after_slider
    beq FDRAW_return

-   ; :mov #$84 ; (ZP_ENTRY), y
    ; :mov #color_slider_off ; (COL_SRC_LO), y
    lda #$84
    sta (ZP_ENTRY),y
    lda #color_slider_off
    sta (COL_SRC_LO),y
    jsr f_next_line
    dex
    bne -

FDRAW_return:
    rts

f_next_line:
    ;:add16_8 ZP_ENTRY ; #40
    ;:add16_8 COL_SRC_LO ; #40
    clc
    lda ZP_ENTRY
    adc #40
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+
    clc
    lda COL_SRC_LO
    adc #40
    sta COL_SRC_LO
    bcc +
    inc COL_SRC_LO+1
+
    rts


F_DRAW_PRECALC:
    ; :if P_NUM_DIR_ENTRIES ; LE ; #23 ; ELSE ; !skip+
    lda #23
    cmp P_NUM_DIR_ENTRIES
    bcc +
    ; we have <= 23 enties -> single page
    ; :mov #0 ; P_DRAW_SLIDER_SIZE
    lda #0
    sta P_DRAW_SLIDER_SIZE
    rts
+   ; more than a page
    ;.const src1 = P_BUFFER+0
    ;.const src2 = P_BUFFER+2
    ;.const rem = P_BUFFER+4
    ; calc size
    ; :mov16 #23*23 ; src1
    lda #<(23*23)
    sta P_BUFFER+0
    lda #>(23*23)
    sta P_BUFFER+1
    ; :mov P_NUM_DIR_ENTRIES ; src2+0
    ; :mov #0 ; src2+1
    lda P_NUM_DIR_ENTRIES
    sta P_BUFFER+2
    lda #0
    sta P_BUFFER+3

    ; will destroy first byte of P_DRAW_SLIDER_FAC (will be calc'ed later)
    ;:div16_round src1 ; src2 ; P_DRAW_SLIDER_SIZE ; rem ; X
    lda #0
    sta P_BUFFER+4
    sta P_BUFFER+5
    ldx #$10
-   asl P_BUFFER+0
    rol P_BUFFER+1
    rol P_BUFFER+4
    rol P_BUFFER+5
    sec
    lda P_BUFFER+4
    sbc P_BUFFER+2
    sta P_BUFFER+4
    lda P_BUFFER+5
    sbc P_BUFFER+3
    sta P_BUFFER+5
    bcs +
    clc
    lda P_BUFFER+4
    adc P_BUFFER+2
    sta P_BUFFER+4
    lda P_BUFFER+5
    adc P_BUFFER+3
    sta P_BUFFER+5
    clc
+   rol P_DRAW_SLIDER_SIZE
    rol P_DRAW_SLIDER_SIZE+1
    dex
    bne -
    asl P_BUFFER+4
    rol P_BUFFER+5
    lda P_BUFFER+3
    cmp P_BUFFER+5
    bcc +
    lda P_BUFFER+5
    cmp P_BUFFER+3
    bcc ++
    lda P_BUFFER+4
    cmp P_BUFFER+2
    bcc ++
+   inc P_DRAW_SLIDER_SIZE
    bne ++
    inc P_DRAW_SLIDER_SIZE+1
++

    ; basic checks: size should be 2-22
    ; :if P_DRAW_SLIDER_SIZE ; LT ; #2 ; ELSE ; !else+
    lda P_DRAW_SLIDER_SIZE
    cmp #2
    bcs +
    ; size < 2 -> make it two
    lda #2
    sta P_DRAW_SLIDER_SIZE
    bne ++  ; bra
+   ; !else: :if P_DRAW_SLIDER_SIZE ; GT ; #22 ; ENDIF ; !endif+
    lda #22
    cmp P_DRAW_SLIDER_SIZE
    bcs ++
    sta P_DRAW_SLIDER_SIZE
++  ; !endif:

    ; calc factor
    ; :mov #0 ; src1+0
    lda #0
    sta P_BUFFER+0
    ; :sub #23 ; P_DRAW_SLIDER_SIZE ; [P_BUFFER+1]
    sec
    lda #23
    sbc P_DRAW_SLIDER_SIZE
    sta P_BUFFER+1

    ; :mov P_DRAW_LAST_START ; src2+0
    ; :mov #0 ; src2+1
    lda P_DRAW_LAST_START
    sta P_BUFFER+2
    lda #0
    sta P_BUFFER+3

    ;:div16_round src1 ; src2 ; P_DRAW_SLIDER_FAC ; rem ; X
    lda #0
    sta P_BUFFER+4
    sta P_BUFFER+5
    ldx #$10
-   asl P_BUFFER+0
    rol P_BUFFER+1
    rol P_BUFFER+4
    rol P_BUFFER+5
    sec
    lda P_BUFFER+4
    sbc P_BUFFER+2
    sta P_BUFFER+4
    lda P_BUFFER+5
    sbc P_BUFFER+3
    sta P_BUFFER+5
    bcs +
    clc
    lda P_BUFFER+4
    adc P_BUFFER+2
    sta P_BUFFER+4
    lda P_BUFFER+5
    adc P_BUFFER+3
    sta P_BUFFER+5
    clc
+   rol P_DRAW_SLIDER_FAC
    rol P_DRAW_SLIDER_FAC+1
    dex
    bne -
    asl P_BUFFER+4
    rol P_BUFFER+5
    lda P_BUFFER+3
    cmp P_BUFFER+5
    bcc +
    lda P_BUFFER+5
    cmp P_BUFFER+3
    bcc ++
    lda P_BUFFER+4
    cmp P_BUFFER+2
    bcc ++
+   inc P_DRAW_SLIDER_FAC
    bne ++
    inc P_DRAW_SLIDER_FAC+1
++
    rts


; --- "entries/scan.asm"

F_SCAN_DIR:
!zone Scan {
!if EASYLOADER_BANK != EASYFILESYSTEM_BANK {
FILESYSTEM_START_ADDR = $6800
    ; copy read-addr routine to $df00
    +copy_to_df00 COPY_FILESYSTEM_START, COPY_FILESYSTEM_END - COPY_FILESYSTEM_START
    ; copy the directory
    jsr F_COPY_FILESYSTEM
} else {
FILESYSTEM_START_ADDR = $a000
}
    ; copy read-addr routine to $df00
    +copy_to_df00 READ_LOADADDR_START, READ_LOADADDR_END - READ_LOADADDR_START

    ; :mov16 #[FILESYSTEM_START_ADDR - V_EFS_SIZE] ; ZP_EFS_ENTRY
    lda #<(FILESYSTEM_START_ADDR - V_EFS_SIZE)
    sta ZP_EFS_ENTRY
    lda #>(FILESYSTEM_START_ADDR - V_EFS_SIZE)
    sta ZP_EFS_ENTRY+1
    ; :mov16 #P_DIR ; ZP_ENTRY
    lda #<P_DIR
    sta ZP_ENTRY
    lda #>P_DIR
    sta ZP_ENTRY+1

    ; :mov #0 ; P_NUM_DIR_ENTRIES
    ; :mov #0 ; P_SCREENSAVER_BANK
    lda #0
    sta P_NUM_DIR_ENTRIES
    ; no screensaver for now
    sta P_SCREENSAVER_BANK

.big_loop:
    ; :add16_8 ZP_EFS_ENTRY ; #V_EFS_SIZE
    clc
    lda ZP_EFS_ENTRY
    adc #V_EFS_SIZE
    sta ZP_EFS_ENTRY
    bcc +
    inc ZP_EFS_ENTRY+1
+

    ; copy TYPE,BANK,OFFSET,SIZE to BUFFER (maybe not used...)
    ; parts of the DIR name is scrambled
    ldy #V_EFS_SIZE - 1
-   lda (ZP_EFS_ENTRY),y
    sta P_DIR_BUFFER + O_DIR_TYPE - O_EFS_TYPE,y
    dey
    bpl -

    ; check for screen saver
    ldy #$00
    ; check a char
-   lda (ZP_EFS_ENTRY),y
    cmp screen_saver,y
    bne ++ ;!no_saver+
    iny
    cpy #(screen_saver_end - screen_saver)
    bne -

    ; found screen saver!
    ldy #>$8000
    lda P_DIR_BUFFER + O_DIR_TYPE
    and #O_EFST_MASK
    cmp #O_EFST_8KULTCRT
    bne +
    ldy #>$a000
+   and #$10 ; check for crt
    beq ++ ;!no_saver+
    sty P_SCREENSAVER_OFS
    lda P_DIR_BUFFER + O_DIR_BANK
    sta P_SCREENSAVER_BANK

++ ;!no_saver:
    ; switch by type
    lda P_DIR_BUFFER + O_DIR_TYPE
    bmi .maybe_hidden ; negative number -> bit 7 set -> hidden file
    and #O_EFST_MASK
    ; :if A ; EQ ; #O_EFST_END ; JMP ; return
    cmp #O_EFST_END ; $1f -> EOF
    bne +
    jmp .return
+   ; :if A ; LE ; #O_EFST_FILE_HI ; file
    cmp #(O_EFST_FILE_HI + 1)
    bcc .file
    ; :if A ; LT ; #O_EFST_8KCRT ; big_loop
    cmp #O_EFST_8KCRT   ; not an crt, and other non crt formats are (currently) not supported
    bcc .big_loop
    and #$13    ; strip down to the "use" crt types
    ; :if A ; EQ ; #O_EFST_8KCRT ; rom8
    cmp #O_EFST_8KCRT
    beq .rom8
    ; :if A ; EQ ; #O_EFST_16KCRT ; rom16
    cmp #O_EFST_16KCRT
    beq .rom16
    ; // :if A ; EQ ; #O_EFST_8KULTCRT ; romu8
    ; // :if A ; EQ ; #O_EFST_16KULTCRT ; romu16

.romu8:
.romu16:
    lda #MODE_ULT ; set game/exrom correctly + $20 for overwrite jumper
    jmp .romicon

.rom8:
    lda #MODE_8k ; set game/exrom correctly + $20 for overwrite jumper
    jmp .romicon

.rom16:
    lda #MODE_16k ; set game/exrom correctly + $20 for overwrite jumper
    jmp .romicon

.maybe_hidden:
    and #O_EFST_MASK
    ; :if A ; EQ ; #O_EFST_END ; JMP ; return // $1f -> EOF
    cmp #O_EFST_END ; $1f -> EOF
    bne +
    jmp .return
+   jmp .big_loop ; is a hidden file

.file:
    ; get offset within first bank
    lda P_DIR_BUFFER + O_DIR_OFFSET+0
    sta ZP_SCAN_SIZETEXT+0
    lda P_DIR_BUFFER + O_DIR_OFFSET+1
    clc
    adc #>$8000
    sta ZP_SCAN_SIZETEXT+1

    ; get bank
    lda P_DIR_BUFFER + O_DIR_BANK
    ; read start-addr
    jsr F_READ_LOADADDR

    ; check for a valid file
    ; :if P_DIR_BUFFER + O_DIR_SIZE+2 ; NE ; #$00 ; not_loadable
    lda P_DIR_BUFFER + O_DIR_SIZE+2
    bne .not_loadable ; size >64k
    ; :if16 P_DIR_BUFFER + O_DIR_LOADADDR+1 ; LT ; #$01 ; not_loadable  ; BUG, checks +2
    lda P_DIR_BUFFER + O_DIR_LOADADDR+1
    cmp #1
    bcc .not_loadable ; loadaddr < $0100
    ; :sub16 P_DIR_BUFFER + O_DIR_LOADADDR ; #3 ; ZP_SCAN_SIZETEXT
    sec
    lda P_DIR_BUFFER + O_DIR_LOADADDR
    sbc #3
    sta ZP_SCAN_SIZETEXT
    lda P_DIR_BUFFER + O_DIR_LOADADDR+1
    sbc #0
    sta ZP_SCAN_SIZETEXT+1
    ; :add16 ZP_SCAN_SIZETEXT ; P_DIR_BUFFER + O_DIR_SIZE
    clc
    lda ZP_SCAN_SIZETEXT
    adc P_DIR_BUFFER + O_DIR_SIZE
    sta ZP_SCAN_SIZETEXT
    lda ZP_SCAN_SIZETEXT+1
    adc P_DIR_BUFFER + O_DIR_SIZE+1
    sta ZP_SCAN_SIZETEXT+1
    bcs .not_loadable ; loadaddr+size (minus 2 for loadaddr) > $ffff (future limit)
    ; :if ZP_SCAN_SIZETEXT+1 ; GE ; #$d0 ; not_loadable
    lda ZP_SCAN_SIZETEXT+1
    cmp #>$d000
    bcs .not_loadable ; >= $d000 (current limit)

    ldx #$7d
    jmp .copyit

.not_loadable:
    sta P_DIR_BUFFER+O_DIR_TYPE ; 0 => type => not loadable
    ldx #$1f
    jmp .copyit

.romicon:
    ; remember mode (8k,16k,ultimax)
    sta P_DIR_BUFFER + O_DIR_MODULE_MODE
    ldx #$7b
.copyit:
    ; copy icon (X) (part of the new name)
    stx P_DIR_BUFFER+0
    inx
    stx P_DIR_BUFFER+1

    ; copy name
    ldy #O_EFS_TYPE-1
-   lda (ZP_EFS_ENTRY),y
    beq .char_ok    ; keep $00
    cmp #$20        ; $01-$1f => bad
    bcc .char_bad
    cmp #$60        ; $60 => bad
    beq .char_bad
    cmp #$7b        ; $7b-$ff => bad
    bcc .char_ok
.char_bad:
    lda #'*'
.char_ok:
    sta P_DIR_BUFFER+O_DIR_UNAME,y
    ; cmp #$00
    bne +
    lda #' '
+   sta P_DIR_BUFFER+2,y
    dey
    bpl -

    ; create upper petscii name
    ldy #O_EFS_TYPE-1
-   lda P_DIR_BUFFER+O_DIR_UNAME,y
    ; :if A ; LT ; #$61 ; !skip+
    cmp #$61
    bcc +
    eor #$20
    sta P_DIR_BUFFER+O_DIR_UNAME,y
+   dey
    bpl -

    ldx #18
    ; a space
    lda #32
    sta P_DIR_BUFFER,x
    inx
    ; size
    lda P_DIR_BUFFER + O_DIR_TYPE
    and #$10
    bne +++ ; !at_least_xxxk+
    ; :if P_DIR_BUFFER + O_DIR_SIZE+2 ; NE ; #$00 ; !at_least_64k+
    lda P_DIR_BUFFER + O_DIR_SIZE+2
    bne +++
    ; :if16 P_DIR_BUFFER + O_DIR_SIZE ; LE ; #999 ; JMP ; show_bytes
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    cmp #>999
    bcs +
    jmp .show_bytes
+   lda #>999
    cmp P_DIR_BUFFER + O_DIR_SIZE+1
    bcc +
    lda #<999
    cmp P_DIR_BUFFER + O_DIR_SIZE
    bcc +
    jmp .show_bytes
+   ; :if16 P_DIR_BUFFER + O_DIR_SIZE ; LE ; #[9.9*1024] ; JMP ; show_x_x_kbytes
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    cmp #$27
    bcs +
    jmp .show_x_x_kbytes
+   lda #$27
    cmp P_DIR_BUFFER + O_DIR_SIZE+1
    bcc +
    lda #$99
    cmp P_DIR_BUFFER + O_DIR_SIZE
    bcc +
    jmp .show_x_x_kbytes
+
    ;!at_least_xxxk:
+++ ;!at_least_64k:
    ; :if16 P_DIR_BUFFER + O_DIR_SIZE+1 ; LE ; #[[999*1024]>>8] ; JMP ; show_xxx_kbytes
    lda P_DIR_BUFFER + O_DIR_SIZE+2
    cmp #$0f
    bcs +
    jmp .show_xxx_kbytes
+   lda #$0f
    cmp P_DIR_BUFFER + O_DIR_SIZE+2
    bcc +
    lda #$9c
    cmp P_DIR_BUFFER + O_DIR_SIZE+1
    bcc +
    jmp .show_xxx_kbytes
+
    jmp .show_x_x_mbytes

.next_after_size:
    ; end of line
    lda #$7f
    sta P_DIR_BUFFER,x

    ; copy buffer
    ldy #V_DIR_SIZE-1
-   lda P_DIR_BUFFER,y
    sta (ZP_ENTRY),y
    dey
    bpl -

    ; next line
    ; :add16_8 ZP_ENTRY ; #V_DIR_SIZE
    clc
    lda ZP_ENTRY
    adc #<V_DIR_SIZE
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+
    inc P_NUM_DIR_ENTRIES
    jmp .big_loop

.return:
    ; P_DRAW_LAST_START = max(0, P_NUM_DIR_ENTRIES-23)
    lda P_NUM_DIR_ENTRIES
    sec
    sbc #23
    bcs +
    lda #0
+   sta P_DRAW_LAST_START

    ; calc slider things
    jmp F_DRAW_PRECALC
    ; return (thru rts in precalc)

.show_bytes:
    ; :mov16 P_DIR_BUFFER + O_DIR_SIZE ; P_BINBCD_IN
    lda P_DIR_BUFFER + O_DIR_SIZE
    sta P_BINBCD_IN
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    sta P_BINBCD_IN+1
    lda #$62
    jmp .show_xxx

.show_x_x_kbytes:
    ; :mov16 P_DIR_BUFFER + O_DIR_SIZE ; ZP_SCAN_SIZETEXT
    lda P_DIR_BUFFER + O_DIR_SIZE
    sta ZP_SCAN_SIZETEXT
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    sta ZP_SCAN_SIZETEXT+1
    asl ZP_SCAN_SIZETEXT
    rol ZP_SCAN_SIZETEXT+1
    asl ZP_SCAN_SIZETEXT
    rol ZP_SCAN_SIZETEXT+1
    lda #$4b
    bne .show_x_x   ; bra

.show_xxx_kbytes:
    ; :mov16 P_DIR_BUFFER + O_DIR_SIZE+1 ; P_BINBCD_IN
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    sta P_BINBCD_IN
    lda P_DIR_BUFFER + O_DIR_SIZE+2
    sta P_BINBCD_IN+1
    lsr P_BINBCD_IN+1
    ror P_BINBCD_IN
    lsr P_BINBCD_IN+1
    ror P_BINBCD_IN
    bcc +
    ; the last bit shifted down was set -> round
    inc P_BINBCD_IN
    bne +
    inc P_BINBCD_IN+1
+   lda #$4b
    bne .show_xxx   ; bra

.show_x_x_mbytes:
    ; :mov16 P_DIR_BUFFER + O_DIR_SIZE+1 ; ZP_SCAN_SIZETEXT
    lda P_DIR_BUFFER + O_DIR_SIZE+1
    sta ZP_SCAN_SIZETEXT
    lda P_DIR_BUFFER + O_DIR_SIZE+2
    sta ZP_SCAN_SIZETEXT+1
    lda #$4d
    ;jmp .show_x_x  ; fall through

.show_x_x:
    pha ; keep unit
    lda ZP_SCAN_SIZETEXT+1
    sta P_BINBCD_IN
    lsr P_BINBCD_IN
    lsr P_BINBCD_IN
    lsr P_BINBCD_IN
    lsr P_BINBCD_IN

    ; convert bin->dec
    jsr F_BINBCD_8BIT

    ; display 1 digit
    lda P_BINBCD_OUT
    jsr F_BCDIFY_LOWER_BUF

    ; display "."
    lda #'.'
    sta P_DIR_BUFFER,x
    inx

    ; calculate the nachkommastelle FIXME ???
    lda ZP_SCAN_SIZETEXT+1
    and #$0f
    sta ZP_SCAN_SIZETEXT
    lsr
    lsr
    clc
    adc ZP_SCAN_SIZETEXT
    lsr
    jsr F_BCDIFY_LOWER_BUF

    ; display unit
    pla
    sta P_DIR_BUFFER,x
    inx

    jmp .next_after_size

.show_xxx:
    pha ; keep unit

    ; convert bin->dec
    jsr F_BINBCD_10BIT

    ; display 3 digits
    lda P_BINBCD_OUT+1
    jsr F_BCDIFY_LOWER_BUF
    lda P_BINBCD_OUT+0
    jsr F_BCDIFY_BUF

    ; remove leading 0
    ldy #$fe
-   lda P_DIR_BUFFER+19-$fe,y
    cmp #'0'
    bne +
    lda #' '
    sta P_DIR_BUFFER+19-$fe,y
    iny
    bne -
+
    ; display unit
    pla
    sta P_DIR_BUFFER,x
    inx

    jmp .next_after_size

screen_saver:
!pet "!el_screen-saver"
screen_saver_end:

READ_LOADADDR_START:
!pseudopc $df00 {
F_READ_LOADADDR:
    sta $de00
    ldy #$00
    lda (ZP_SCAN_SIZETEXT),y
    sta P_DIR_BUFFER + O_DIR_LOADADDR+0
    iny
    lda (ZP_SCAN_SIZETEXT),y
    sta P_DIR_BUFFER + O_DIR_LOADADDR+1
    lda #EASYLOADER_BANK
    sta $de00
    rts
}
READ_LOADADDR_END:

!if EASYLOADER_BANK != EASYFILESYSTEM_BANK {
COPY_FILESYSTEM_START:
!pseudopc $df00 {
F_COPY_FILESYSTEM:
    lda #EASYFILESYSTEM_BANK
    sta $de00

    ldx #$00
-
smc_src:
    lda $a000,x
smc_dst:
    sta $6800,x
    inx
    bne -
    inc smc_src+2
    inc smc_dst+2
    bpl -   ; when src+2 ($60) is negative ($80) we are done

    lda #EASYLOADER_BANK
    sta $de00
    rts
}
COPY_FILESYSTEM_END:
}   ; !if

} ; !zone Scan


; --- "entries/search.asm"

F_SEARCH_INIT:
    ; search is inactive
    lda #$1
    sta P_SEARCH_ACTIVE
    rts

P_SEARCH_SCREEN_OUT = SCREEN + 6*40 + 28
P_SEARCH_COLOR_OUT = COLORRAM + 6*40 + 28

F_SEARCH_START:
    jmp F_SEARCH_DRAW

F_SEARCH_KEY:
!zone Search_key {
.eq_line = P_BINBCD_OUT+0
.eq_len = P_BINBCD_OUT+1
.eq_res = P_BINBCD_OUT+2
.todo = P_BINBCD_IN+0
.cur_line = P_BINBCD_IN+1

    ; load pos
    ldx P_SEARCH_POS
    ; check for V_SEARCH_MAX_CHAR (== 9) chars
    cpx #V_SEARCH_MAX_CHAR
    bne +
    rts
+
    ; fine
    ; store them
    sta P_SEARCH_SCREEN_OUT,x
    lda #COLOR_WHITE
    sta P_SEARCH_COLOR_OUT,x

    ; next pos
    inc P_SEARCH_POS

    ; best match: none
    lda #$ff
    sta .eq_len

    lda #0
    sta .eq_line
    sta .cur_line

    ; lines to work over
    lda P_NUM_DIR_ENTRIES
    sta .todo

    ; absolute first entry
    ;:mov16 #P_DIR - V_DIR_SIZE ; ZP_ENTRY
    lda #<(P_DIR - V_DIR_SIZE)
    sta ZP_ENTRY
    lda #>(P_DIR - V_DIR_SIZE)
    sta ZP_ENTRY+1

.search_loop:
    ; :add16_8 ZP_ENTRY ; #V_DIR_SIZE
    clc
    lda ZP_ENTRY
    adc #<V_DIR_SIZE
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+
    ldx #0 - 1
    ldy #O_DIR_UNAME - 1

.char_loop:
    iny
    inx
    cpx P_SEARCH_POS
    beq .exit_char_loop

    lda P_SEARCH_SCREEN_OUT,x
    sec
    sbc (ZP_ENTRY),y
    ; since all chars are in range $20..$5a there is never a overflow
    beq .char_loop

.exit_char_loop:
    tay

    ; x = position of difference
    ; y = result (0 = equal, <0 = line < search = below, >0 = line > search = above)
    lda .eq_len
    bmi .use
    cpx .eq_len
    bcc .dont_use
    bne .use

    ; identical length, check result
    lda .eq_res
    beq .dont_use   ; last one was already perfect, don't use this
    bmi .was_below

.was_above:
    tya
    bmi .dont_use   ; last: above, now: below -> use
    sec
    sbc .eq_res
    beq .use
    bcc .use
    bcs .dont_use

.was_below:
    tya
    bpl .use        ; last: below, now: above -> dont use
    sec
    sbc .eq_res
    beq .dont_use
    bcc .dont_use
    bcs .use

.use:
    stx .eq_len
    sty .eq_res
    lda .cur_line
    sta .eq_line

.dont_use:
    inc .cur_line
    dec .todo
    bne .search_loop

    lda .eq_line
    sta P_DRAW_OFFSET
} ; !zone Search_key


F_SEARCH_DRAW:
    ; draw search box (not the contents)
    ldx P_SEARCH_POS
    lda #$60
    sta P_SEARCH_SCREEN_OUT,x
    lda #COLOR_CYAN
    sta P_SEARCH_COLOR_OUT,x
    ldx #12
-   lda F_SEARCH_DRAW_line1,x
    sta SCREEN + 5*40 + 26,x
    lda #$82
    sta SCREEN + 7*40 + 26,x
    lda #COLOR_CYAN
    sta COLORRAM + 5*40 + 26,x
    sta COLORRAM + 7*40 + 26,x
    dex
    bpl -
    lda #$80
    sta SCREEN + 6*40 + 26 + 0
    lda #$84
    sta SCREEN + 6*40 + 26 + 12
    lda #COLOR_CYAN
    sta COLORRAM + 6*40 + 26 + 0
    sta COLORRAM + 6*40 + 26 + 12
    lda #$81
    sta SCREEN + 7*40 + 26 + 0
    lda #$83
    sta SCREEN + 7*40 + 26 + 12
    lda #'>'
    sta SCREEN + 6*40 + 26 + 1
    lda #COLOR_WHITE
    sta COLORRAM + 6*40 + 26 + 1

    ; search is active
    lda #0
    sta P_SEARCH_ACTIVE

    ; draw screen
    lda #0
    sta P_DRAW_START
    jmp F_DRAW ; includes rts

F_SEARCH_DRAW_line1:
!byte $00, $f3, $c5, $c1, $d2, $c3, $c8, $a0, $01, $01, $01, $01, $02

F_SEARCH_DEL:
    dec P_SEARCH_POS
;   beq F_SEARCH_RESET
    bmi F_SEARCH_RESET
    ldx P_SEARCH_POS
    lda #$60
    sta P_SEARCH_SCREEN_OUT,x
    lda #COLOR_CYAN
    sta P_SEARCH_COLOR_OUT,x
    lda #$20
    sta P_SEARCH_SCREEN_OUT+1,x
    jmp F_DRAW ; includes rts

F_SEARCH_RESET:
!zone Search_reset {
    ; search is inactive
    lda #$1
    sta P_SEARCH_ACTIVE

    lda #0
    sta P_SEARCH_POS

    ldx #12
-   lda ORIG_SCREEN + 5*40 + 26,x
    sta SCREEN + 5*40 + 26,x
    lda ORIG_SCREEN + 6*40 + 26,x
    sta SCREEN + 6*40 + 26,x
    lda ORIG_SCREEN + 7*40 + 26,x
    sta SCREEN + 7*40 + 26,x
    lda #COLOR_LIGHT_BLUE
    sta COLORRAM + 5*40 + 26,x
    sta COLORRAM + 6*40 + 26,x
    sta COLORRAM + 7*40 + 26,x
    dex
    bpl -
    rts
} ; !zone Search_reset


; --- "helper/tools.asm"

F_BINBCD_8BIT:
    sed     ; Switch to decimal mode
    lda #0  ; Ensure the result is clear
    sta P_BINBCD_OUT+0
    sta P_BINBCD_OUT+1
    ldy #8  ; The number of source bits
-   asl P_BINBCD_IN         ; Shift out one bit
    lda P_BINBCD_OUT+0      ; And add into result
    adc P_BINBCD_OUT+0
    sta P_BINBCD_OUT+0
    lda P_BINBCD_OUT+1      ; propagating any carry
    adc P_BINBCD_OUT+1
    sta P_BINBCD_OUT+1
    dey             ; And repeat for next bit
    bne -
    cld             ; Back to binary
    rts             ; All Done.

F_BINBCD_10BIT:
    sed     ; Switch to decimal mode
    lda #0  ; Ensure the result is clear
    sta P_BINBCD_OUT+0
    sta P_BINBCD_OUT+1
    ldy #10 ; The number of source bits

    ; drop 6 bits
!for .i, 6 {
    ; :asl16 P_BINBCD_IN
    asl P_BINBCD_IN
    rol P_BINBCD_IN+1
}

-   ; :asl16 P_BINBCD_IN
    asl P_BINBCD_IN         ; Shift out one bit
    rol P_BINBCD_IN+1
    lda P_BINBCD_OUT+0      ; And add into result
    adc P_BINBCD_OUT+0
    sta P_BINBCD_OUT+0
    lda P_BINBCD_OUT+1      ; propagating any carry
    adc P_BINBCD_OUT+1
    sta P_BINBCD_OUT+1
    dey             ; And repeat for next bit
    bne -
    cld             ; Back to binary
    rts             ; All Done.

F_BINBCD_16BIT:
    sed     ; Switch to decimal mode
    lda #0  ; Ensure the result is clear
    sta P_BINBCD_OUT+0
    sta P_BINBCD_OUT+1
    sta P_BINBCD_OUT+2
    ldy #16  ; The number of source bits

-   ; :asl16 P_BINBCD_IN
    asl P_BINBCD_IN         ; Shift out one bit
    rol P_BINBCD_IN+1
    lda P_BINBCD_OUT+0      ; And add into result
    adc P_BINBCD_OUT+0
    sta P_BINBCD_OUT+0
    lda P_BINBCD_OUT+1      ; propagating any carry
    adc P_BINBCD_OUT+1
    sta P_BINBCD_OUT+1
    lda P_BINBCD_OUT+2      ; propagating any carry
    adc P_BINBCD_OUT+2
    sta P_BINBCD_OUT+2
    dey             ; And repeat for next bit
    bne -
    cld             ; Back to binary
    rts             ; All Done.

F_BCDIFY_BUF:
!zone BCDify {
    tay
    lsr
    lsr
    lsr
    lsr
    jsr .hexc           ; convert upper nibble
    jsr .output
    tya
F_BCDIFY_LOWER_BUF:
    and #$0f            ; convert lower nibble
    jsr .hexc
.output:
    sta P_DIR_BUFFER,x  ; output a byte using Y-index
    inx                 ; increment the output address
    rts
.hexc:
    cmp #$a             ; subroutine converts 0-F to a character
    bcs .hexa
    clc                 ; digit 0-9
    adc #'0'
    bne .hexb           ; unconditional jump coz Z=FALSE always
.hexa:
    lda #' '
.hexb:
    rts
} ; !zone BCDify


F_COPY_TO_DF00:
    sta P_BINBCD_IN+0
    stx P_BINBCD_IN+1
-   lda (P_BINBCD_IN),y
    sta $deff,y
    dey
    bne -
    rts


; --- "loader/common.asm"

F_LAUNCH:
!zone Launch {
.ZP_LINE = P_BUFFER

    lda P_DRAW_OFFSET
    sta .ZP_LINE
    clc
    lda .ZP_LINE
    adc P_DRAW_START
    sta .ZP_LINE

    ; :mul8_16 ZP_LINE ; #V_DIR_SIZE ; ZP_ENTRY ; X
    ldx #8
    lda #0
    sta ZP_ENTRY
    sta ZP_ENTRY+1
-   asl ZP_ENTRY
    rol ZP_ENTRY+1
    asl .ZP_LINE
    bcc +
    lda ZP_ENTRY
    clc
    adc #V_DIR_SIZE
    sta ZP_ENTRY
    bcc +
    inc ZP_ENTRY+1
+   dex
    bne -

    ; :add16 ZP_ENTRY ; #P_DIR
    clc
    lda ZP_ENTRY
    adc #<P_DIR
    sta ZP_ENTRY
    lda ZP_ENTRY+1
    adc #>P_DIR
    sta ZP_ENTRY+1

    ; :lday( ZP_ENTRY , O_DIR_TYPE )
    ldy #O_DIR_TYPE
    lda (ZP_ENTRY),y

    and #O_EFST_MASK
    beq .return     ; 0 => not loadable
    ; if it's a file:
    ; :if A ; LE ; #O_EFST_FILE_HI ; JMP ; F_LAUNCH_FILE
    cmp #O_EFST_FILE_HI+1
    bcs +
    jmp F_LAUNCH_FILE
+   and #$10
    beq .return     ; 0 => (in range $2..$f, unknown) => not loadable
    ; otherwise it must be a crt
    jmp F_LAUNCH_CRT

.return:
    rts
} ; !zone Launch

F_RESET_GRAPHICS:
    ; make screen black
    lda #COLOR_BLACK
    tax
-   sta COLORRAM,x
    sta COLORRAM + $100,x
    sta COLORRAM + $200,x
    sta COLORRAM + $300,x
    dex
    bne -

    ; reset graphics
    lda #$00
    sta $d011
    lda #$c0
    sta $d016
    lda #$01
    sta $d018
    lda #$ff
    sta $dd00
    rts


; --- "loader/basic.asm"

F_BASIC:
    jsr F_LAST_CONFIG_WRITE

    ; fill screen with spaces
    lda #' '
    ldx #$00
-
!for .i, 4 {
    sta SCREEN_BASIC + (.i - 1) * $100,x
}
    dex
    bne -

    ldx #(F_BASIC_SUB_end - F_BASIC_SUB_begin)-1
-   lda F_BASIC_SUB_begin,x
    sta $02,x
    dex
    bpl -

    jmp $02

F_BASIC_SUB_begin:
    lda #MODE_RAM
    sta IO_MODE
    jmp ($fffc)
F_BASIC_SUB_end:


; --- "loader/cart.asm"

F_LAUNCH_CRT:
    jsr F_RESET_GRAPHICS

    +copy_to_df00 LCOPY_START, LCOPY_END - LCOPY_START

    ; get working mode and bank
    ldy #O_DIR_BANK
    lda (ZP_ENTRY),y
    sta $df00
    ldy #O_DIR_MODULE_MODE
    lda (ZP_ENTRY),y
    sta smc_crt_mode+1

    ; keep this config
    jsr F_LAST_CONFIG_WRITE

    ; fill memory as before!!!
    lda #0
    tax
-   sta $00,x
    sta $0100,x
    ; "clean" vic+cia's (done with sid on startup)
    sta $d000,x
    sta $dc00,x
    sta $dd00,x
    inx
    bne -

    ; set timer latches to $ffff (as an physical reset would do)
    lda #$ff
    sta $dc04
    sta $dc05
    sta $dc06
    sta $dc07
    sta $dd04
    sta $dd05
    sta $dd06
    sta $dd07

    ; wait for some time to allow joystick button release
    ldx #28
--  lda $d011
    bpl --
-   lda $d011
    bmi -
    dex
    bne --

    jmp LCOPY_PRG

LCOPY_START:
!pseudopc $df00 {
!byte 0 ; bank lo for xbank's - will be overwritten later
!byte 0 ; bank hi for xbank's
LCOPY_PRG:
    ; switch bank ($8000-$ffff is now undefined!!)
    lda $df00
    sta $de00
smc_crt_mode:
    lda #$00
    sta IO_MODE
    ; jump to reset routine
    jmp ($fffc)
}
LCOPY_END:


; --- "loader/file.asm"

F_LAUNCH_FILE:
    ; remember file mode
    pha

    jsr F_LAST_CONFIG_WRITE

    ; CARTRIDGE IS ACTIVE
    ; DATA in $02-$7fff
    ; EXTRACT DATA
    ; COPY ALL REQUIRED TO $df00-$dffb

    jsr F_RESET_GRAPHICS

    +copy_to_df00 FILE_EXT1_START, FILE_EXT1_END - FILE_EXT1_START

    ; check file mode
    pla
    ; :if A ; EQ ; # O_EFST_FILE_LO ; ELSE ; !else+
    cmp #O_EFST_FILE_LO
    bne +
    lda #>$a000
    sta smc_end1_ofs3+3
    sta smc_end2_ofs3+3
    bne ++
+   ; :if A ; EQ ; # O_EFST_FILE_HI ; ENDIF ; !endif+
    cmp #O_EFST_FILE_HI
    bne ++
    lda #>$a000
    sta smc_start1_ofs1+1
    sta smc_start2_ofs1+1
++

    ; copy bank,offset,size,loadaddr,name (part 1)
    ldy #O_DIR_BANK
-   lda (ZP_ENTRY),y
    sta P_BOSLN-O_DIR_BANK,y
    iny
    cpy #V_DIR_SIZE
    bne -

    ; create integer of loadaddr
    ; convert bin->bcd
    lda P_BOSLN-O_DIR_BANK + O_DIR_LOADADDR
    sta P_BINBCD_IN
    lda P_BOSLN-O_DIR_BANK + O_DIR_LOADADDR + 1
    sta P_BINBCD_IN+1
    jsr F_BINBCD_16BIT
    ; convert bcd->petscii
    ldx #$00
    lda P_BINBCD_OUT+2
    jsr F_BCDIFY_LOWER_BUF
    lda P_BINBCD_OUT+1
    jsr F_BCDIFY_BUF
    lda P_BINBCD_OUT+0
    jsr F_BCDIFY_BUF
    ; strip leading spaces
    ldx #0
-   lda P_BUFFER,x
    cmp #'0'
    bne +
    lda #' '
    sta P_BUFFER,x
    inx
    cpy #4
    bne -
+
    ; copy to a safe place
    ldx #4
-   lda P_BUFFER,x
    sta P_SYS_NUMBERS,x
    dex
    bpl -

    ; DO A PARTIAL RESET (EVERYTHING EXCEPT I/O IS LOST)

    jmp DO_PARTIAL_RESET
BACK_FROM_PARTIAL_RESET:

    ; SETUP A HOOK IN THE CHRIN VECTOR

    ; change CHRIN vector
    lda $324
    sta SMC_RESTORE_LOWER+1
    lda $325
    sta SMC_RESTORE_UPPER+1

    ; add our vector
    lda #<RESET_TRAP
    sta $324
    lda #>RESET_TRAP
    sta $325

    ; DETECT A C64GS KERNAL, IF SO CREATE A SECOND TRAP (CHROUT VECTOR)

    ; :if16 $e449 ; EQ ; #$f72e ; ENDIF ; !endif+
    lda $e44a
    cmp #>$f72e
    bne +
    lda $e449
    cmp #<$f72e
    bne +

    lda $326
    sta SMC_RESTORE_CHROUT_LOWER+1
    lda $327
    sta SMC_RESTORE_CHROUT_UPPER+1

    lda #<CHROUT_TRAP
    sta $326
    lda #>CHROUT_TRAP
    sta $327
+
    ; DO THE REST OF THE RESET

    ; continue reset-routine
    jmp GO_RESET

    ; BACK IN CARTRIDGE

FILE_COPIER:
    ; display >LOADING "xxx",EF,1<

    ldy #0
-   lda loading_1,y
    jsr $ffd2
    iny
    cpy #(loading_1_end - loading_1)
    bne -

    ldy #0
-   lda P_BOSLN-O_DIR_BANK + O_DIR_UNAME,y
    beq +
    jsr $ffd2
    iny
    cpy #16
    bne -
+

    ldy #0
-   lda loading_2,y
    jsr $ffd2
    iny
    cpy #(loading_2_end - loading_2)
    bne -

ZP_BANK = $ba
ZP_SIZE = $07 ; $08 - Temporary Integer during OR/AND
ZP_SRC = $b7 ; $b8
ZP_DST = $ae ; $af

    ; copy bank
    lda P_BOSLN-O_DIR_BANK + O_DIR_BANK
    sta ZP_BANK

    ; copy size-2
    ; :sub16 P_BOSLN-O_DIR_BANK + O_DIR_SIZE ; #2 ; ZP_SIZE
    sec
    lda P_BOSLN-O_DIR_BANK + O_DIR_SIZE
    sbc #2
    sta ZP_SIZE
    lda P_BOSLN-O_DIR_BANK + O_DIR_SIZE + 1
    sbc #0
    sta ZP_SIZE+1
    ; (don't load the load address)

    ; copy offset within first bank
    ; :add16 P_BOSLN-O_DIR_BANK + O_DIR_OFFSET ; #2 ; ZP_SRC
    clc
    lda P_BOSLN-O_DIR_BANK + O_DIR_OFFSET
    adc #2
    sta ZP_SRC
    lda P_BOSLN-O_DIR_BANK + O_DIR_OFFSET + 1
    adc #0
    sta ZP_SRC+1
    ; add 2 (don't load the loadaddress)

    ; if the offset is now >= $4000 switch to next bank
    ;:if ZP_SRC+1 ; EQ ; #$40 ; ENDIF ; !endif+
    ; lda ZP_SRC+1    ; already in A
    cmp #>$4000
    bne +
    lda #$00
    sta ZP_SRC+1
    inc ZP_BANK
+
    ; make offset ($0000-$3fff) to point into real address ($8000-$bfff)
    ; :add ZP_SRC+1 ; #$80
    clc
    lda ZP_SRC+1
    adc #>$8000
    sta ZP_SRC+1

    ; copy dst address
    ; :mov16 P_BOSLN-O_DIR_BANK + O_DIR_LOADADDR ; ZP_DST
    lda P_BOSLN-O_DIR_BANK + O_DIR_LOADADDR
    sta ZP_DST
    lda P_BOSLN-O_DIR_BANK + O_DIR_LOADADDR + 1
    sta ZP_DST+1

    ; :if16 ZP_DST ; LE ; #$0801 ; ELSE ; !else+
    ; lda ZP_DST+1    ; already in A
    cmp #>$0801
    bcc +
    lda #>$0801
    cmp ZP_DST+1
    bcc ++
    lda #<$0801
    cmp ZP_DST
    bcc ++
    ; LOAD ADDR $200-$0801 -> run
+   lda #'r'
    sta KEYBUF
    lda #'u'
    sta KEYBUF + 1
    lda #'n'
    sta KEYBUF + 2
    lda #$0d    ; CR
    sta KEYBUF + 3
    lda #$04    ; len(run\n)
    sta KEYBUF_NUM
    bne +++ ; bra
++ ; !else:
    lda #'s'
    sta KEYBUF
    lda #'y'
    sta KEYBUF + 1
    lda #'s'
    sta KEYBUF + 2

    ldx #4
-   lda P_SYS_NUMBERS,x
    sta KEYBUF + 3,x
    dex
    bpl -
    lda #$08
    sta KEYBUF_NUM

+++ ; !endif:

    ; update size (for faked start < 0)
    clc
    lda ZP_SIZE
    adc ZP_SRC
    sta ZP_SIZE
    bcc +
    inc ZP_SIZE+1
+

    ; lower source -> y ; copy always block-wise
    ; :sub16_8 ZP_DST ; ZP_SRC
    sec
    lda ZP_DST
    sbc ZP_SRC
    sta ZP_DST
    bcs +
    dec ZP_DST+1
+
    ldy ZP_SRC
    lda #0
    sta ZP_SRC

    ; :if ZP_SIZE+1 ; NE ; #$00 ; JMP ; COPY_FILE
    lda ZP_SIZE+1
    beq +
    jmp COPY_FILE
+   sty smc_limit+1
    jmp COPY_FILE_LESS_THEN_ONE_PAGE

    ; CART IS FILE (AND NO LONGER EASYLOADER)
    ; COPY THE REQUIRED PROG

loading_1:
!byte $91
!pet "loading "
!byte $22
loading_1_end:
loading_2:
!byte $22
!pet ",ef,1"
!byte $0d
!pet "ready."
!byte $0d, $0d
loading_2_end:


FILE_EXT1_START:
!pseudopc $df00 {
DO_PARTIAL_RESET:
    ;    PARTIAL RESET

    ; disable rom
    lda #MODE_RAM
    sta IO_MODE

    ; do a partial reset
    ldx #$ff
    txs
    ldx #$05
    stx $d016
    jsr $fda3
    jsr $fd50
    jsr $fd15
    jsr $ff5b

    ; enable rom
    lda #MODE_16k
    sta IO_MODE

    jmp BACK_FROM_PARTIAL_RESET

    ;    RESET, PART 2

GO_RESET:
    lda #MODE_RAM
    sta IO_MODE
    jmp $fcfe

    ; ONLY USED WITH C64GS KERNAL:
    ; RESTORE CHROUT VECTOR
    ; SET $302 VECTOR TO BASIC (INSTEAD OF ANIMATION LOOP)
    ; REQUIRES 27 BYTES OF RAM

CHROUT_TRAP:
    sei
    pha

SMC_RESTORE_CHROUT_LOWER:
    lda #$00
    sta $326
SMC_RESTORE_CHROUT_UPPER:
    lda #$00
    sta $327

    ; :mov16 #$a483 ; $0302
    lda #<$a483
    sta $0302
    lda #>$a483
    sta $0303

    pla
    cli
    jmp ($326)

; RESET IS DONE
; RESTORE VECTOR
; JUMP BACK IN CARTRIDGE

RESET_TRAP:
    ; restore A,X,Y
    sei
    pha
    txa
    pha
    tya
    pha

    ; restore_vector (by self-modifying-code)
SMC_RESTORE_LOWER:
    lda #$00
    sta $324
SMC_RESTORE_UPPER:
    lda #$00
    sta $325

    ; activate easyloader program
    lda #MODE_16k
    sta IO_MODE

    ; jump back to program
    jmp FILE_COPIER
; DATA
P_BOSLN:
!fill 25, 0
P_SYS_NUMBERS:
!fill 5, 0

;    for the file-copy
add_bank:
smc_start1_ofs1:
    lda #>$8000
    sta ZP_SRC+1
    inc ZP_BANK
COPY_FILE:
    lda ZP_BANK
    sta $de00
-   lda (ZP_SRC),y
    sta (ZP_DST),y
    iny
    bne -
    inc ZP_DST+1
    inc ZP_SRC+1
    dec ZP_SIZE+1
    beq +
smc_end1_ofs3:
    ; :if ZP_SRC+1 ; EQ ; #$c0 ; add_bank
    lda ZP_SRC+1
    cmp #>$c000
    beq add_bank
    jmp -
+
smc_end2_ofs3:
    ; :if ZP_SRC+1 ; EQ ; #$c0 ; ENDIF ; !endif+
    lda ZP_SRC+1
    cmp #>$c000
    bne +
smc_start2_ofs1:
    lda #>$8000
    sta ZP_SRC+1
    inc ZP_BANK
COPY_FILE_LESS_THEN_ONE_PAGE:
    lda ZP_BANK
    sta $de00
+ ;   !endif:
    ldy ZP_SIZE
    beq +
-   dey
    lda (ZP_SRC), y
    sta (ZP_DST), y
smc_limit:
    cpy #$00
    bne -
+
    ; setup end of program
    lda #<$0801
    sta $2b
    lda #>$0801
    sta $2c

    ; :add16_8 ZP_DST ; ZP_SIZE ; $2d
    clc
    lda ZP_DST
    adc ZP_SIZE
    sta $2d
    sta $2f
    sta $31
    sta $ae
    lda ZP_DST+1
    adc #0
    sta $2d+1
    sta $2f+1
    sta $31+1
    sta $ae+1

    ; DISABLE CART, RESTORE REGS, JUMP TO THE REAL CHRIN

    ; disable cart
    lda #MODE_RAM
    sta IO_MODE

    ; write $08 in $ba (last used drive)
    lda #$08
    sta $ba

    ; restore A,X,Y
    pla
    tay
    pla
    tax
    pla

    cli
    jmp ($324)
} ; !pseudopc
FILE_EXT1_END:


; --- "screen/colors.asm"

F_INIT_COLORS:
    lda #<(COLORRAM + 24*40)
    sta COL_DST_LO
    lda #>(COLORRAM + 24*40)
    sta COL_DST_LO+1

    lda #>col_pattern
    sta COL_SRC_HI
    ldx #24
--
    lda col_pattern,x
    sta COL_SRC_LO
    ldy #39
-   lda (COL_SRC_LO),y
    sta (COL_DST_LO),y
    dey
    bpl -

    ; :sub16_8 COL_DST_LO ; #40
    sec
    lda COL_DST_LO
    sbc #40
    sta COL_DST_LO
    bcs +
    dec COL_DST_LO+1
+
    dex
    bpl --
    rts

F_CLEAR_COLORS:     ; clear the colors in the wait-box
    lda #<(COLORRAM + 14*40)
    sta COL_DST_LO
    lda #>(COLORRAM + 14*40)
    sta COL_DST_LO+1

    lda #<col_line_help
    sta COL_SRC_LO
    lda #>col_line_help
    sta COL_SRC_LO+1

    ldx #5
--  ldy #39
-   lda (COL_SRC_LO),y
    sta (COL_DST_LO),y
    dey
    bpl -

    ; :sub16_8 COL_DST_LO ; #40
    sec
    lda COL_DST_LO
    sbc #40
    sta COL_DST_LO
    bcs +
    dec COL_DST_LO+1
+

    dex
    bpl --
    rts


; --- "screen/init.asm"

F_INIT_SCREEN:
!zone Init_screen {

.SRC = P_BINBCD_IN
.DST = P_BINBCD_OUT

    lda #<SCREEN_DATA
    sta .SRC
    lda #>SCREEN_DATA
    sta .SRC+1
    lda #<start_cols_screen_sprites
    sta .DST
    lda #>start_cols_screen_sprites
    sta .DST+1

    ldx #>(SCREEN_DATA_END - SCREEN_DATA + $ff)

    ldy #0
-   lda (.SRC),y
    sta (.DST),y
    iny
    bne -

    inc .SRC + 1
    inc .DST + 1

    dex
    bne -

    rts

} ; !zone Init_screen


; --- "ui/input.asm"

F_GETIN:
!zone Getin {
    jsr .get_shift
    beq .is_shift

    ; no shift
    lda #<tab1
    sta ZP_INPUT_KEYTABLE
    lda #>tab1
    sta ZP_INPUT_KEYTABLE+1

    bne +
    ; with shift
.is_shift:
    lda #<tab2
    sta ZP_INPUT_KEYTABLE
    lda #>tab2
    sta ZP_INPUT_KEYTABLE+1
+

    ; go through rows
    ldy #63 ; char in table
    lda #$7f
.row_loop:
    sta ZP_INPUT_MATRIX
    sta $dc00
    ; row-loop
-   lda $dc01
    cmp $dc01
    bne -
    ldx #7
    ; col-loop
.col_loop:
    asl
    bcc .a_char
.doch_nicht_a_char: ; FIXME
    dey
    bmi .no_char
    dex
    bpl .col_loop

    ; row-loop
    lda ZP_INPUT_MATRIX
    sec
    ror
    bne .row_loop ; branches always

.no_char:
    lda #$00
    rts

.ZP_BUFFER = P_BUFFER

.a_char:
    sta .ZP_BUFFER
    ; check whether shift is still the same
    jsr .get_shift
    beq +
    lda #<tab1 ; no shift
    bne ++
+   lda #<tab2 ; shift
++  cmp ZP_INPUT_KEYTABLE
    bne F_GETIN  ; if shift state is different: just restart

    lda (ZP_INPUT_KEYTABLE),y
    beq .doch_nicht_a_char_l
    rts

.doch_nicht_a_char_l:   ; FIXME
    lda .ZP_BUFFER
    bne .doch_nicht_a_char

.get_shift:
    lda #$bf
    sta $dc00
-   lda $dc01
    cmp $dc01
    bne -
    and #$10
    beq + ; !is_shift+
    lda #$fd
    sta $dc00
-   lda $dc01
    cmp $dc01
    bne -
    and #$80
+ ; !is_shift:
    rts
} ; !zone Getin

;                               Port B - $DC01
;              +-----+-----+-----+-----+-----+-----+-----+-----+
;              |Bit 7|Bit 6|Bit 5|Bit 4|Bit 3|Bit 2|Bit 1|Bit 0|
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 7| R/S |  Q  |  C= |SPACE|  2  | CTRL|A_LFT|  1  |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 6|  /  | A_UP|  =  | S_R | HOME|  ;  |  *  |POUND|
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 5|  ,  |  @  |  :  |  .  |  -  |  L  |  P  |  +  |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 4|  N  |  O  |  K  |  M  |  0  |  J  |  I  |  9  |
; Port A +-----+-----+-----+-----+-----+-----+-----+-----+-----+
; $DC00  |Bit 3|  V  |  U  |  H  |  B  |  8  |  G  |  Y  |  7  |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 2|  X  |  T  |  F  |  C  |  6  |  D  |  R  |  5  |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 1| S_L |  E  |  S  |  Z  |  4  |  A  |  W  |  3  |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+
;        |Bit 0|C_U/D|  F5 |  F3 |  F1 |  F7 |C_L/R|  CR | DEL |
;        +-----+-----+-----+-----+-----+-----+-----+-----+-----+

tab1:
!byte V_KEY_DEL, V_KEY_RETURN, V_KEY_CRIGHT, V_KEY_F7, V_KEY_F1, V_KEY_F3, V_KEY_F5, V_KEY_CDOWN
!byte '3', 'w', 'a', '4', 'z', 's', 'e', 0   ; no SHIFT
!byte '5', 'r', 'd', '6', 'c', 'f', 't', 'x'
!byte '7', 'y', 'g', '8', 'b', 'h', 'u', 'v'
!byte '9', 'i', 'j', '0', 'm', 'k', 'o', 'n'
!byte '+', 'p', 'l', '-', '.', ':', '@', ','
!byte $5c, '*', ';', V_KEY_HOME, 0, '=', '^', '/' ; no SHIFT
!byte '1', '_', V_KEY_CTRL,  $32, $20, V_KEY_COMD, 'q', V_KEY_STOP

tab2:
!byte V_KEY_INS, V_KEY_RETURN, V_KEY_CLEFT, V_KEY_F8, V_KEY_F2, V_KEY_F4, V_KEY_F6, V_KEY_CUP
    ; shifted RETURN = RETURN
!byte $23, $77, $61, $24, $7a, $73, $65, 0   ; no shifted SHIFT
!byte $25, $72, $64, $26, $63, $66, $74, $78
!byte $27, $79, $67, $28, $62, $68, $75, $76
!byte $29, $69, $6a, $30, $6d, $6b, $6f, $6e
!byte 0,   $70, $6c, 0,   $3e, $5b, 0,   $3c ; no shifted +,-,@
!byte 0,   0,   $5d, V_KEY_CLR,  0, 0,   0,   $3f ; no shifted POUND,*,SHIFT,=,A_UP
!byte $21, 0,   V_KEY_SCTRL, $22, $20, V_KEY_SCOMD, $71, V_KEY_RUN
    ; no shifted A_LEFT

F_INPUT_INIT:
    ; init
    lda #0
    sta ZP_INPUT_LAST_CHAR

    ; init CIA1-data direction
    ldx #$ff
    stx $dc02
    inx
    stx $dc03

    ; CIA2
    ; stop timer A
    lda #%11000000
    sta $dd0e
    ; stop timer B
    lda #%01000000
    sta $dd0f
    ; set latch for timer A
    lda #$08 ; 0808 -> 2056 cycles -> ~0.002 sec / ~2/1000 sec
    sta $dd05
    sta $dd04
    ; set upper latch for timer B to 0
    lda #$00
    sta $dd07
    ; start timer A
    lda #%11000001
    sta $dd0e

    rts

F_INPUT_GETJOY:
!zone Getjoy {
    lda #$7f
    sta $dc00
-   lda $dc00
    cmp $dc00
    bne -
    lsr
    bcc .up
    lsr
    bcc .down
    lsr
    bcc .left
    lsr
    bcc .right
    lsr
    bcc .fire
    lda #$00
    rts

.up:
    lda #V_KEY_CUP ; crsr up
    rts

.down:
    lda #V_KEY_CDOWN ; crsr down
    rts

.left:
    lda #V_KEY_CLEFT ; crsr left
    rts

.right:
    lda #V_KEY_CRIGHT ; crsr right
    rts

.fire:
    lda #V_KEY_RETURN ; return
    rts
} ; !zone Getjoy

F_INPUT_GETKEY:
!zone Getkey {
    ; get from joystick
    jsr F_INPUT_GETJOY
    bne +
    ; if no joystick: get from keyboard
    jsr F_GETIN
+
    ; process the key
    beq .no_char

    ; a key is pressed
    cmp ZP_INPUT_LAST_CHAR
    bne .use_key ; key is different to the last, get it

    lda $dd0f
    and #$01
    bne .show_no_char ; time is not yet done -> no key

    ; rep-loop is done -> emit a key and use another rep.time
    lda #48
    jsr .set_timer
    jmp .return_char

.use_key:
    sta ZP_INPUT_LAST_CHAR
    lda #200
    jsr .set_timer
.return_char:
    lda ZP_INPUT_LAST_CHAR
    rts

.no_char:
    sta ZP_INPUT_LAST_CHAR
    rts

.set_timer:
    sta $dd06
    lda #%01011001
    sta $dd0f
    rts

.show_no_char:
    lda #$00
.return:
    rts
} ; !zone Getkey


; --- "ui/last_config.asm"

P_LAST_CONFIG_ADDRESS = $dffc

F_LAST_CONFIG_READ:
!zone Last_config_read {

    ; scan stack-area for $00 or $ff -> less than 5 others do a fresh_start
    ldx #$00
    ldy #-5
-   lda $100,x
    beq +
    cmp #$ff
    beq +
    iny
    beq .next_step
+   inx
    bne -
    beq .fresh_start

.next_step:
    lda P_LAST_CONFIG_ADDRESS+0
    sta P_DRAW_START
    lda P_LAST_CONFIG_ADDRESS+1
    sta P_DRAW_OFFSET

    lda P_DRAW_START
    clc
    adc P_DRAW_OFFSET
    clc
    adc P_LAST_CONFIG_ADDRESS+2
    cmp #$65
    bne .fresh_start

    ; just belive we're right
    rts

.fresh_start:
    lda #0
    sta P_DRAW_START ; show first line
    sta P_DRAW_OFFSET ; first line is active

    jsr F_LAST_CONFIG_WRITE

    +copy_to_df00 copy_scan_boot_start, (copy_scan_boot_end - copy_scan_boot_start)
    jmp scan_boot ; does a rts it not found

copy_scan_boot_start:
!pseudopc $df00 {
.TEMP = $02
.end_scan:
    lda #EASYLOADER_BANK
    sta $de00
    rts

scan_boot:
    lda #EASYFILESYSTEM_BANK
    sta $de00
    lda #<($a000-V_EFS_SIZE)
    sta .TEMP
    lda #>($a000-V_EFS_SIZE)
    sta .TEMP+1

.big_loop:
    ; :add16_8 TEMP ; #V_EFS_SIZE
    clc
    lda .TEMP
    adc #V_EFS_SIZE
    sta .TEMP
    bcc +
    inc .TEMP+1
+

    ldy #O_EFS_TYPE
    lda (.TEMP),y
    and #O_EFST_MASK
    cmp #O_EFST_END
    beq .end_scan ; type = end of fs
    and #$10
    beq .big_loop ; not of type crt

    ldy #$00
    ; check a char
-   lda (.TEMP),y
    cmp boot_once,y
    bne .big_loop

    iny
    cpy #(boot_once_end - boot_once)
    bne -

.found_boot:
    ldy #O_EFS_TYPE
    lda (.TEMP),y
    and #$03
    tax
    lda type2mode_table,x
    tax
    iny
    lda (.TEMP),y
    sta $de00
    sta $df00
    lda #$00
    sta $df01
    stx $de02
    jmp ($fffc)

type2mode_table:
!byte MODE_8k
!byte MODE_16k
!byte MODE_ULT
!byte MODE_ULT

boot_once:
!pet "!el_boot-once", 0
boot_once_end:

} ; !pseudopc
copy_scan_boot_end:
} ; !zone Last_config_read

F_LAST_CONFIG_WRITE:
    lda #$65
    sec
    sbc P_DRAW_START
    sec
    sbc P_DRAW_OFFSET
    sta P_LAST_CONFIG_ADDRESS+2

    lda P_DRAW_START
    sta P_LAST_CONFIG_ADDRESS+0
    lda P_DRAW_OFFSET
    sta P_LAST_CONFIG_ADDRESS+1

    rts

; --- "ui/menu.asm"

F_MENU:
!zone Menu {
    jsr F_INPUT_INIT

main_loop:
    ; reset screensaver (after a keystroke)
    lda #$00
    sta P_SCREENSAVER_COUNTER+0
    sta P_SCREENSAVER_COUNTER+1
main_loop2:
    ; inc screensaver
    inc P_SCREENSAVER_COUNTER
    bne +
    inc P_SCREENSAVER_COUNTER+1
+
    ; :if P_SCREENSAVER_COUNTER+1 ; GE ; #180 ; JMP ; start_saver
    lda P_SCREENSAVER_COUNTER+1
    cmp #180
    bcc +
    jmp start_saver
+
    jsr F_INPUT_GETKEY
    beq main_loop2

    ; :if A ; EQ ; #V_KEY_F2 ; JMP ; F_BASIC
    cmp #V_KEY_F2
    bne +
    jmp F_BASIC
+

    ; if no enties -> don't move
    ldx P_NUM_DIR_ENTRIES
    beq main_loop

    ; :if A ; EQ ; #V_KEY_CUP ; move_up
    cmp #V_KEY_CUP
    beq move_up
    ; :if A ; EQ ; #V_KEY_CDOWN ; move_down
    cmp #V_KEY_CDOWN
    beq move_down
    ; :if A ; EQ ; #V_KEY_CLEFT ; JMP ; page_up
    cmp #V_KEY_CLEFT
    beq page_up
    ; :if A ; EQ ; #V_KEY_CRIGHT ; page_down
    cmp #V_KEY_CRIGHT
    beq page_down
    ; :if A ; EQ ; #V_KEY_RETURN ; JSR ; F_LAUNCH
    cmp #V_KEY_RETURN
    bne +
    jsr F_LAUNCH    ; may return
+
    ; :if A ; EQ ; #V_KEY_DEL ; JSR ; F_SEARCH_DEL
    cmp #V_KEY_DEL
    bne +
    jsr F_SEARCH_DEL
+
    ; :if A ; EQ ; #V_KEY_CLR ; JSR ; F_SEARCH_RESET
    cmp #V_KEY_CLR
    bne +
    jsr F_SEARCH_RESET
+

    ldx P_SEARCH_ACTIVE
    bne +++ ; !else2+

    ; search box is active.
    ; get almost all keys into search

    ; every printable char (except uppercase and control chars)
    ; :if A ; GE ; #$20 ; ENDIF ; !endif+
    cmp #' '
    bcc main_loop ; !endif
    ; :if A ; LE ; #$5a ; JSR ; F_SEARCH_KEY
    cmp #'z'+1
    bcs main_loop
    jsr F_SEARCH_KEY
    jmp main_loop

+++ ; !else2:
    ; search box is inactive.
    ; only 0..9 and a..z and / (prints no char) will trigger search

    ; key 0..9
    ; :if A ; GE ; #$30 ; ENDIF ; !endif+
    cmp #'0'
    bcc +
    ; :if A ; LE ; #$39 ; JSR ; F_SEARCH_KEY
    cmp #'9'+1
    bcs +
    jsr F_SEARCH_KEY
+   ; !endif:
    ; key a..z
    ; :if A ; GE ; #$41 ; ENDIF ; !endif+
    cmp #'a'
    bcc +
    ; :if A ; LE ; #$5a ; JSR ; F_SEARCH_KEY
    cmp #'z'+1
    bcs +
    jsr F_SEARCH_KEY
+   ; !endif:
    ; :if A ; EQ ; #$2f ; JSR ; F_SEARCH_START
    cmp #$2f
    bne +
    jsr F_SEARCH_START
+   ; :if A ; EQ ; #$3f ; JMP ; show_version
    cmp #'?'    ; shift+'/' -> '?'
    beq show_version
    ; !endif2:
    jmp main_loop

move_up:
    dec P_DRAW_OFFSET
    jmp draw_screen

move_down:
    inc P_DRAW_OFFSET
    jmp draw_screen

page_up:
    sec
    lda P_DRAW_OFFSET
    sbc #23
    sta P_DRAW_OFFSET
    jmp draw_screen

page_down:
    clc
    lda P_DRAW_OFFSET
    adc #23
    sta P_DRAW_OFFSET
;   jmp draw_screen

draw_screen:
    lda P_SEARCH_POS
    beq +
    jsr F_SEARCH_RESET
+   jsr F_DRAW
    jmp main_loop

;    version info

version_len = version_end - version

show_version:
    ldx #version_len-1
-   lda version,x
    sta SCREEN + 24*40 + 39-version_len,x
    dex
    bpl -

-   jsr F_GETIN
    cmp #$3f
    beq -

    ldx #version_len-1
    lda #$82
-   sta SCREEN + 24*40 + 39-version_len,x
    dex
    bpl -

    jmp main_loop

version:
!scr "ALX V1.3"
version_end:

;    screen saver

start_saver:
    ; copy boot-code
    +copy_to_df00 COPY_STARTSAVER_START, COPY_STARTSAVER_END - COPY_STARTSAVER_START
    ; do boot
    jmp $df00

COPY_STARTSAVER_START:
!pseudopc $df00 {
    ; load bank
    lda P_SCREENSAVER_BANK
    beq +++

    ; store bank (lower 8bit)
    sta $de00
    sta $df00

    ; store bank (higher 8bit)
    lda #$00
    sta $df01

    ; load and use offset
    lda P_SCREENSAVER_OFS
    sta smc_jsr+2
smc_jsr:
    jsr $0000 ; upper byte will be filled with P_SCREENSAVER_OFS

    lda #EASYLOADER_BANK
    sta $de00
+++
    jmp main_loop
} ; !pseudopc
COPY_STARTSAVER_END:


} ; !zone Menu

} ; !zone Subs


!zone Start {
F_START:
    ; init basics!!
    sei
    ldx #$ff
    txs
    cld
    stx $8004 ; no CBM80
    lda #$e7
    sta $01
    lda #$2f
    sta $00

    ; check for /^boot(.crt)?$/
    jsr F_LAST_CONFIG_READ

    ; init SID
    ldx #$19-1
    lda #$00
-   sta $d400,x
    dex
    bpl -

    ; init VIC2
    ldx #ini_d000_end-ini_d000-1
-   lda ini_d000,x
    sta $d000,x
    dex
    bpl -

    ; init CIA1/2
    ldx #ini_dc00_dd00_end-ini_dc00_dd00-1
-   lda ini_dc00_dd00,x
    sta $dc00,x
    sta $dd00,x
    dex
    bpl -

    jsr F_INIT_SCREEN

    ldx #0
-
!for .i, 8 {
    lda CUSTOM_CHARSET + (.i - 1) * $0100,x
    sta CHARSET_RAM + (.i - 1) * $0100,x
}
    inx
    bne -

    jsr F_INIT_COLORS

    lda #$9b
    sta $d011
    lda #$ff
    sta $d015

    lda #MODE_16k
    sta P_LED_STATE

    jsr F_SCAN_DIR

    jsr F_SEARCH_INIT

    jsr F_CLEAR_COLORS

    jsr F_DRAW

    jmp F_MENU
} ; !zone Start


!zone Init_data {
ini_d000:
; sprite data
.left_pos = $e9
.top_pos = $40

.spr0x = .left_pos + 0*24
.spr1x = .left_pos + 1*24
.spr2x = .left_pos + 2*24
.spr3x = .left_pos + 3*24
.spr4x = .left_pos + 5*8
.spr5x = .left_pos + 5*8
.spr6x = .left_pos + 5*8 - 4
.spr7x = .left_pos + 5*8 - 4

!byte <.spr0x, .top_pos
!byte <.spr1x, .top_pos
!byte <.spr2x, .top_pos
!byte <.spr3x, .top_pos
!byte <.spr4x, .top_pos
!byte <.spr5x, .top_pos
!byte <.spr6x, .top_pos+20*8
!byte <.spr7x, .top_pos+20*8

!byte ((>.spr7x) << 7) | ((>.spr6x) << 6) | ((>.spr5x) << 5) | ((>.spr4x) << 4) | ((>.spr3x) << 3) | ((>.spr2x) << 2) | ((>.spr1x) << 1) | (>.spr0x)
!byte $8b, $37, $00, $00, $00, $08, $00
!byte ((>(SCREEN & $3fff)) << 2) | ((>(CHARSET_RAM & $3800)) >> 2) | 1
!byte $71, $f0, $00, $00, $00, $00, $00
!byte $f0, $f0, $f0, $f0, $f0, $f0, $f0
!byte $fc, $fc, $fc, $fc, $f8, $f9, $f8, $f9
ini_d000_end:

ini_dc00_dd00:
!byte $ff, $ff, $00, $00, $ff, $ff, $ff, $ff, $00, $00, $00, $01, $00, $00, $00, $00
ini_dc00_dd00_end:
} ; !zone Init_data

!zone ColsScreenSprite {
SCREEN_DATA:
!bin "easyloader_cols.bin"
ORIG_SCREEN:
!bin "easyloader_screen.bin"
!bin "easyloader_sprites.bin"
SCREEN_DATA_END:
} ; !zone

!if SCREEN_DATA_END > EASYLOADER_STARTADDRESS + $1800 {
!error "screen data goes over font!"
} ; !if

* = EASYLOADER_STARTADDRESS + $1800
!zone Font {
CUSTOM_CHARSET:
!bin "easyloader_font.bin"
} ; !zone Font
