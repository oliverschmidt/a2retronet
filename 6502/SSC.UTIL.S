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
;                               ;
; UTILITY ROUTINES              ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL-BASIC KEYBOARD FETCH ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CKKBD   CLC             ; RETURN CARRY CLEAR FOR NO DATA
        LDA MISCFLG,X
        AND #$04        ; ANSWER NO IF KEYBOARD IS DISABLED
        BEQ CKKBDXIT

CKKBD1  LDA KBD
        BPL CKKBDXIT
        STA KBDSTRB
        SEC             ; INDICATE DATA
CKKBDXIT RTS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GET A CHAR FROM KEYBOARD FOR BASIC ONLY ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETKBD  INC RNDL        ; MIX UP RANDOM # SEED
        BNE GETKBD1     ;  FOR BASIC
        INC RNDH
GETKBD1 JSR CKKBD       ; KEYBOARD FETCH ROUTINE
        CLV             ; INDICATE NO ESCAPE SEQUENCE
        BCC CKKBDXIT    ; EXIT IF NO KEY PRESS
        JSR RESTORE     ; DO BASIC CURSED DUTY
        AND #$7F
        CMP CMDBYTE,X   ; IS IT THE START OF A COMMAND?
        BNE GETKBDONE   ; IF NOT, EXIT INDICATING DATA
        LDY SLOT16
        LDA DIPSW1,Y    ; ONLY DO CMD ESC FOR PPC, SIC MODES
        LSR A
        BCS GETKBDONE
;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KEYBOARD ESCAPE HANDLER ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
KBDESC  LDY #$A         ; FIRST PRINT A PROMPT
PROMPTLOOP LDA PROMPTBL,Y
        STA CHARACTER
        TYA
        PHA
        JSR SCREENOUT1  ; ALWAYS SEND TO SCREEN
        PLA
        TAY
        DEY
        BPL PROMPTLOOP

        LDA #1          ; START OUT IN COMMAND STATE 1
        JSR SETOSTATE

GETCMD  JSR CKKBD1      ; WAIT FOR KEYBOARD CHARACTER
        BPL GETCMD
        CMP #$88        ; BACKSPACE?
        BEQ KBDESC      ; IF SO, THEN START OVER
        STA CHARACTER

        JSR SCREENOUT1
        JSR CMDSEQCK    ; PUMP THRU CMD INTERPRETER

        LDA STATEFLG,X  ; ARE WE DONE?
        AND #$07
        BNE GETCMD      ; IF NOT, GO AGAIN

        LDA #$8D        ; FORCE BACK A CARRIAGE RETURN
        STA CHARACTER
        BIT IORTS       ; INDICATE THAT A CMD SEQ HAS OCCURRED
GETKBDONE SEC           ; INDICATE SUCCESS
        RTS

PROMPTBL ASC ":CSS ELPPA"
        DFB $8D

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ROUTINE TO PRINT A CHARACTER ON THE CURRENT DISPLAY ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SCREENOUT LDA MISCFLG,X
        BPL NOOUT       ; IF SCREEN DISABLED

SCREENOUT1 LDA MISCFLG,X ; ENTRY AFTER ECHO CHECK
        AND #$02        ; IF IT ISN'T CIC MODE,
        BEQ ASCREEN     ; ALWAYS USE THE APPLE SCREEN
        LDA STATEFLG,X  ; CURRENT SCREEN = APPLE SCREEN?
        AND #$38
        BEQ ASCREEN     ; SLOT O= APPLE SCREEN

        TXA             ; JUMP TO CN00 SPACE
        PHA
        LDA #<SENDCD-1  ;  TO VECTOR TO THE PERIPHERAL
        PHA             ;  IN THE CHAIN SLOT
NOOUT   RTS
;
; APPLE 40-COL SCREEN DRIVER
;
ASCREEN JSR GETXLATE    ; GET THE TRANSLATE OPTIONS
        ORA #$80        ; SET HIGH BIT OF CHAR
        CMP #$E0        ; LOWERCASE?
        BCC TESTLETTER
        EOR LCMASK,Y    ; DO LOWERCASE TRIP
TOSCREEN JMP VIDOUT     ; ALL REGS ARE PRESERVED
;
; IF UPPERCASE, WE ONLY MAP LETTERS
;
TESTLETTER CMP #$C1     ; <A?
        BCC TOSCREEN
        CMP #$DB        ; >Z?
        BCS TOSCREEN
        EOR UCMASK,Y
        BCC TOSCREEN    ; <ALWAYS>

; MASKS FOR CASE TRANSLATION
LCMASK  DFB $20,$00,$E0,$20
UCMASK  DFB $00,$00,$00,$C0
REVMASK DFB $00,$00,$E0,$C0

GETXLATE LDA DELAYFLG,X ; TRANSLATE OPTIONS IN B6-B7
        ROL A
        ROL A
        ROL A
        AND #$03
        TAY
        LDA CHARACTER
        RTS
