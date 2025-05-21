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
; CORE SUBROUTINES              ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GENERAL PURPOSE WAIT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; WAITMS WAITS FOR [A-REG] MILLISECONDS (256 IF A-REG=0)
;
WAITMS  LDX #202
WAITMS1 DEX             ; <DON'T LET THIS LOOP CROSS A PAGE>
        BNE WAITMS1     ; 5 MICROSECOND LOOP
        SEC
        SBC #01
        BNE WAITMS
        LDX MSLOT
        RTS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ACIA STATUS REGISTER READ ROUTINES ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; SRIN USED TO CHECK ACIA INPUT STATUS
;
SRIN    LDY SLOT16      ; SLOT16=$N0
        LDA STREG,Y
        PHA
        AND #$20        ; DCD?
        LSR A           ; AN ERROR IF NOT
        LSR A
        STA ZPTEMP
        PLA
        AND #$0F
        CMP #$08        ; SET CARRY IF RDR FULL, ELSE CLEAR
        BCC SRIN1
        AND #$07        ; PE, FE, OVR VALID ONLY WHEN RDR=1
        BCS SRIN2       ; <ALWAYS>
SRIN1   LDA ZPTEMP
SRIN2   ORA ZPTEMP      ; GET DCD ERROR BIT
        BEQ SRIN3       ; BRANCH IF NO ERRORS FOUND
        ORA #$20        ; ELSE SET BIT 5 TO OFFSET FOR PASCAL
        STA STSBYTE,X   ; AND SAVE IN STATUS TEMP
SRIN3   RTS             ; CY=1 MEANS DATA IS AVAILABLE
;
; SROUT CHECKS IF TDR IS EMPTY + HARDWARE HANDSHAKE IS OK
;
SROUT   LDY SLOT16
        LDA STREG,Y
        AND #$70
        CMP #$10        ; EQU IF TDR EMPTY, DCD, DSR, & CTS
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;
; GENERAL INPUT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;;
INPUT   JSR SRIN
        BCC NOINPUT1

        LDA RDREG,Y     ; GET THE ACIA INPUT
        ORA #$80        ; SET HI BIT FOR BASIC
        CMP #$8A        ; LINEFEED?
        BNE INPUT2

        TAY
        LDA MISCFLG,X   ; SEE IF WE SHOULD EAT IT
        AND #$20
        BNE NOINPUT     ; IF SO, JUST KEEP IT A SECRET
        TYA

INPUT2  SEC             ; INDICATE DATA
        RTS

NOINPUT CLC             ; CARRY CLEAR FOR NO INPUT
NOINPUT1 RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;
; GENERAL OUTPUT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; START OF COMMAND CHECK ROUTINE
;
CMDSEQCK LDY SLOT16
        LDA DIPSW1,Y
        LSR A
        BCS NOCMD       ; DON'T WORRY ABOUT CMD SEQ FOR SIC
        LDA STATEFLG,X
        AND #$07        ; ARE WE IN A COMMAND SEQUENCE?
        BEQ ESCCHECK
        JSR CMDPROC     ; IF SO, GOTO COMMAND CENTRAL
        SEC             ; INDICATE COMMAND
        RTS

ESCCHECK LDA CHARACTER
        AND #$7F        ; IGNORE HIGH BIT
        CMP CMDBYTE,X   ; IS THIS BEGINNING OF A CMD SEQ?
        BNE XOFFCK
        INC STATEFLG,X  ; START UP COMMAND MODES
        SEC             ; INDICATE COMMAND
        RTS

XOFFCK  LDA MISCFLG,X   ; IS XON ENABLED?
        AND #$08
        BEQ NOCMD       ; SKIP THIS IF NOT

        JSR INPUT       ; ANY INPUT?
        BCC NOCMD       ; IF NOT, GO OUTPUT
        CMP #$93        ; IS IT AN XOFF?
        BEQ XONWAIT     ; IF SO, GO WAIT FOR ANOTHER INPUT
        PHA
        LDA MISCFLG,X   ; CIC MODE?
        LSR A
        LSR A
        PLA
        BCC ANRTS
        STA BUFBYTE,X   ; IF SO, WE HAVE A BUFFER
NOCMD   CLC             ; INDICATE NOT A CMD SEQ
ANRTS   RTS

XONWAIT JSR GETCHAR     ; GET ACIA/KBD DATA
        CMP #$91        ; IS IT AN XON?
        BNE XONWAIT     ; IF NOT, WAIT
        CLC             ; OTHERWISE, INDICATE NOT A CMD SEQ
        RTS             ;  AND RETURN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; NOW THE OUTPUT ROUTINE YOU'VE BEEN WAITING FOR ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OUTPUT  JSR CMDSEQCK
        BCS ANRTS       ; DON'T OUTPUT COMMAND SEQUENCES

OUTPUT1 JSR SCREENOUT

OUTPUT2 LDY SLOT16
        LDA DIPSW1,Y
        LSR A
        BCC OUTPUT3     ; SKIP ETX/ACK FOR NATIVE MODES
        LSR A
        BCC OUTPUT3     ; BRANCH IF NOT P8A EMULATION

