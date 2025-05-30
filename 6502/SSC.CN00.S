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

        ORG $C000+$0100*SLOT

.IF BANK & 1 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        BIT IORTS       ; SET THE V-FLAG
        BVS BENTRY      ; <ALWAYS>
        SEC
        DFB $90         ; OPCODE FOR BCC
        CLC
        CLV
        BVC BENTRY      ; <ALWAYS> SKIP AROUND PASCAL 1.1 ENTRY

.ELSE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        CPX #$20        ; $CN01:$20
        LDX #$00        ; $CN03:$00
        CPY #$03        ; $CN05:$03
        LDY #$00        ; $CN07:$00
        CLV
        BVC DENTRY      ; <ALWAYS> SKIP AROUND PASCAL 1.1 ENTRY

.ENDIF ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.ASSERT .LOBYTE(*)=$0B,ERROR

        DFB $01         ; GENERIC SIGNATURE BYTE
        DFB $31         ; DEVICE SIGNATURE BYTE
        DFB <PINIT
        DFB <PREAD
        DFB <PWRITE
        DFB <PSTATUS

;;;;;;;;;;;;;;;;;;;
;                 ;
;  BASIC ENTRIES  ;
;                 ;
;;;;;;;;;;;;;;;;;;;

IENTRY  SEC             ; BASIC INPUT ENTRY
        DFB $90         ; OPCODE FOR BCC
OENTRY  CLC             ; BASIC OUTPUT ENTRY
        CLV

BENTRY  STA CHARACTER
        STX ZPTEMP      ; INPUT BUFFER INDEX
        TXA             ; SAVE X AND Y REGS ON STACK
        PHA
        TYA
        PHA
        JSR SETUP
        JMP BINIT

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

;;;;;;;;;;;;;;;;;;;;;;;;
; SHARED SETUP ROUTINE ;
;;;;;;;;;;;;;;;;;;;;;;;;
SETUP   LDX #$C0+SLOT   ; X-REG WILL GENERALLY BE $CN
        STX MSLOT
        LDY #$10*SLOT   ; Y-REG WILL GENERALLY BE $N0
        STY SLOT16
        STA ROMSOFF     ; SWITCH OUT OTHER $C800 ROMS
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

        .RES $0100-$0A-<*

        ASC "OlSc"

.IF BANK & 1 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ASC "APPLE"

.ELSE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        DFB $00
        DFB $00         ; SMARTPORT ID
        DFB $00,$00     ; TOTAL BLOCKS
        DFB $D7         ; STATUS BYTE

.ENDIF ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.ASSERT .LOBYTE(*)=$FF,ERROR

        DFB <DISK
