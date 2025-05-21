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
; SSC COMMAND PROCESSOR         ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; COMMAND TABLE (USED BY COMMAND PROCESSER ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CMDTBL  DFB $42         ; B(REAK)
        DFB $67         ; CIC     PAS  NS=7
        DFB <BREAKCMD-1
        DFB $54         ; T(ERMINAL)
        DFB $47         ; CIC          NS=7
        DFB <TERMCMD-1
        DFB $43         ; C(R GENERATE)
        DFB $87         ;     PPC      NS=7
        DFB <TERMCMD-1
        DFB $51         ; Q( UIT)
        DFB $47         ; CIC          NS=7
        DFB <QUITCMD-1
        DFB $52         ; R ESET)
        DFB $C7         ; CIC PPC      NS=7
        DFB <RESETCMD-1
        DFB $5A         ; Z COMMAND
        DFB $E7         ; CIC PPC PAS  NS=7
        DFB <ZCMD-1
        DFB $49         ; I COMMAND
        DFB $90         ;     PPC      NS=0
        DFB <ICMD-1
        DFB $4B         ; K COMMAND
        DFB $90         ;     PPC      NS=0
        DFB <KCMD-1

        DFB $45         ; E(CHO)
        DFB $43         ; CIC          NS=3
        DFB $80
        DFB $46         ; F(ROMKYBD)
        DFB $E3         ; CIC PPC PAS  NS=3
        DFB $04
        DFB $4C         ; L(F GENERATE)
        DFB $E3         ; CIC PPC PAS  NS=3
        DFB $01
        DFB $58         ; X(OFF)
        DFB $E3         ; CIC PPC PAS  NS=3
        DFB $08
        DFB $54         ; T(ABBING)
        DFB $83         ;     PPC      NS=3
        DFB $40
        DFB $53         ; S(HIFTING)
        DFB $43         ; CIC          NS=3
        DFB $40
        DFB $4D         ; M(UNCH LF)
        DFB $E3         ; CIC PPC PAS  NS=3
        DFB $20

        DFB $00         ; END OF FIRST PART MARKER

CMDTBL1 DFB $42         ; B(AUD)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <BAUDCMD-1
        DFB $50         ; P(ARITY)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <PARITYCMD-1
        DFB $44         ; D(ATA)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <DATACMD-1
        DFB $46         ; F(F DELAY)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <FFCMD-1
        DFB $4C         ; L(F DELAY)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <LFCMD-1
        DFB $43         ; C(R DELAY)
        DFB $F6         ; CIC PPC PAS  NS=6
        DFB <CRCMD-1
        DFB $54         ; T(RANSLATE)
        DFB $D6         ; CIC PPC      NS=6
        DFB <TRANCMD-1
        DFB $4E         ; N COMMAND
        DFB $90         ;     PPC      NS=0
        DFB <NCMD-1
        DFB $53         ; S(CREENSLOT)
        DFB $56         ; CIC          NS=6
        DFB <SSLOTCMD-1

        DFB $00         ; END OF TABLE MARKER

;;;;;;;;;;;;;;;;;;;;;;
; COMMAND ROUTINES   ;
; (CALLED BY PARSER) ;
; (MUST START IN     ;
;  PAGE $CD . . . )  ;
;;;;;;;;;;;;;;;;;;;;;;
TRANCMD LDA #$3F        ; SET SCREEN TRANSLATE OPTIONS
        LDY #$7
        BNE DELAYSET    ; <ALWAYS>
CRCMD   LDA #$CF        ; SET CR DELAY
        LDY #$5
        BNE DELAYSET    ; <ALWAYS>

LFCMD   LDA #$F3        ; SET LF DELAY
        LDY #$3
        BNE DELAYSET    ; <ALWAYS>

FFCMD   LDA #$FC        ; SET FF DELAY
        LDY #$1
