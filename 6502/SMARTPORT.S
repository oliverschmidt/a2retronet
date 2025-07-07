;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                                ;
; MIT License                                                                    ;
;                                                                                ;
; Copyright (c) 2024 Oliver Schmidt (https://a2retro.de/)                        ;
;                                                                                ;
; Permission is hereby granted, free of charge, to any person obtaining a copy   ;
; of this software and associated documentation files (the "Software"), to deal  ;
; in the Software without restriction, including without limitation the rights   ;
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      ;
; copies of the Software, and to permit persons to whom the Software is          ;
; furnished to do so, subject to the following conditions:                       ;
;                                                                                ;
; The above copyright notice and this permission notice shall be included in all ;
; copies or substantial portions of the Software.                                ;
;                                                                                ;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     ;
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       ;
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    ;
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         ;
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  ;
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  ;
; SOFTWARE.                                                                      ;
;                                                                                ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                               ;
; C800 SPACE                    ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG $C800

        DFB $00         ; B/C OF RTS OFFSET

;;;;;;;;;;;;;;;;;;;
; PRODOS COMMANDS ;
;;;;;;;;;;;;;;;;;;;
PDSTS   ; EXECUTE AND GET RETURN CODE
        JSR RETCODE
        BNE PDDONE      ; ERROR?

        LDX DATA        ; SIZE LO
        LDY DATA        ; SIZE HI
PDOKAY  LDA #$00        ; SUCCESS
PDDONE  CMP #$01        ; SUCCESS?
        JMP QUIT

PDRD    ; EXECUTE AND GET RETURN CODE
        JSR RETCODE
        BNE PDDONE      ; ERROR?

        ; READ BLOCK
        JSR RDBLOCK
        BEQ PDOKAY      ; ALWAYS

PDWR    ; WRITE BLOCK
        JSR WRBLOCK

        ; EXECUTE AND GET RETURN CODE
        JSR RETCODE
        CMP #$01        ; SUCCESS?
        JMP QUIT

;;;;;;;;;;;;;;;;;;;;;;
; SMARTPORT COMMANDS ;
;;;;;;;;;;;;;;;;;;;;;;
STATUS  ; EXECUTE AND GET STATUS
        JSR GETSTS
        BNE RETURN      ; ERROR?

        ; READ STATUS LIST SIZE
        LDA DATA
        STA SIZEL
        LDA DATA
        STA SIZEH

        ; READ STATUS LIST
        LDY #$02        ; OFFSET OF STATUS LIST ADDRESS
        JSR SETADDR
        JSR RDBUF
        LDX SIZEL
        LDY SIZEH
        JMP RETURN

READBL  ; EXECUTE AND GET STATUS
        JSR GETSTS
        BNE RETURN      ; ERROR?

        ; READ BLOCK
        LDY #$02        ; OFFSET OF BUFFER ADDRESS
        JSR SETADDR
        JSR RDBLOCK
        JMP RETURN

WRITEBL ; WRITE BLOCK
        LDY #$02        ; OFFSET OF BUFFER ADDRESS
        JSR SETADDR
        JSR WRBLOCK
        ; FALL THROUGH

FORMAT  ; EXECUTE AND GET STATUS
        JSR GETSTS
        JMP RETURN

CONTROL ; WRITE CONTROL LIST
        LDY #$02        ; OFFSET OF CONTROL LIST ADDRSS
        JSR SETADDR
        LDY #$00        ; OFFSET OF CONTROL LIST SIZE
        JSR SETSIZE
        LDX #$02        ; SIZE OF CONTROL LIST SIZE
        JSR INCADDR
        JSR WRBUF
        ; FALL THROUGH

INIT    ; FALL THROUGH

OPEN    ; FALL THROUGH

CLOSE   ; EXECUTE AND GET STATUS
        JSR GETSTS
        JMP RETURN

READ    ; EXECUTE AND GET STATUS
        JSR GETSTS
        BNE RETURN      ; ERROR?

        ; READ BUFFER
        LDY #$04        ; OFFSET OF BYTE COUNT
        JSR SETSIZE
        LDY #$02        ; OFFSET OF BUFFER ADDRESS
        JSR SETADDR
        JSR RDBUF
        JMP RETURN

WRITE   ; WRITE BUFFER
        LDY #$04        ; OFFSET OF BYTE COUNT
        JSR SETSIZE
        LDY #$02        ; OFFSET OF BUFFER ADDRESS
        JSR SETADDR
        JSR WRBUF

        ; EXECUTE AND GET STATUS
        JSR GETSTS
        ; FALL THROUGH

RETURN  ; RESTORE ZEROPAGE LOCATIONS
        PLA
        STA ADDRL
        PLA
        STA ADDRH
        PLA
        STA SIZEL
        PLA
        STA SIZEH
        PLA             ; STATUS
        CMP #$01        ; SET CARRY
        JMP QUIT

;;;;;;;;;;;;;;;;;;
; HANDLER SWITCH ;
;;;;;;;;;;;;;;;;;;
SWITCH  BCC PRODOS
        JMP SMARTPORT

;;;;;;;;;;;;;;;;;;
; PRODOS HANDLER ;
;;;;;;;;;;;;;;;;;;
PRODOS: ; WRITE PARAMETERS
        LDY CMD
        STY DATA
        LDA UNIT
        STA DATA
        LDA BLOCKL
        STA DATA
        LDA BLOCKH
        STA DATA

        ; DISPATCH COMMAND
        LDA #>$C800
        PHA
        LDA PDCMD,Y
        PHA
        RTS

PDCMD   DFB <PDSTS-1, <PDRD-1, <PDWR-1

;;;;;;;;;;;;;;;;;;;;;
; SMARTPORT HANDLER ;
;;;;;;;;;;;;;;;;;;;;;
SMARTPORT:
        ; SAVE ZEROPAGE LOCATIONS AND RESERVE STATUS SLOT
        TSX
        LDY #$04
SAVE    LDA ADDRL,Y
        PHA
        DEY
        BPL SAVE

        ; GET ADDRESS OF PARAMETERS AND ADJUST STACK
        CLC
        INX
        LDA $0100,X
        STA ADDRL
        ADC #<$0003     ; SIZE OF PARAMETERS
        STA $0100,X 
        INX
        LDA $0100,X
        STA ADDRH
        ADC #>$0003     ; SIZE OF PARAMETERS
        STA $0100,X

        ; PREPARE COMMAND DISPATCH
        LDA #>$C800
        PHA
        LDY #$01+$00    ; PUSH OFFSET + OFFSET OF COMMAND NUMBER
        LDA (ADDRL),Y
        TAX
        LDA SPCMD,X
        PHA

        ; WRITE COMMAND NUMBER
        STX DATA

        ; GET COMMAND LIST ADDRESS
        LDY #$01+$01    ; PUSH OFFSET + OFFSET OF COMMAND LIST ADDRESS
        JSR SETADDR

        ; WRITE COMMAND LIST
        LDA #<$0009     ; SIZE OF LARGEST COMMAND LIST
        STA SIZEL
        LDA #>$0009     ; SIZE OF LARGEST COMMAND LIST
        STA SIZEH
        JSR WRBUF

        ; DISPATCH COMMAND
        RTS

SPCMD   DFB <STATUS-1, <READBL-1, <WRITEBL-1, <FORMAT-1, <CONTROL-1
        DFB <INIT  -1, <OPEN  -1, <CLOSE  -1, <READ  -1, <WRITE  -1

;;;;;;;;;
; UTILS ;
;;;;;;;;;
SETSIZE ; SET 16-BIT BUFFER SIZE
        LDA (ADDRL),Y
        STA SIZEL
        INY
        LDA (ADDRL),Y
        STA SIZEH
        RTS

SETADDR ; SET BUFFER ADDRESS
        LDA (ADDRL),Y
        TAX
        INY
        LDA (ADDRL),Y
        STA ADDRH
        STX ADDRL
        RTS

INCADDR ; INCREMENT BUFFER ADDRESS
        CLC
        LDA ADDRL
        ADC ZERO,X
        STA ADDRL
        BCC NOINC
        INC ADDRH
NOINC   RTS

ZERO    DFB $00, $01, $02

GETSTS  ; EXECUTE SMARTPORT COMMAND AND GET STATUS
        LDA #$02        ; SMARTPORT
        JSR WRCMD

        ; SAVE STATUS IN STACK SLOT
        TSX
        TAY
        TXA
        CLC
        ADC #$07        ; OFFSET OF STACK SLOT
        TAX
        TYA
        STA $0100,X
        RTS

RETCODE ; EXECUTE PRODOS COMMAND AND GET RETURN CODE
        LDA #$01        ; PRODOS
        ; FALL THROUGH

WRCMD   ; WRITE COMMAND REG AND READ RESPONSE
        STA CTRL
WAIT    LDA CTRL
        BPL WAIT
        LDA DATA
        RTS

RDBUF   ; READ BUFFER
        LDY #$00
        LDX SIZEH
        BEQ RDREST
RDFULL  JSR RDPAGE
        INC ADDRH
        DEX
        BNE RDFULL
RDREST  LDX SIZEL
        BEQ RDDONE
RDMORE  LDA DATA
        STA (ADDRL),Y
        INY
        DEX
        BNE RDMORE
RDDONE  RTS

WRBUF   ; WRITE BUFFER
        LDY #$00
        LDX SIZEH
        BEQ WRREST
WRFULL  JSR WRPAGE
        INC ADDRH
        DEX
        BNE WRFULL
WRREST  LDX SIZEL
        BEQ WRDONE
WRMORE  LDA (ADDRL),Y
        STA DATA
        INY
        DEX
        BNE WRMORE
WRDONE  RTS

RDBLOCK ; READ 512-BYTE BLOCK
        LDY #$00
        JSR RDPAGE
        INC ADDRH
RDPAGE  LDA DATA
        STA (ADDRL),Y
        INY
        BNE RDPAGE
        RTS

WRBLOCK ; WRITE 512-BYTE BLOCK
        LDY #$00
        JSR WRPAGE
        INC ADDRH
WRPAGE  LDA (ADDRL),Y
        STA DATA
        INY
        BNE WRPAGE
        RTS

;;;;;;;;;;;;;;;;;;;;
; CONFIG KEY CHECK ;
;;;;;;;;;;;;;;;;;;;;
CHECK   LDX #$FF
RETRY   LDA KBD
        BMI HAVEKEY
        INX
        STX DATA
        LDA #$40        ; CONFIG
        JSR WRCMD
        BNE DONE
        LDA DATA
        STA $07F7
        BNE RETRY       ; ALWAYS
HAVEKEY STA KBDSTRB
        CMP #'C'+$80
        BEQ DONE
        CMP #'c'+$80 
DONE    RTS

;;;;;;;;;;;;;;;;;;
; CONFIG HANDLER ;
;;;;;;;;;;;;;;;;;;
CFGHDLR STA $C051       ; TEXT
        STA $C054       ; PAGE 1
        LDA $FBB3
        CMP #$38        ; APPLE ][
        BEQ UPPER
        CMP #$EA        ; APPLE ][+
        BEQ UPPER
        LDA $FBC0
        CMP #$EA        ; APPLE //e
        BEQ UPPER
        LDA $C01E       ; CHARSET?
        PHA
        STA $C00F       ; ALT CHARSET
        LDA $C01F       ; COLS?
        PHA
        STA $C00C       ; 40 COLS
        LDA #$65        ; LOWERCASE
        PHP
        BNE LOOP        ; ALWAYS
UPPER   PHP
        LDA #$64        ; UPPERCASE

LOOP    STA DATA
        LDA #$40        ; CONFIG
        JSR WRCMD
        BNE SCRN        ; QUIT?

        PLP
        BEQ NOCHAR
        PLA             ; COLS
        BPL NOCOLS      ; 40 COLS?
        STA $C00D       ; 80 COLS
NOCOLS  PLA             ; CHARSET
        BMI NOCHAR      ; ALT CHARSET?
        STA $C00E       ; PRI CHARSET
NOCHAR  JSR HOME
        RTS

SCRN    LDX #$00
LINE    TXA
        JSR BASCALC
        LDY #$00
CHAR    LDA DATA
        STA (BASL),Y
        INY
        CPY #$28        ; COLS
        BCC CHAR 
        INX
        CPX #$18        ; ROWS
        BCC LINE

GETKEY  LDA KBD
        BPL GETKEY
        STA KBDSTRB
        BMI LOOP

        .RES $CF00-*
