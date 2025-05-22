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
; CN00 SPACE CODE               ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.REPEAT 2,MODE
.REPEAT 8,SLOT

.IF MODE .OR SLOT
        .SCOPE
.ENDIF

        ORG $C000+$0100*SLOT

.IF MODE = 1 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BINIT   BIT IORTS       ; SET THE V-FLAG
        BVS BENTRY      ; <ALWAYS>
IENTRY  SEC             ; BASIC INPUT ENTRY
        DFB $90         ; OPCODE FOR BCC
OENTRY  CLC             ; BASIC OUTPUT ENTRY
        CLV
        BVC BENTRY      ; <ALWAYS> SKIP AROUND PASCAL 1.1 ENTRY

.ELSE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BINIT   CPX #$20        ; $CN01:$20
        LDX #$00        ; $CN03:$00
IENTRY  CPY #$03        ; $CN05:$03
        LDY #$00        ; $CN07:$00
OENTRY  CLV
        BVC DENTRY      ; <ALWAYS> SKIP AROUND PASCAL 1.1 ENTRY

.ENDIF ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.ASSERT .LOBYTE(*)=$0B,ERROR

        DFB $01         ; GENERIC SIGNATURE BYTE
        DFB $31         ; DEVICE SIGNATURE BYTE
        DFB <PINIT
        DFB <PREAD
        DFB <PWRITE
        DFB <PSTATUS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                          ;
;  DISK INTERFACE ENTRIES  ;
;                          ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DISK    CLC             ; PRODOS DRIVER ENTRY
        BCC MARK
        SEC             ; SMARTPORT DRIVER ENTRY
MARK    LDX #$FF
DENTRY  LDA #$C0+SLOT
        STA MSLOT
        STA ROMSOFF
        JMP COMMON

;;;;;;;;;;;;;;;;;
;               ;
;  BASIC ENTRY  ;
;               ;
;;;;;;;;;;;;;;;;;

BENTRY  STA CHARACTER
        STX ZPTEMP      ; INPUT BUFFER INDEX
        TXA             ; SAVE X AND Y REGS ON STACK
        PHA
        TYA
        PHA
        JSR SETUP
        BVC NORMIO
;
; BASIC INITIALIZATION
;
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

;;;;;;;;;;;;;;;;;;;;;;;;
; SHARED SETUP ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;
SETUP   LDX #$C0+SLOT   ; X-REG WILL GENERALLY BE $CN
        STX MSLOT
        LDY #$10*SLOT   ; Y-REG WILL GENERALLY BE $N0
        STY SLOT16
        STA ROMSOFF     ; SWITCH OUT OTHER $C800 ROMS
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                ;
;  NEW PASCAL INTERFACE ENTRIES  ;
;                                ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PINIT   JSR PASCALINIT
        LDX #0          ; NO ERROR POSSIBLE
        RTS
PREAD   JMP PASCALREAD
PWRITE  JMP PASCALWRITE
;
; NEW PASCAL STATUS REQUEST
;
; A-REG=0 -> READY FOR OUTPUT?
; A-REG=1 -> HAS INPUT BEEN RECEIVED?
;
PSTATUS LSR A           ; SAVE REQUEST TYPE IN CARRY
        JSR PENTRY      ; (PRESERVES CARRY)
        BCS PSTATIN
        JSR SROUT       ; READY FOR OUTPUT?
        BEQ PSTATUS2
        CLC
        BCC PSTATUS2    ; CARRY CLEAR EOR NOT READY

PSTATIN JSR SRIN        ; SETS CARRY CORRECTLY
PSTATUS2 LDA STSBYTE,X  ; GET ERROR FLAGS
        TAX
        RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ROUTINE TO SEND A CHARACTER TO ANOTHER CARD ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SENDCD  LDX #3
SAVEHOOK LDA CSWL,x
        PHA
        DEX
        BPL SAVEHOOK
;
; NOW PUT CARD ADDRESS IN HOOK
;
        LDX MSLOT
        LDA CHNBYTE,X
        STA CSWL
        LDA STATEFLG,X  ; GET SLOT #
        AND #$38
        LSR A
        LSR A
        LSR A
        ORA #$C0        ; FORM $CN
        STA CSWH
;
; OUTPUT TO THE PERIPHERAL
;
        LDA CHARACTER
        PHA
        ORA #$80        ; 80 COL BOARDS WANT HI-BIT ON
        JSR COUT
;
; NOW RESTORE EVERYTHING THE OTHER CARD MAY HAVE CLOBBERED
;
        PLA
        STA CHARACTER
        JSR SETUP
;
; PUT BACK CSWL INTO CHNBYTE
;
        LDA CSWL
        STA CHNBYTE,X

        LDX #0
RESTORHOOK PLA
        STA CSWL,X
        INX
        CPX #4
        BCC RESTORHOOK

        LDX MSLOT
        RTS

        ASC "OlSc"

.IF MODE = 1 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ASC "APPLE"

.ELSE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        DFB $00
        DFB $00         ; SMARTPORT ID
        DFB $00,$00     ; TOTAL BLOCKS
        DFB $D7         ; STATUS BYTE

.ENDIF ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.ASSERT .LOBYTE(*)=$FF,ERROR

        DFB <DISK

.IF MODE .OR SLOT
        .ENDSCOPE
.ENDIF

.ENDREP
.ENDREP
