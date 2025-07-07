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
; CF00 SPACE                    ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG $CF00

;;;;;;;;;;;;;;;;;;
; COMMON HANDLER ;
;;;;;;;;;;;;;;;;;;
COMMON  STA BANKSP
        JMP SWITCH

;;;;;;;;;;;;;;;
; COMMON QUIT ;
;;;;;;;;;;;;;;;
QUIT    STA BANKSSC
        RTS

;;;;;;;;;;
; CONFIG ;
;;;;;;;;;;
CFG     STA BANKSP
        JSR CFGHDLR
        STA BANKSSC
        RTS

;;;;;;;;;;;;;;;;
; BOOT HANDLER ;
;;;;;;;;;;;;;;;;
BOOT    STA BANKSP
        JSR CHECK
        STA BANKSSC
        BNE DOIT
        JSR CFG

DOIT    LDX #$01
        STX CMD
        LDA MSLOT
        ASL
        ASL
        ASL
        ASL
        STA UNIT        ; DRIVE 1
        DEX
        STX ADDRL
        LDY #>$0800
        STY ADDRH
        STX BLOCKL
        STX BLOCKH
        STA BANKSP
        JSR PRODOS      ; CALL PRODOS HANDLER
        STA BANKSSC
        BCS FAILURE
        LDY $0800       ; NUMBER OF DISK II SECTORS TO READ
        DEY
        BNE FAILURE     ; ... MUST BE 1
        LDY $0801       ; FIRST OPCODE
        BEQ FAILURE     ; ... MUSTN'T BE BRK
        LDX UNIT
        JMP $0801       ; GO FOR IT

FAILURE LDY LOC0        ; POTENTIAL AUTOSTART SCAN ROM PTR LO
        BNE ERREXIT     ; ... MUST POINT TO A SLOT ROM START
        LDY LOC1        ; POTENTIAL AUTOSTART SCAN ROM PTR HI
        CPY MSLOT
        BNE ERREXIT     ; ... MUST POINT TO CURRENT SLOT ROM
        JMP SLOOP       ; LOOKS LIKE AUTOSTART SCAN SO CONTINUE SCAN

ERREXIT JSR SETSCR
        JSR SETKBD
        JSR SETTXT
        JSR HOME
        LDY #$08        ; 'HDD ERROR'
ERR     LDA ERRTEXT,Y
        JSR COUT
        DEY
        BPL ERR
        JMP BASIC

ERRTEXT DFB 'R'|$80, 'O'|$80, 'R'|$80, 'R'|$80, 'E'|$80, ' '|$80, 'D'|$80, 'D'|$80, 'H'|$80

;;;;;;;;;;;;;;;;;;;;;;;;
; BASIC INITIALIZATION ;
;;;;;;;;;;;;;;;;;;;;;;;;
BINIT   STA BENTER      ; ENTER A2RETRONET BASIC MODE
        BVC NORMIO
        ASL CMDBYTE,X   ; ALWAYS ENABLE COMMANDS
        LSR CMDBYTE,X
        LDA CMDREG,Y    ; JUST HAD A POWER-ON OR PROGRAM RESET?
        AND #$1F
        BNE BINIT1
        LDA #$EF        ; IF SO, GO JOIN INIT IN PROGRESS
        JSR INIT1

BINIT1  CPX CSWH
        BNE FROMIN
        LDA #<OENTRY
        CMP CSWL        ; IF CSW IS ALREADY POINTING TO OENTRY,
        BEQ FROMIN      ;  THEN WE MUST HAVE COME FROM KSW
        STA CSWL        ; OTHERWISE, SET CSW TO OENTRY
FROMOUT CLC             ; INDICATE WE ARE CALLED FOR OUTPUT
        BCC NORMIO      ; <ALWAYS>
FROMIN  CPX KSWH        ; MAKE SURE KSW POINTS HERE
        BNE FROMOUT     ;
        LDA #<IENTRY
        STA KSWL        ; SET UP KSW (NOTE CARRY SET FROM CPX)
;
; BRANCH TO APPROPRIATE BASIC I/O ROUTINE
;
NORMIO  LDA MISCFLG,X   ; SEPARATE CIC MODE FROM OTHERS
        AND #$02        ; NOT ZERO FOR CIC MODE
        PHP             ; SAVE CIC MODE INDICATION
        BCC BOUTPUT
        JMP BINPUT

BOUTPUT LDA STATEFLG,X  ; CHECK FOR AFTER LOWERCASE INPUT
        PHA
        ASL A
        BPL BOUTPUT1    ; SKIP IF NOT
        LDX ZPTEMP
        LDA CHARACTER
        ORA #$20
        STA INBUFF,X    ; RESTORE LOWERCASE IN BUFFER
        STA CHARACTER   ; AND FOR OUTPUT ECHO
        LDX MSLOT
BOUTPUT1 PLA
        AND #$BF        ; ZERO THE FLAG
        STA STATEFLG,X
        PLP             ; RETRIEVE CIC MODE INDICATION
        BEQ BOUTPUT2    ; BRANCH FOR PPC, SIC MODES
        JSR OUTPUT      ; CIC MODE OUTPUT
        JMP CICEXIT     ; FINISH BY CHECKING FOR TERM MODE

BOUTPUT2 JMP SEROUT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; RETURN FROM BASIC ENTRIES ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BEXIT   LDA CHARACTER
        STA BLEAVE      ; LEAVE A2RETRONET BASIC MODE
        RTS

        .RES $D000-*
