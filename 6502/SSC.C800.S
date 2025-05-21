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
; C800 SPACE: HIGH LEVEL STUFF  ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL 1.0 INIT ENTRY ;
;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG $C800

PASCALINIT JSR PENTRY   ; PASCAL 1.0 INITIALIZATION ENTRY
        LDA #$16        ; NO XOFF, ECHO, LF EAT, OR LF GEN
INIT1   PHA             ; GOES TO MISCFLG AFTER MODIFICATION
        LDA #0
        STA STATEFLG,X
        STA DELAYFLG,X
        STA HANDSHKE,X
        STA STSBYTE,X
        STA PWDBYTE,X
        STA COLBYTE,X
        LDA DIPSW2,Y    ; SET LF GEN OPTION FROM D2-S5
        STA ZPTMP2      ; SAVE FOR LATER
        LSR A           ; S5-> CARRY
        LSR A           ; IF S5=ON=0 THEN LEAVE MISCFLG ALONE
        BCC INIT1A
        PLA             ; OTHERWISE, MAKE SURE LF GEN
        AND #$FE        ;  ENABLE IS RESET
        PHA
INIT1A  CLV             ; V WILL BE CLEAR FOR CIC MODE
        LDA DIPSW1,Y
        LSR A           ; SIC MODES SET CARRY
        BCS INIT2       ; BRANCH FOR SIC MODES
        LSR A
        BCS INIT2B      ; PPC MODE BRANCH
        LDA #$01        ; CTL-A
        BNE INIT5       ; <ALWAYS> CIC MODE BRANCH

INIT2   LSR A           ; SET CARRY FOR P8A
        LDA #$03        ; SET ETX AS DEFAULT INQUIRY CHAR
        BCS INIT2A      ; BRANCH FOR P8A
        LDA #$80        ; FOR P8 SET AUTO CR GEN
INIT2A  STA STATEFLG,X
INIT2B  BIT IORTS       ; SET V-FLAG FOR PPC, SIC MODES
        LDA ZPTMP2
        AND #$20        ; SET CR DELAY
        EOR #$20        ; SO 1=ENB, 0=DISABLE
        STA DELAYFLG,X  ; FROM D2-S2

        BVS INIT3       ; <ALWAYS> BRANCH AROUND PASCAL
;;;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL 1.0 READ ENTRY ;
; (MUST BE AT $C84D)    ;
;;;;;;;;;;;;;;;;;;;;;;;;;
PREAD0  JSR PASCALREAD  ; DO PASCAL 1.1 READ
        LDX MSLOT       ; MODIFY FOR 1.0
        STA STSBYTE,X   ; CHARACTER READ
        RTS
;;;;;;;;;;;;;;;;;;;;;;;;
; NOW WHERE WERE WE??? ;
;;;;;;;;;;;;;;;;;;;;;;;;

INIT3   LDA ZPTMP2      ; PPC, SIC MODES USE SWITCHES
        LSR A           ; TO SET PWIDTH, CR DELAY
        LSR A
        AND #$03
        TAY
        BEQ INIT4

        PLA             ; RESET VIDEO ENABLE FOR PWIDTH#40
        AND #$7F
        PHA

INIT4   LDA PWDTBL,Y
        STA PWDBYTE,X
        LDY SLOT16

        PLA             ; CLEAR CIC BIT IN FUTURE MISCFLG
        AND #$95        ;  (AND TABBING, XOFF AND LF EAT BITS)
        PHA
        LDA #$09        ; CTL-I

INIT5   STA CMDBYTE,X   ; CMD ESC CHAR (IGNORED FOR SIC MODES)
        PLA
        STA MISCFLG,X   ; SET MISCFLG FLAGS
;
; NOW FOR THE ACIA INITIALIZATION ROUTINE
;
INITACIA LDA ZPTMP2     ; DIPSW2
        PHA
        AND #$A0        ; DATA BIT OPTIONS FOR CIC MODE
        BVC INITACIA1   ; BRANCH FOR CIC MODE
        AND #$80        ; 8 DATA, 1 OR 2 STOP FOR SIC, PPC
INITACIA1 JSR DATACMD1  ; SET CONTROL REG
        JSR BAUDCMD1    ; SET DIPSWITCH BAUD RATE
        PLA
        AND #$0C        ; PARITY OPTIONS FOR CIC MODE
        BVC INITACIA2   ; BRANCH FOR CIC MODE
        LDA #$0         ; DISABLE PARITY FOR SIC, PPC MODES
INITACIA2 ASL A
        ASL A
        ASL A
        ORA #$0B
        STA CMDREG,Y
        LDA RDREG,Y     ; THROW OUT THE STRANGE STUFF
        RTS
;;;;;;;;;;;;;;;;;;;;;;;
; PASCAL READ ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;
PASCALREAD JSR PENTRY   ; SHARED BY BOTH PASCAL VERSIONS
PASCALREAD1 JSR GETCHAR ; GET ACIA/KBD DATA
        AND #$7F        ; CLEAR HIGH BIT FOR PASCAL
PASEXIT LDY MSLOT
        LDX STSBYTE,Y   ; ERROR STATUS-> X-REG
        RTS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GETCHAR ROUTINE WAITS FOR  ;
;  THE NEXT CHAR FROM EITHER ;
;  THE ACIA OR KEYBOARD (IF  ;
;  ENABLED).  USED BY PASCAL ;
;  READ ROUTINE, XON WAIT,   ;
;  AND ACK WAIT. DATA IS RE- ;
;  TURNED IN THE A=REGISTER  ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETCHAR JSR INPUT       ; ACIA DATA?
        BCS GETCHAR1
        JSR CKKBD       ; KEYBOARD INPUT?
        BCC GETCHAR
GETCHAR1 RTS            ; EXIT WHEN WE HAVE SOMETHING