DELAYSET AND DELAYFLG,X ; DON'T DISTURB THE OTHER FLAGS
        STA ZPTMP1
        LDA PARAMETER,X
        AND #$03        ; JUST USE TWO BITS
        CLC
        ROR A           ; ONCE FOR FUN
ROTATE  ROL A           ; CHANGE DIRECTIONS
        DEY
        BNE ROTATE      ; PREPARE IT TO OR INTO THE FLAGS

        ORA ZPTMP1
        STA DELAYFLG,X
        RTS

SSLOTCMD AND #$7        ; SET SLOT COMMAND
        ASL A
        ASL A
        ASL A
        STA ZPTMP1
        ASL A
        CMP SLOT16      ; MAKE SURE WE DON'T SET IT
        BEQ SSLOTCMD1   ;  TO OUR OWN SLOT
        LDA STATEFLG,X
        AND #$C7        ; PUT NEW SLOT NUMBER IN BITS 3-5
        ORA ZPTMP1      ;  OF CMDBYTE,X
        STA STATEFLG,X
        LDA #0          ; STORE ZERO INTO
        STA CHNBYTE,X   ; SLOT OFFSET (SET TO CN00 ENTRY)
SSLOTCMD1 RTS

BAUDCMD AND #$0F        ; SET NEW BAUD RATE
        BNE BAUDCMD2
BAUDCMD1 LDA DIPSW1,Y   ; ZERO PARM = RELOAD FROM SWITCHES
        LSR A
        LSR A
        LSR A
        LSR A
BAUDCMD2 ORA #$10       ; SET INT. BAUD RATE GENERATOR
        STA ZPTMP1
        LDA #$E0
CTLREGSET STA ZPTMP2
        LDA CTLREG,Y
        AND ZPTMP2
        ORA ZPTMP1
        STA CTLREG,Y
        RTS

PARITYCMD DEY           ; TRICK: SO CTLREG,Y ACTUALLY
                        ;  ADDRESSES THE COMMAND REG

DATACMD ASL A           ; SET NEW # OF DATA BITS
        ASL A
        ASL A
        ASL A
        ASL A
DATACMD1 STA ZPTMP1
        LDA #$1F
        BNE CTLREGSET   ; <ALWAYS>

TERMCMD ASL STATEFLG,X  ; SET TERMINAL MODE
        SEC
        BCS QCMD1       ; <ALWAYS>

RESETCMD STA RESET,Y    ; DROP RTS, DTR
        JSR SETSCR      ; PR#0
        JSR SETKBD      ; IN#O
        LDX MSLOT
QUITCMD ASL STATEFLG,X  ; CLEAR TERMINAL MODE
        CLC
QCMD1   ROR STATEFLG,X
        RTS

BREAKCMD LDA CMDREG,Y   ; SEND BREAK SIGNAL
        PHA             ;  FOR 233 MILLISECONDS
        ORA #$0C
        STA CMDREG,Y
        LDA #233        ; DELAY FOR 233 MICROSEC.
        JSR WAITMS
        PLA             ; RESTORE OLD COMMAND REG CONTENTS
        STA CMDREG,Y
        RTS

ICMD    LDA #$28
        STA PWDBYTE,X   ; SET PRINTER WIDTH TO 40
        LDA #$80
        ORA MISCFLG,X   ; SET SCREEN ECHO
        BNE KCMD2       ; <ALWAYS>

KCMD    LDA #$FE        ; RESET THE LF GENERATE FLAG
KCMD1   AND MISCFLG,X
KCMD2   STA MISCFLG,X
        RTS

NCMD    CMP #40         ; >=40?
        BCC ZCMDRTS     ; IF NOT, JUST EXIT
        STA PWDBYTE,X   ; SET NEW PRINTER WIDTH
        LDA #$3F        ; DISABLE SCREEN, SET LISTING MODE
        BNE KCMD1       ; <ALWAYS>

ZCMD    ASL CMDBYTE,X   ; DISABLE COMMAND RECOGNITION
        SEC
        ROR CMDBYTE, X
