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
; CIC, SIC, PPC MODE HIGH-LEVEL ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CIC EXIT ROUTINE . . .        ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CICEXIT JSR CHECKTERM   ; SEE IF WE'VE ENTERED TERMINAL MODE
;;;;;;;;;;;;;;;;;;;;;;
; BASIC EXIT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;
BASICEXIT PLA
        TAY
        PLA
        TAX
        LDA CHARACTER
        RTS
;;;;;;;;;;;;;;;;;;;;;;;
; BASIC INPUT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;
BINPUT  BEQ BINACIA     ; BRANCH IF NOT CIC MODE
        LDA BUFBYTE,X   ; INPUT BUFFER FULL?
        BPL BINKBD
        LSR BUFBYTE,X   ; RESET BUFFER FULL
        BNE BINACIA1    ; <ALWAYS>

BINKBD  JSR GETKBD      ; KEYBOARD DATA?
        BCC BINACIA

BINEND  LDA DELAYFLG,X
        AND #$C0        ; TRANSLATE LOWERCASE TO UPPERCASE?
        BEQ BINEND1     ; IF SO, LET THE MONITOR DO IT
        LDA CHARACTER   ; IF NOT, SET FLAG IF
        CMP #$E0        ;  THIS IS A LOWERCASE CHAR
        BCC BINEND1     ;  FOR INPUT BUFFER CORRECTION
        LDA STATEFLG,X  ;  (CIRCUMVENT APPLE MONITOR)
        ORA #$40
        STA STATEFLG,X

BINEND1 PLP
        BEQ BASICEXIT   ; BRANCH IF NOT CIC MODE
        BNE CICEXIT     ; <ALWAYS> CHECK TO SEE IF WE
                        ;  ENTERED TERM MODE (VIA KYBD ESCAPE)
BINACIA JSR INPUT       ; ACIA DATA?
        BCC BINKBD
BINACIA1 JSR RESTORE    ; DO BASIC CURSED DUTY
        PLP
        PHP             ; GET CIC MODE INDICATOR
        BEQ BINEND      ; SKIP IF NOT CIC MODE
        JSR CKINPUT     ; LOOK FOR INPUT STREAM SPECIAL CHARS
        JMP BINEND
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SIC, PPC BASIC OUTPUT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SEROUT  JSR CMDSEQCK    ; CHECK FOR A COMMAND SEQUENCE
        BCS BASICEXIT   ; BRANCH IF WE WERE IN COMMAND MODE
        LDA CHARACTER   ; SAVE CHAR ON STACK
        PHA
        LDA MISCFLG,X   ; IF VIDEO OR TABBING ENABLED,
        AND #$C0        ;  DON'T MESS WITH THE CURSOR
        BNE TABCHECK

        LDA CH          ; CHECK FOR COMMA TABBING
        BEQ NOTAB       ; IF CH=0, THERE WAS NO TAB OR COMMA
        CMP #8          ; INTEGER BASIC COMMA?
        BEQ COMMA
        CMP #16         ; APPLESOFT COMMA?
        BNE TABCHECK
COMMA   ORA #$F0
        AND COLBYTE,X   ; SET COL TO PREVIOUS TAB
        CLC
        ADC CH          ; THEN INCREMENT TO NEXT TAB
        STA CH

TABCHECK LDA COLBYTE,X
        CMP CH          ; IS TABBING NEEDED?
        BEQ NOTAB       ; IF EQUAL THEN NO TAB NEEDED
        LDA #$A0        ; SPACE FOR FORWARD TAB
        BCC TAB1
        LDA MISCFLG,X   ; DON'T BACKSPACE UNLESS TABBING
        ASL A           ;  OPTION IS ENABLED
        BPL NOTAB
        LDA #$88        ; BACKSPACE FOR BACKTAB
TAB1    STA CHARACTER
        BIT IORTS       ; SET V=1 TO INDICATE TABBING
        PHP             ; SAVE TABBING INDICATOR
        BVS TAB2        ; <ALWAYS> AROUND BATCH MOVE ENTRY
        NOP
;;;;;;;;;;;;;;;;;;;;;;;;
; SHORT BATCH MOVE:    ;
;  LOCATE AT $C93D FOR ;
;  COMPATIBILITY WITH  ;
;  SIC P8 BLOCK MOVE.  ;
;;;;;;;;;;;;;;;;;;;;;;;;
BATCHIN BIT IORTS
        DFB $50         ; DUMMY BVC
BATCHOUT CLV            ; V=0 FOR OUTPUT ENTRY
        LDX MSLOT
        JMP BATCHIO