;;;;;;;;;;;;;;;;;;;;;
; P8A ETX/ACK STUFF ;
;;;;;;;;;;;;;;;;;;;;;
; AFTER 148 CHARACTERS BUT NOT WITHIN AN ESCAPE SEQUENCE
; OF UP TO 5 CHARACTERS, THE HANDSHAKE IS PERFORMED
; (WILL DELAY UNTIL 'NOT ESC' AND THEN 4 MORE CHARS
;  OR UNTIL AN 'ESC')

P8AOUT1 LDA CHARACTER   ; SAVE CHAR ON STACK
        PHA
        LDA HANDSHKE,X  ; CHAR COUNT FOR BUFFER FULL
        CMP #103        ; IF <103 THEN 153 CHARS IN BUFFER
        BCC ETX
        CMP #108        ; IF >=108 THEN LESS THAN 149 CHARS
        BCS P8AOUT2     ;  SO NO HANDSHAKE IS NEEDED YET
        CMP #107        ; SETS CARRY IF 107 (149 SENT)
        PLA
        PHA
        EOR #$9B        ; ESC?
        AND #$7F        ; IGNORE HI-BIT
        BNE P8AOUT2     ; COUNT AS 1 OF 5 IF NOT 'ESC'
        BCS P8AOUT3     ; DON'T COUNT IF 149TH CHAR IS 'ESC'

ETX     LDA STATEFLG,X  ; SEND QUERY CHAR TO PRINTER
        AND #$1F        ; (DEFAULT IS ETX)
        ORA #$80
        STA CHARACTER
        JSR ACIAOUT
ACK     JSR GETCHAR     ; GET ACIA/KBD DATA
        EOR #$86        ; ACK?
        BNE ETX         ; IF NOT ACK, REPEAT HANDSHAKE
        STA HANDSHKE,X  ; INIT CHAR COUNT TO 255

P8AOUT2 DEC HANDSHKE,X
P8AOUT3 PLA             ; GET REAL CHAR TO OUTPUT
        STA CHARACTER
        EOR #$8D        ; IF CR AND CR DELAY MODE
        ASL A
        BNE P8AOUT4     ;  THEN FAKE CHAR COUNT TO LESS THAN
        LDA DELAYFLG,X  ;  48 TO FORCE HANDSHAKE ON NEXT
        AND #$30        ;  CHARACTER OUT
        BEQ P8AOUT4
        STA HANDSHKE,X

P8AOUT4 JSR ACIAOUT
        JMP LFGEN       ; (SKIP DELAYS)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; AND BACK TO NORMAL OUTPUT ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OUTPUT3 JSR ACIAOUT     ; OUTPUT THE CHARACTER
;
; NOW CHECK FOR CR, LF, AND FF DELAYS
;
        ASL A
        TAY
        LDA DELAYFLG,X  ; GET DELAY FLAGS
        CPY #$18        ; FORM FEED?
        BEQ OUTDLY1
        LSR A
        LSR A           ; RIGHT JUSTIFY LF DELAY
        CPY #$14        ; LINE FEED?
        BEQ OUTDLY1
        LSR A
        LSR A           ; RIGHT JUSTIFY CR DELAY
        CPY #$1A        ; CARRIAGE RETURN?
        BNE OUTPUTEND
OUTDLY1 AND #$03        ; JUST WANT LOWEST 2 BITS
        BEQ LFGEN       ; NO DELAY INDICATED
        TAY
        LDA DLYTBL-1,Y
        TAY             ; DELAY IN 32 MSEC INCREMENTS
OUTDLYLP LDA #32
        JSR WAITMS
        DEY
        BNE OUTDLYLP
;
; CHECK ON LF GENERATION OPTION
;
LFGEN   LDA CHARACTER
        ASL A
        CMP #$1A        ; CARRIAGE RETURN?
        BNE OUTPUTEND
        LDA MISCFLG,X   ; IS LF GENERATE ENABLED?
        ROR A
        BCC OUTPUTEND
        LDA #$8A
        STA CHARACTER   ; LINE FEED
        JMP OUTPUT2     ; (DON'T ECHO IT)
OUTPUTEND RTS

DLYTBL  DFB $01         ; 32 MSEC
        DFB $08         ; 1/4 SEC
        DFB $40         ;  2  SEC
;;;;;;;;;;;;;;;;;;;;;;;
; ACIA OUTPUT ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;
ACIAOUT JSR SROUT       ; READY FOR OUTPUT?
        BNE ACIAOUT
        TYA
        ORA #<TDREG+1   ; PREPARE TO ADDRESS ACIA,
        TAY             ;  CAUSING 6502 FALSE READ TO OCCUR
        LDA CHARACTER   ;  ON PAGE $BF (AVOIDING RDR READ)
        STA $BFFF,Y     ; HERE YOU ARE ACIA
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; RESTORE CURSOR (NOT FOR PASCAL) ;
; (A-REG SHOULD CONTAIN NEW CHAR) ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
RESTORE PHA             ; SAVE NEW CHARACTER
        LDY CH
        LDA CHARACTER   ; OLD CHARACTER
        STA (BASL),Y
        PLA

        CMP #$95        ; SCREEN PICK?
        BNE RESTOREND
        LDA CHARACTER   ; IF SO, USE SCREEN CHAR
        CMP #$20        ; INVERSE?
        BCS RESTOREND
        JSR GETXLATE    ; REVERSE THE TRANSLATION
        EOR REVMASK,Y
RESTOREND STA CHARACTER
        RTS