ZCMDRTS RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; VECTOR ACCORDING TO COMMAND STATE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CMDPROC TAY             ; A-REG=COMMAND STATE
        LDA CHARACTER
        AND #$7F

        CMP #$20        ; SKIP SPACES FOR ALL MODES
        BNE CMDPROC2
        CPY #$3         ; EXCEPT MODE 3
        BEQ CMDPROC1
        RTS
CMDPROC1 LDA #$4
        BNE SETOSTATE   ; <ALWAYS>

CMDPROC2 CMP #$0D       ; CARRIAGE RETURN?
        BNE CMDPROC4
        JSR ZEROSTATE   ; ABORT FOR STATES 0-5, EXIT FOR 6,7
        CPY #$07        ; IN STATE 7 WE VECTOR TO THE PROC
        BEQ CMDPROC3
        RTS             ; OTHERWISE, JUST EXIT

CMDPROC3 LDA #$CD       ; ALL PROCS MUST START IN PAGE $CD
        PHA
        LDA PARAMETER,X
        PHA
        LDY SLOT16      ; NEEDED BY BREAK CMD
        RTS

CMDPROC4 STA ZPTEMP
        LDA #$CE        ; ALL ROUTINES MUST START
        PHA             ;  IN PAGE SCE
        LDA STATETBL,Y
        PHA
        LDA ZPTEMP
        RTS             ; RTS TO COMMAND PROCEDURE
;
; NOW THE STATE ROUTINES
;
;;;;;;;;;;;;;;;;;;;;;;
; STATE BRANCH TABLE ;
;;;;;;;;;;;;;;;;;;;;;;
STATETBL DFB <STATERR-1 ; BAD STATE
        DFB <CSTATE1-1  ; <CMD> SEEN
        DFB <CSTATE2-1  ; ACCUMULATE PARAMETER
        DFB <CDONE-1    ; SKIP UNTIL SPACE
        DFB <CSTATE4-1  ; E/D SOMETHING
        DFB <STATERR-1  ; ILLEGAL STATE
        DFB <CDONE-1    ; SKIP UNTIL CR
        DFB <CDONE-1    ; SKIP UNTIL CR THEN DO CMD
;;;;;;;;;;;;;;;;;;;
; COMMAND STATE 1 ;
;;;;;;;;;;;;;;;;;;;
CSTATE1 CMP CMDBYTE,X   ; IS IT <CMD>?
        BNE CSTATE1A
        DEC STATEFLG,X  ; SET STATE BACK TO ZERO
        JMP ACIAOUT     ; OUTPUT <CMD> IF SO

CSTATE1A CMP #$30       ; >=0?
        BCC CSTATE1B
        CMP #$3A        ; <=9?
        BCS CSTATE1B
        AND #$0F        ; IT'S A NUMBER
        STA PARAMETER,X
        LDA #2
        BNE SETOSTATE   ; <ALWAYS> SET MODE 2 AND RETURN

CSTATE1B CMP #$20       ; IS IT A CONTROL CHAR?
        BCS CSTATE1C
        STA CMDBYTE,X   ; SET NEW COMMAND CHARACTER
        JMP ZEROSTATE   ; RESET STATE TO ZERO

CSTATE1C LDY #0         ; USE COMMAND TABLE
        BEQ CMDSEARCH   ; <ALWAYS>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; COMMAND STATE 2: ACCUMULATE PARAMETER ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CSTATE2 EOR #$30        ; CONVERT $30-$39 TO 0-9
        CMP #$A         ; 0-9?
        BCS CSTATE2A
        LDY #$A         ; IT'S A NUMBER, SO ADD
ACCLOOP ADC PARAMETER,X ;  IT TO 10*PARAMETER
        DEY
        BNE ACCLOOP
        STA PARAMETER, X
        BEQ CDONE       ; <ALWAYS>