;;;;;;;;;;;;;;
; BURP . . . ;
;;;;;;;;;;;;;;
TAB2    JSR ADJUST      ; ADJUST COLUMN COUNT
        JSR OUTPUT2     ; DON'T GO TO SCREEN WHEN TABBING
        JMP FORCECR     ; SHARE SOME CODE. . .

NOTAB   PLA
        CLV
        PHP             ; SAVE 'NO TAB' INDICATION
NOTAB1  STA CHARACTER   ; (FORCE CR REENTRY)
        PHA
        JSR OUTPUT1     ; ENTER AFTER CMD SEQ CHECK
        JSR ADJUST
        PLA
        EOR #$8D        ; WAS IT A CR?
        ASL A
        BNE FORCECR
        STA COLBYTE,X   ; IF SO, RESET COLUMN TO 0
        STA CH

FORCECR LDA STATEFLG,X  ; FORCE CR DISABLED?
        BPL SEREND
        LDA PWDBYTE,X   ; FORCE CR IF LIMIT REACHED
        BEQ SEREND      ; (FOR P8 POKE COMPATIBILITY)
        CLC
        SBC COLBYTE,X
        LDA #$8D
        BCC NOTAB1      ; BRANCH TO FORCE CR

SEREND  PLP
        BVS TABCHECK    ; BRANCH IF TABBING

        LDA MISCFLG,X   ; DON'T MESS WITH CURSOR
        BMI SEREND2     ;  WHEN VIDEO IS ON
        LDY COLBYTE,X
        ASL A
        BMI SETCH       ; SET CH TO VALUE OF COL FOR TABBING
        TYA
        LDY #0
        SEC
        SBC PWDBYTE,X
        CMP #$F8        ; WITHIN 8 CHARS OF PWIDTH?
        BCC SETCH
        ADC #$27        ; IF SO, ADJUST TO WITHIN 8 OF 40
        TAY
SETCH   STY CH

SEREND2 JMP BASICEXIT   ; THAT'S ALL

;;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL ENTRY ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;
PENTRY  STX MSLOT
        STY SLOT16
        LDA #0
        STA STSBYTE,X
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SIC MODE PRINTER WIDTH TABLE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PWDTBL  DFB $29         ; 40 COLUMNS
        DFB $48         ; 72 COLUMNS
        DFB $50         ; 80 COLUMNS
        DFB $84         ; 132 COLUMNS
;;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL WRITE ROUTINE ;
; (DOUBLES AS PASCAL   ;
;  1.0 ENTRY POINT)    ;
; -MUST BE AT SC9AA-   ;
;;;;;;;;;;;;;;;;;;;;;;;;
PASCALWRITE STA CHARACTER
        JSR PENTRY
        JSR OUTPUT
        JMP PASEXIT     ; LOAD X-REG WITH ERROR BYTE & RTS

;;;;;;;;;;;;;;;;;;;;;;;;;
; COLUMN ADJUST ROUTINE ;
; (PPC, SIC MODES ONLY) ;
;;;;;;;;;;;;;;;;;;;;;;;;;
ADJUST  LDA CHARACTER
        EOR #$08        ; BACKSPACE?
        ASL A
        BEQ DECRCOL     ; IF SO, DECREMENT COLUMN
        EOR #$EE        ; DELETE? ($FF, RUB)
        BNE CTRLTST
DECRCOL DEC COLBYTE,X   ; DECREMENT COLUMN COUNT
        BPL ADJRTS
        STA COLBYTE,X   ; DON'T ALLOW TO GO BELOW 0
ADJRTS  RTS
CTRLTST CMP #$C0        ; DON'T INCREMENT COLUMN COUNT FOR
        BCS ADJRTS      ;  CONTROL CHARACTERS
        INC COLBYTE,X
        RTS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ROUTINE TO PROCESS SPECIAL INPUT CHARS ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CKINPUT LDA MISCFLG,X
        AND #$08        ; INPUT CTL CHARS ENABLED?
        BEQ CIEND

        LDA STATEFLG,X
        LDY CHARACTER
        CPY #$94        ; CTL-T?
        BNE CKINPUT1
        ORA #$80        ; SET TERMINAL MODE
        BNE CKINPUT2    ; <ALWAYS>

CKINPUT1 CPY #$92       ; CONTROL-R?
        BNE CIEND
        AND #$7F        ; RESET TERMINAL MODE
CKINPUT2 STA STATEFLG,X
CIEND   RTS
