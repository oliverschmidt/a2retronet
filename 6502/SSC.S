.FEATURE    labels_without_colons
.MACPACK    apple2
.DEFINE ASC scrcode
.DEFINE EQU =
.DEFINE DFB .byte
.DEFINE ORG .org

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
; VARIABLE DEFINITIONS          ;
;                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;
; ZERO PAGE EQUS ;
;;;;;;;;;;;;;;;;;;
LOC0    EQU $00
LOC1    EQU $01
CH      EQU $24         ; CURSOR HORIZONTAL POSITION
SLOT16  EQU $26         ; SAVE $N0 TO FREE UP Y-REG
CHARACTER EQU $27       ; OUTPUT, SCREEN AND INPUT CHARS
BASL    EQU $28         ; BASE SCREEN ADDRESS POINTER
ZPTEMP  EQU $35         ; WORKHORSE TEMPORARY
ZPTMP1  EQU $2A         ; WHEN ZPTEMP ISN'T ENOUGH
ZPTMP2  EQU $2B         ; TEMPORARIES, TEMPORARIES!
CSWL    EQU $36         ; CHAR OUT VECTOR
CSWH    EQU $37
KSWL    EQU $38         ; CHAR IN VECTOR
KSWH    EQU $39
A1L     EQU $3C         ; BATCH MOVE POINTER
RNDL    EQU $4E         ; RANDOM NUMBER SEED
RNDH    EQU $4F
PCMD    EQU $42         ; PRODOS DISK DRIVER COMMAND
PUNIT   EQU $43         ; PRODOS DISK DRIVER UNIT
PBUFL   EQU $44         ; PRODOS DISK DRIVER BUFFER
PBUFH   EQU $45
PBLOCKL EQU $46         ; PRODOS DISK DRIVER BLOCK
PBLOCKH EQU $47
;;;;;;;;;;;;;;;;;;;
; GENERAL EQUATES ;
;;;;;;;;;;;;;;;;;;;
STACK   EQU $0100       ; SYSTEM STACK BLOCK
INBUFF  EQU $0200       ; SYSTEM INPUT BUFFER
KBD     EQU $C000       ; KEYBOARD INPUT
KBDSTRB EQU $C010       ; KEYBOARD CLEAR
ROMSOFF EQU $CFFF       ; DISABLES C0-RES. $C800 ROMS
;;;;;;;;;;;;;;;;;;;;;;
; SSC CARD ADDRESSES ;
;;;;;;;;;;;;;;;;;;;;;;
DIPSW1  EQU  $C081      ; (+$N0) DIPSWITCH BLOCK 1
DIPSW2  EQU  $C082      ; (+$N0) DIPSWITCH BLOCK 2
TDREG   EQU $C088       ; (+$N0) TRANSMIT DATA REG (WRITE)
RDREG   EQU $C088       ; (+$N0) READ DATA REG (READ)
STREG   EQU $C089       ; (+$N0) STATUS REGISTER (READ)
RESET   EQU $C089       ; (+$N0) SOFTWARE RESET (WRITE)
CMDREG  EQU $C08A       ; (+$N0) COMMAND REGISTER (R/W)
CTLREG  EQU $C08B       ; (+$N0) CONTROL REGISTER (R/W)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  BIT-> B7  B6  B5  B4  B3  B2  B1  B0
;        +----------------------------+
; DIPSW1 S1  S2  S3  S4  Z   Z   S5  S6  (LEFT DIPSWITCH)
;
; (S1-S4 USED FOR BAUD RATE, S5-S6 FOR FIRMWARE MODE)
;
; DIPSW2 S1  Z   S2  Z   S3  S4  S5  CTS (RIGHT DIPSWITCH)
;
; STREG  INT DSR DCD TDR RDR OVR FE  PE
;
; CTLREG STB << WL >> CK << BAUD RATE >>
;
; CMDREG <<PARITY >> ECH <<XMIT>> RE DTR
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SCREEN VARIABLES: PPC AND SIC MODES ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CMDBYTE EQU $5F8-$C0    ; HOLDS COMMAND CHARACTER (PPC & CIC)
HANDSHKE EQU $4F8-$C0   ; SIC P8A CHAR COUNTER FOR ETX/ACK
PARAMETER EQU $4F8-$C0  ; ACCUMULATOR FOR CMD PARAMETER
STATEFLG EQU $578-$C0
; B7=CR GEN ENB FLAG  B6=AFTER LC INPUT FLG
; B2-B0=COMMAND INTERPRETER STATES
;   0 0 0   IDLE
;   0 0 1   CMD CHAR RECEIVED
;   0 1 0   COLLECT <N> UNTIL CHAR THEN DO COMMAND
;   0 1 1   SKIP UNTIL SPACE, THEN GOTO STATE 4
;   1 0 0   E/D COMMANDS
;   1 0 1   UNUSED
;   1 1 0   WAIT UNTIL CR THEN SET STATE TO ZERO
;   1 1 1   WAIT UNTIL CR THEN DO PROC INDICATED BY PARM
;
; (B4-B0 DETERMINE ENQUIRE CHAR FOR P8A MODE)
;
DELAYFLG EQU $478-$C0
; B7-B6=SCREEN TRANSLATION OPTIONS
;  0 0   LC->UC
;  0 1   NO TRANSLATION
;  1 0   LC->UC INVERSE
;  1 1   LC->UC, UC->UC INVERSE
;  (1-3 WILL ALLOW LC CHARS TO PASS THRU MONITOR)
;
; B5-B4=CR DELAY    0 0 = NO DELAY
; B3-B2=LF DELAY    0 1 = 32 MILLISEC
; B1-BO=FF DELAY    1 0 = 1/4 SEC
;                   1 1 = 2 SEC
;
STSBYTE EQU $678-$C0    ; STATUS/IORESULT/INPUT BYTE
PWDBYTE EQU $6F8-$C0    ; PRINTER (FORMAT) WIDTH
COLBYTE EQU $778-$C0    ; COLUMN POSITION COUNTER
MISCFLG EQU $7F8-$C0
; B7=ECHO BIT       B6=TABBING OPTION ENABLE
; B5=LINEFEED EAT   B4=PASCAL/BASIC FLAG
; B3=XOFF ENB FLAG  B2=KEYBOARD ENB
; B1=PPC/CIC MODE   B0=LF GENERATE ENB
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TEMP SCREEN VARS (SLOT INDEPENDENT) ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MSLOT   EQU $7F8        ; BUFFER FOR HI SLOT ADDR (SCN)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SCREEN VARIABLES: CIC MODE ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; STATEFLG: B7=TERMINAL MODE FLAG
;        B3-B5=CHAIN SLOT
;
CHNBYTE EQU $6F8-$C0    ; CURRENT OUTPUT SCREEN ($CN00 ENTRY)
;
; B0-B7=CN00 ENTRY
;
BUFBYTE EQU $778-$C0    ; BUFFER FOR ONE
;                         INPUT BYTE: HIGH BIT IS SET
;                         WHEN BUFFER IS FULL
;
; MISCFLG: B6=TERM MODE SHIFT ENB
;
; OTHER SLOT VARIABLES AS DEFINED FOR PPC AND SIC MODES
;
;;;;;;;;;;;;;;;;;;;;;;;
; MONITOR SUBROUTINES ;
;;;;;;;;;;;;;;;;;;;;;;;
COUT    EQU $FDED       ; CHARACTER OUT (THRU CSW)
SETKBD  EQU $FE89       ; SETS KSW TO APPLE KEYBOARD
IORTS   EQU $FF58       ; KNOWN "RTS" LOCATION
NXTA1   EQU $FCBA       ; INCREMENT A1H,L AND CMP TO A2H,L
SETSCR  EQU $FE93       ; SETS CSW TO APPLE SCREEN
VIDOUT  EQU $FDF6       ; OUTPUT A CHAR TO APPLE SCREEN
SLOOP   EQU $FABA       ; CONTINUE SLOT SCAN
SETTXT  EQU $FB39       ; SET TEXT MODE
HOME    EQU $FC58       ; CLEAR SCREEN AND HOME CURSOR
PRERR   EQU $FF2D       ; DISPLAY "ERR" AND BEEP
BASIC   EQU $E000       ; BASIC INTERPRETER COLD START

.INCLUDE "SSC.CN00.S"
.INCLUDE "SSC.C800.S"
.INCLUDE "SSC.HILEV.S"
.INCLUDE "SSC.TERM.S"
.INCLUDE "SSC.CORE.S"
.INCLUDE "SSC.UTIL.S"
.INCLUDE "SSC.CMD.S"
.INCLUDE "SSC.CF00.S"
