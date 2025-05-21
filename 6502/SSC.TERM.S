;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                               ;
; APPLE II SSC FIRMWARE         ;
;                               ;
;   BY LARRY KENYON             ;
;                               ;
;    -JANUARY 1981-             ;;;;;;;;;;;;;;
;                                            ;
; (C) COPYRIGHT 1981 BY APPLE COMPUTER, INC. ;
;                                            ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SHORT BLOCK MOVE              ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BATCHIO TXA
        ASL A
        ASL A
        ASL A
        ASL A
        STA SLOT16
        LDA #0
        STA STSBYTE,X   ; ZERO ERROR INDICATION
        BVS MOVIN

MOVOUT  LDY #0
        LDA (A1L),Y     ; GET BUFFER DATA
        STA CHARACTER
        JSR ACIAOUT     ; SEND IT OUT THE ACIA
        JSR NXTA1
        BCC MOVOUT
        RTS

MOVIN   JSR SRIN
        BCC MOVIN
        LDA RDREG,Y
        LDY #0
        STA (A1L),Y     ; PUT ACIA DATA INTO BUFFER
        JSR NXTA1
        BCC MOVIN
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;
;                        ;
; TERMINAL MODE ROUTINES ;
;                        ;
;;;;;;;;;;;;;;;;;;;;;;;;;;
CHECKTERM LDA STATEFLG,X ; HAVE WE ENTERED TERMINAL MODE?
        BPL TERMRTS     ; IF NOT, A SIMPLE RTS WILL DO. . .
;
; WE ENTER THE WORLD OF TERMINAL MODE
;
TERMMODE LDA #$02       ; START IN SHIFT-LOCK STATE
        PHA             ; SHIFT STATE IS SAVED ON STACK
        LDA #$7F
        JSR KCMD1       ; RESET ECHO (DEFAULT TO FULL DUP)

TERMNEXT LDY CH
        LDA (BASL),Y
        STA CHARACTER   ; SAVE SCREEN CHARACTER
TERMNEXT1 LDA #$07      ; IMPLEMENT A FLASHING UNDERLINE
        AND RNDH        ;  FOR A CURSOR
        BNE TERMNEXT3
        LDY CH
        LDA #$DF
        CMP (BASL),Y    ; IS UNDERLINE ON THE SCREEN?
        BNE TERMNEXT2   ; IF NOT, PUT IT THERE
        LDA CHARACTER   ; OTHERWISE USE TRUE SCREEN CHAR
TERMNEXT2 STA (BASL),Y
        INC RNDH        ; MAKE IT FLASH, BUT
        INC RNDH        ; NOT TOO SLOW AND NOT TOO FAST

TERMNEXT3 LDA STATEFLG,X ; ARE WE STILL IN TERM MODE?
        BMI TERMACIAIN  ; IF SO, GO CHECK ACIA

TERMEXIT JSR RESTORE    ; ALWAYS REPLACE OUR CURSOR
        PLA             ; CLEAN UP THE STACK
        LDA #$8D        ; RETURN A <CR> TO COVER UP
        STA CHARACTER
TERMRTS RTS

TERMACIAIN JSR INPUT    ; ACIA INPUT?
        BCC TERMKBDIN   ; IF NOT, GO CHECK KEYBOARD
        JSR RESTORE     ; RESTORE CURSOR, INPUT->CHARACTER
        JSR CKINPUT     ; CHECK FOR CTL-T, CTL-R
        JSR SCREENOUT1  ; INPUT->SCREEN ALWAYS
        JMP TERMNEXT

TERMKBDIN JSR GETKBD    ; KEYPRESS?
        BCC TERMNEXT1   ; SKIP IF NOT
        BVS TERMNEXT    ; BRANCH IF WE DID A KBD ESCAPE SEQ.
        LDA MISCFLG,X   ; SHIFTING ENABLED?
        ASL A
        BPL TERMSEND1
        PLA             ; RECOVER TERMSTATE
        TAY
        LDA CHARACTER
        CPY #1          ; 1 = SHIFT LETTERS, XLATE NUMBERS
        BEQ TERMCAP
        BCS TERMLOCK    ; 2 MEANS CAPS LOCK MODE

TERMNORM CMP #$9B       ; ESC?
        BNE TERMLETTER

TERMINC INY             ; INCREMENT STATE
TERMINC1 TYA
        PHA             ; PUT BACK ON STACK
        JMP TERMNEXT

TERMLETTER CMP #$C1     ; <A?
        BCC TERMSEND
        CMP #$DB        ; >Z?
        BCS TERMSEND
        ORA #$20        ; IT'S A LETTER SO TRANSLATE TO LC
        STA CHARACTER

TERMSEND TYA
        PHA             ; PUT STATE BACK ON STACK
TERMSEND1 JSR OUTPUT1   ; GO OUTPUT
        JMP TERMNEXT

TERMCAP CMP #$9B        ; TWO ESCAPES?
        BEQ TERMINC
        CMP #$B0        ; <0?
        BCC TERMCAP1
        CMP #$BB        ; >COLON?
        BCS TERMCAP1
;
; ESC <NUMBER> SO TRANSLATE INTO MISSING ASCII CHAR
;
        TAY
        LDA TRANSLATE-$B0,Y
        STA CHARACTER
TERMCAP1 LDY #0         ; BACK TO STATE 0
        BEQ TERMSEND    ; <ALWAYS>

TERMLOCK CMP #$9B       ; ESC?
        BNE TERMSEND
        LDY #0
        BEQ TERMINC1    ; <ALWAYS>

;;;;;;;;;;;;;;;;;;;
; TRANSLATE TABLE ;
;;;;;;;;;;;;;;;;;;;
TRANSLATE DFB $9B       ; ESC
        DFB $9C         ; FS
        DFB $9F         ; US
        DFB $DB         ; LEFT BRACKET
        DFB $DC         ; LEFT SLASH
        DFB $DF         ; UNDERSCORE
        DFB $FB         ; LEFT ENCLOSE
        DFB $FC         ; VERTICAL BAR
        DFB $FD         ; RIGHT ENCLOSE
        DFB $FE         ; TILDE
        DFB $FF         ; RUB