CSTATE2A LDY #CMDTBL1-CMDTBL ; USE COMMAND TABLE
        BNE CMDSEARCH   ; <ALWAYS>
;;;;;;;;;;;;;;;;;;;;;
; SET COMMAND STATE ;
;;;;;;;;;;;;;;;;;;;;;
ZEROSTATE LDA #0
SETOSTATE STA ZPTMP1
        LDX MSLOT
        LDA STATEFLG,X
        AND #$F8
        ORA ZPTMP1
        STA STATEFLG,X
CDONE   RTS
;;;;;;;;;;;;;;;;;;;;;;;;;
; COMMAND STATE 4 (E/D) ;
;;;;;;;;;;;;;;;;;;;;;;;;;
CSTATE4 TAY             ; E/D -> Y-REG
        LDA PARAMETER,X
        CPY #$44        ; D(ISABLE)?
        BEQ CSTATE4A
        CPY #$45        ; E(NABLE)?
        BNE STATERR     ; IF NOT, IGNORE THIS COMMAND
        ORA MISCFLG,X   ; SET FLAG
        BNE CSTATE4B    ; <ALWAYS>
CSTATE4A EOR #$FF       ; INVERT FOR DISABLE
        AND MISCFLG,X   ; RESET FLAG
CSTATE4B STA MISCFLG,X
;;;;;;;;;;;;;;;;;;;;;
; ESCAPE TO STATE 6 ;
;;;;;;;;;;;;;;;;;;;;;
SETSTATE6 LDA #6
        BNE SETOSTATE   ; <ALWAYS>
STATERR LDA #32         ; CODE FOR BAD COMMAND
        STA STSBYTE,X
        BNE SETSTATE6   ; <ALWAYS>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TABLE DRIVEN COMMAND PROCESSOR ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CMDSEARCH LDA CMDTBL,Y  ; GET CANDIDATE CHARACTER
        BEQ STATERR     ; A ZERO MARKS THE END OF A SUBTABLE
        CMP ZPTEMP      ; MATCH?
        BEQ CMDMATCH
        INY
CMDSEARCH1 INY          ; REENTRY FOR WRONG MODES
        INY             ; ENTRY LENGTH = 3
        BNE CMDSEARCH   ; <ALWAYS>

CMDMATCH INY
        LDA CMDTBL,Y
        STA ZPTMP1
        AND #$20        ; CHECK PASCAL ENABLE
        BNE CMDMATCH1   ; IT'S ON SO DONT CHECK P-BIT
        LDA MISCFLG,X   ; OFF SO MAKE SURE
        AND #$10        ;  THAT WE AREN'T IN PASCAL
        BNE CMDSEARCH1  ; BRANCH IF WE ARE

CMDMATCH1 LDA MISCFLG,X ; GET CIC/PPC BIT
        LSR A           ; SHIFT CIC/PPC MODE BIT TO CARRY
        LSR A
        BIT ZPTMP1      ; PPC=>N CIC=>V
        BCS CMDMATCH2   ; BRANCH IF CIC MODE
        BPL CMDSEARCH1  ; NOT OK FOR PPC
        BMI CMDEXEC     ; AND OK
CMDMATCH2 BVC CMDSEARCH1 ; NOT OK FOR CIC

CMDEXEC LDA ZPTMP1      ; RETRIEVE TABLE MODE BYTE
        PHA
        AND #$07
        JSR SETOSTATE   ; SET NEXT STATE
        INY
        PLA
        AND #$10
        BNE CMDEXEC1    ; IF BIT 4 IS SET, VECTOR TO ROUTINE
        LDA CMDTBL,Y
        STA PARAMETER,X
        RTS

CMDEXEC1 LDA #$CD       ; ROUTINES MUST BE IN PAGE $CD
        PHA
        LDA CMDTBL,Y
        PHA
        LDY SLOT16
        LDA PARAMETER,X ; LOT OF ROUTINES NEED THIS
        RTS

        DFB $C2
