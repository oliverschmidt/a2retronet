.macpack    apple2
.feature    c_comments

/*

MIT License

Copyright (c) 2025 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

        sta     $CFFF
        lda     #<$C8F6
        sta     $80
        lda     #>$C8F6
        sta     $81
        ldx     #$08

slot:   dec     $81
        dex
        beq     quit

        ldy     #$03
byte:   lda     ($80),y
        cmp     olsc,y
        bne     slot
        dey
        bpl     byte

.ifdef  hide
        sta     $CFFD
.else
        sta     $CFFE
.endif

quit:
.ifdef  init
        rts
.else    
        jsr     $BF00           ; MLI call entry point
        .byte   $65             ; Quit
        .word   param
param:  .byte   $04             ; param_count
        .byte   $00             ; quit_type
        .word   $0000           ; reserved
        .byte   $00             ; reserved
        .word   $0000           ; reserved
.endif

olsc:   scrcode "OlSc"
