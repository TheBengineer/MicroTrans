;*********************************************************************
; gcc port (C) 2006 James Murray
; took S12SerMon2r1 and stripped down to be called from within user
; application and run from ram
; $Id: ms2_sermon.s,v 1.1 2013/01/16 23:48:29 jsm Exp $

; To use this first set baud rate etc. on port once after boot
; echo -n \!\!\! > com1:      ; should put MS2 into monitor mode
; ./ms2dl.exe ms2_extra.s19   ; load new code
; echo -n -e "\0264" > com1:  ; reboot

;*********************************************************************
;.pagewidth  120t
;*********************************************************************
;* Title:  S12SerMonxrx.asm        Copyright (c) Motorola 2003
;*********************************************************************
;* Author: Jim Sibigtroth - Motorola TSPG - 8/16 Bit Division
;* Author: Jim Williams - Motorola TSPG - 8/16 Bit Division
;*
;* Description: Bootloader/Monitor program for HCS9S12
;* bootloader will reside in 2K of block protected memory at the
;* end of the memory map of an HCS9S12 MCU (0xF7FF-0xFFFF).
;*
;* Since this code is located in the vector space, all interrupt
;* vectors will be mirrored to the pseudo vector table in user
;* erasable and reprogrammable flash memory just before the start
;* of the protected bootloader code.
;*
;* If a non-FFFF user reset vector is programmed into the
;* pseudo-reset vector, the bootloader will jump to that routine
;* so the user can control all options including write-once bits.
;*
;* This monitor program implements 23 primitive monitor commands that
;* are very similar to BDM commands. Third-party tool vendors can
;* adapt their existing BDM-based tools to work through a serial I/O
;* cable rather than a BDM pod, simply by providing a set of alternate
;* interface routines. Although this monitor approach has some
;* limitations compared to the BDM approach, it provides a free or
;* very low cost alternative for the most cost-sensitive users.
;*
;* This monitor uses SCI0 as the primary interface to the target MCU
;* system and SCI0 Rx interrupts are used to break out of a running
;* user program. This implies that some monitor functions will not be
;* available if the I bit in the CCR is not clear during execution of
;* the user's program. During debug of user initialization programs
;* and interrupt service routines when the I bit is not clear, trace
;* and breakpoint functions still work as expected because these
;* functions use on-chip breakpoint logic.
;*
;*
;*
;* Revision History: not yet released
;* Rev #     Date      Who     Comments
;* -----  -----------  ------  ---------------------------------------
;*  2.00   04-SEP-03   JPW     First Release.
;*  2.01   03-DEC-03   JPW     MC9S12NE64 support added, fixed user jump table,
;*                             fixed PLL/Timer Ch.7 corruption
;*                             Added Flash/EEPROM support > 12.8MHz OSC
;

;*
;*
;*********************************************************************
;*********************************************************************
;* Motorola reserves the right to make changes without further notice
;* to any product herein to improve reliability, function, or design.
;* Motorola does not assume any liability arising out of the
;* application or use of any product, circuit, or software described
;* herein; neither does it convey any license under its patent rights
;* nor the rights of others.  Motorola products are not designed,
;* intended, or authorized for use as components in systems intended
;* for surgical implant into the body, or other applications intended
;* to support life, or for any other application in which the failure
;* of the Motorola product could create a situation where personal
;* injury or death may occur.  Should Buyer purchase or use Motorola
;* products for any such intended or unauthorized application, Buyer
;* shall indemnify and hold Motorola and its officers, employees,
;* subsidiaries, affiliates, and distributors harmless against all
;* claims, costs, damages, and expenses, and reasonable attorney fees
;* arising out of, directly or indirectly, any claim of personal
;* injury or death associated with such unintended or unauthorized
;* use, even if such claim alleges that Motorola was negligent
;* regarding the design or manufacture of the part.
;*
;* Motorola is a registered trademark of Motorola, Inc.
;*********************************************************************

.sect .text      ; was .text3
.globl monitor
.globl ResetCmd

;*********************************************************************
;* Include standard definitions that are common to all derivatives
;*********************************************************************
;             base    10           ;ensure default number base to decimal
             nolist               ;turn off listing
             include "s12asmdefs.inc"
             include "S12SerMon2r1.def"
             list                 ;turn listing back on
;*********************************************************************
;* general equates for bootloader/monitor program valid for all
;* derivatives
;*********************************************************************
.equ BootStart,      0xF800         ;start of protected boot block
.equ RamLast,        0x3fff         ;last RAM location (all devices)
.equ Window,         0x8000         ;PPAGE Window start
.equ RomStart,       0x4000         ;start of flash
.equ VecTabSize,     0x80           ;size of vector table
.equ VectorTable,    0x10000-VecTabSize ;start of vector table
.equ PVecTable,      BootStart-VecTabSize ;start of pseudo vector table
.equ FProtStart,     0xFF00         ;start of FLASH protection/security
.equ FProtBlksz,     0xC7           ;protect code for boot block (0xC7 2K)
;.equ FProtBlksz,     0xFF           ;protect code for boot block (none)
.equ FSecure,        0xBE           ;Disable Security and backdoor access
;.equ FSecure,        0x00           ;Enable Security and backdoor access

.equ longBreak,      1500          ;delay time for >30-bit break
; make TxD low at least 300us (30 bits @ 115200 baud)
; 5~ * 42ns/~ * 1500 = 315us (not exact, just >30 bit times)
.equ asciiCR,        0x0D           ;ascii carriage return

.equ flagReg,        SCI0CR1       ;SCI control1 reg of SCI0
.equ RunFlag,        WAKE          ;SCI Wake bit used as run/mon flag
.equ ArmFlag,        RSRC          ;SCI RSRC bit used for ARM storage
.equ TraceFlag,      ILT           ;SCI Idle bit used as trace flag
; 1=SWI caused by return from Trace1; 0=SWI from breakpoint or DBG

.equ initSCI0CR2,    0x0C           ;SCI0 Control Register 2
.equ traceOne,       0x80           ;BKPCT0 pattern for trace1 cmd
;
;CPU HCS12 CCR immediately after reset is:
.equ initUCcr,       0b11010000     ;initial value for user's CCR
;                    SX-I----     ;I interrupts masked
								  ;(SXHINZVC=11x1xxxx).

.equ ErrNone,        0xE0           ;code for no errors
.equ ErrCmnd,        0xE1           ;command not recognized
.equ ErrRun,         0xE2           ;command not allowed in run mode
.equ ErrSP,          0xE3           ;SP was out of range
.equ ErrWriteSP,     0xE4           ;attempted to write bad SP value
.equ ErrByteNVM,     0xE5           ;write_byte attempt NVM
.equ ErrFlash,       0xE6           ;FACCERR or FPVIOL error
.equ ErrFlErase,     0xE7           ;Error code not implemented
.equ ErrGoVec,       0xE8           ;Error code not implemented
.equ ErrEeErase,      0xE9			  ;EACCERR or EPVIOL error

.equ StatHalt,       0x02           ;stopped by Halt command
.equ StatTrace,      0x04           ;returned from a Trace1 command
.equ StatBreak,      0x06           ;Breakpoint or DBG (SWI) request
.equ StatCold,       0x08           ;just did a cold reset
.equ StatWarm,       0x0C           ;warm start because int with bad SP

;*********************************************************************
;* User CPU registers stack frame...
;*   +0  UCcr   <- Monitor's SP
;*   +1  UDreg   (B:A)
;*   +3  UXreg
;*   +5  UYreg
;*   +7  UPc
;*   +9  ---     <- User's SP
; Offsets from actual SP to user CPU regs while in monitor
;*********************************************************************

.equ UCcr,           0             ;user's CCR register
.equ UDreg,          1             ;user's D register (B:A)
.equ UXreg,          3             ;user's X register
.equ UYreg,          5             ;user's Y register
.equ UPc,            7             ;user's PC
.equ SPOffset,       9             ;offset of stack pointer while in monitor

.equ MaxMonStack,    35             ;maximum number of bytes used by Monitor
.equ LowSPLimit,     RamStart+MaxMonStack-SPOffset
.equ HighSPLimit,    RamLast-SPOffset+1

; named locations on stack if SWI with bad SP value
;*********************************************************************

;first portion of code runs from flash
;*********************************************************************

monitor: sei                   ;turn off interrupts to prevent normal code running
;  turn off interrupt sources to be on the safe side
             movb   #0x00,CRGINT   ;  disable CRG interrupt
             movb   #0x80,CRGFLG   ;  clear interrupt flag (0 writes have no effect)
             movb   #0x00,SCI0CR1  ;  no parity and stuff
             movb   #0x0c,SCI0CR2  ;  SCI, no ints, enable tx,rx
             movb   #0x00,TSCR1    ;  disable timer
             movb   #0x00,TIE      ;  no timer interrupts

             lds    #RamLast+1     ;point one past RAM

             ldx   #ramEndMonitor-ramMonitor+1
monmoveLoop: ldd   ramMonitor-1,x      ;read from flash
             std   RamStart-1,x     ;move into base of ram
             dbne x,monmoveLoop    ;loop till whole routine moved
             jmp   RamStart        ; jump to the code we've copied

;*********************************************************************
;  Formal start of Monitor code
;*********************************************************************
ramMonitor:
;
; set baud rate to 115.2 kbaud and turn on Rx and Tx
;
;             movb  #baud115200,SCI0BDL  ;..BDH=0 so baud = 115.2 K
;             movb  #initSCI0CR2,SCI0CR2 ;Rx and Tx on

; Cold start so Generate long break to host
;
coldBrk:     brclr  SCI0SR1,TDRE,coldBrk ;wait for Tx (preamble) empty
             bset   SCI0CR2,SBK   ;start sending break after preamble
             ldx   #longBreak     ;at least 30 bit times for Windows
BrkLoop:     cpx   #0             ;[2]done?
             dbne   x,BrkLoop     ;[3]
             bclr   SCI0CR2,SBK   ;stop sending breaks

waitforCR:   jsr    GetChar-ramMonitor+RamStart      ;should be asciiCR or 0x00 with FE=1
             cmpa  #asciiCR       ;.eq. if 115.2K baud OK
             bne    waitforCR

;*********************************************************************
;* end of reset initialization, begin body of program
;*********************************************************************
;
; Send a cold start prompt and wait for new commands
;
             ldaa  #ErrNone       ;code for no errors (0xE0)
             jsr    PutChar-ramMonitor+RamStart       ;send error code (1st prompt char)
             ldaa  #StatCold      ;status code for cold start (0x08)
             bra    EndPrompt     ;finish warm start prompt
;
; normal entry point after a good command
; Prompt is an alt entry point if an error occurred during a command
; endPrompt is an alternate entry for Trace1, Break (SWI), Halt,
; or warm/cold resets so an alternate status value can be sent
; with the prompt
;

CommandOK:   ldaa  #ErrNone       ;code for no errors (0xE0)
Prompt:      jsr    PutChar-ramMonitor+RamStart       ;send error code
;             ldaa   flagReg       ;0 means monitor active mode
;             anda  #RunFlag       ;mask for run/monitor flag (SCI WAKE)
;             lsra                 ;shift flag to LSB
;             lsra                 ; for output as status
;             lsra                 ;0x00=monitor active, 0x01=run
             clra
EndPrompt:   jsr    PutChar-ramMonitor+RamStart       ;send status code
             ldaa  #'>'
             jsr    PutChar-ramMonitor+RamStart       ;send 3rd character of prompt seq

;;test flagReg for run / DBG arm status.
;             brclr  flagReg,RunFlag,Prompt1  ;no exit if run flag clr
;             brclr  flagReg,ArmFlag,PromptRun  ;If DBG was not armed just run
;             bset	DBGC1,ARM	  ;re-arm DBG module
;PromptRun:   jmp    GoCmd-ramMonitor+RamStart         ;run mode so return to user program
;

Prompt1:     jsr    GetChar-ramMonitor+RamStart       ;get command code character
             ldx   #commandTbl-ramMonitor+RamStart    ;point at first command entry
CmdLoop:     cmpa    ,x           ;does command match table entry?
             beq    DoCmd          ;branch if command found
             leax   3,x
             cpx   #tableEnd-ramMonitor+RamStart      ;see if past end of table
             bne    CmdLoop       ;if not, try next entry
             ldaa  #ErrCmnd       ;code for unrecognized command
             bra    Prompt        ;back to prompt; command error

DoCmd:       ldx    1,x           ;get pointer to command routine
             jmp     ,x           ;go process command
;
; all commands except GO, Trace_1, and Reset to user code - jump to
; Prompt after done. Trace_1 returns indirectly via a SWI.
;
;*********************************************************************
;* Command table for bootloader/monitor commands
;*  each entry consists of an 8-bit command code + the address of the
;*  routine to be executed for that command.
;*********************************************************************
commandTbl:  fcb   0xA1
             fdb  RdByteCmd-ramMonitor+RamStart     ;read byte
             fcb   0xA2
             fdb  WtByteCmd-ramMonitor+RamStart     ;write byte
             fcb   0xA3
             fdb  RdWordCmd-ramMonitor+RamStart     ;read word of data
             fcb   0xA4
             fdb  WtWordCmd-ramMonitor+RamStart     ;write word of data
             fcb   0xA5
             fdb  RdNextCmd-ramMonitor+RamStart     ;read next word
             fcb   0xA6
             fdb  WtNextCmd-ramMonitor+RamStart     ;write next word
             fcb   0xA7
             fdb  ReadCmd-ramMonitor+RamStart       ;read n bytes of data
             fcb   0xA8
             fdb  WriteCmd-ramMonitor+RamStart      ;write n bytes of data
             fcb   0xA9
             fdb  RdRegsCmd-ramMonitor+RamStart     ;read CPU registers
             fcb   0xAA
             fdb  WriteSpCmd-ramMonitor+RamStart    ;write SP
             fcb   0xAB
             fdb  WritePcCmd-ramMonitor+RamStart    ;write PC
             fcb   0xAC
             fdb  WriteIYCmd-ramMonitor+RamStart    ;write IY
             fcb   0xAD
             fdb  WriteIXCmd-ramMonitor+RamStart    ;write IX
             fcb   0xAE
             fdb  WriteDCmd-ramMonitor+RamStart     ;write D
             fcb   0xAF
             fdb  WriteCcrCmd-ramMonitor+RamStart   ;write CCR
;             fcb   0xB1
;             fdb  GoCmd-ramMonitor+RamStart         ;go
;             fcb   0xB2
;             fdb  Trace1Cmd-ramMonitor+RamStart     ;trace 1
;             fcb   0xB3
;             fdb  HaltCmd-ramMonitor+RamStart       ;halt
             fcb   0xB4
             fdb  ResetCmd-ramMonitor+RamStart      ;reset - to user vector or monitor
;            0xB5 - Command not implemented
             fcb   0xB6          ;code - erase flash command
             fdb  EraseAllCmd-ramMonitor+RamStart   ;erase all flash and eeprom command routine
             fcb   0xB7          ;return device ID
             fdb  DeviceCmd-ramMonitor+RamStart
             fcb   0xB8          ;erase current flash bank selected in PPAGE
             fdb  ErsPage-ramMonitor+RamStart
;             fcb   0xB9			;Bulk erase eeprom if available
;             fdb  EraseEECmd-ramMonitor+RamStart	;
tableEnd: ;XXX    equ    *           ;end of command table marker

;*********************************************************************
;* Device ID Command -  Ouputs hex word from device ID register
;*********************************************************************
DeviceCmd:   ldaa   #0xDC         ;get part HCS12 descripter
             jsr    PutChar-ramMonitor+RamStart      ;out to term
             ldaa   PARTIDH      ;get part ID high byte
             jsr    PutChar-ramMonitor+RamStart      ;out to term
             ldaa   PARTIDL      ;get part ID low byte
             jsr    PutChar-ramMonitor+RamStart      ;out to term
             ldaa   #ErrNone     ;error code for no errors
             jmp    Prompt-ramMonitor+RamStart       ;ready for next command


;*********************************************************************
;* Halt Command - halts user application and enters Monitor
;*   This command is normally sent by the debugger while the user
;*   application is running. It changes the state variable in order
;*   to stay in the monitor
;*********************************************************************
;HaltCmd: ;    bclr   flagReg,RunFlag ;run/mon flag = 0; monitor active
;             ldaa  #ErrNone        ;error code for no errors
;             jsr    PutChar-ramMonitor+RamStart        ;send error code
;             ldaa  #StatHalt       ;status code for Halt command
;             jmp    EndPrompt-ramMonitor+RamStart      ;send status and >

;*********************************************************************
;* Reset Command - forces a reset
;*********************************************************************
ResetCmd:     movb   #0x43,COPCTL  ; turn on COP
              movb   #0x00,ARMCOP  ; try to force COP reset
              fcb    0x3c          ; force illegal instruction reset
              rts                  ; in case this is a multi-byte command,
              rts                  ; ensure don't fall through to next routine
              rts


;*********************************************************************
;* Erase EE Command -  mass
;*  erase all EEPROM locations
;*
;* Eeprom erasure assumes no protection. (Mass command will fail)
;*********************************************************************
;EraseEECmd:  jsr    abClr-ramMonitor+RamStart         ;abort commands and clear errors
;
;             brclr  MEMSIZ0,eep_sw1+eep_sw0,ErsPageErr1  ;Check if device has EEprom
;             ldy   #EEpromStart   ; get device eeprom start
;             std    0,y           ; write to eeprom (latch address)
;                                  ; data is don't care (but needed)
;
;             movb  #0x41,ECMD      ;mass erase command
;             movb  #CBEIF,ESTAT   ;register the command
;             nop                  ; wait a few cycles for
;             nop                  ; command to sync.
;             nop
;ChkDoneE:    ldaa   ESTAT         ;wait for CBEIF=CCIF=1 (cmnd done)
;             bpl    ChkDoneE      ;loop if command buffer full (busy)
;             asla                 ;moves CCIF to MSB (set/clear N bit)
;             bpl    ChkDoneE      ;loop if CCIF=0 (not done)
;             ldaa   FSTAT
;             anda  #0x30           ;mask all but PVIOL or ACCERR
;             bne    ErsPageErr1   ;back to prompt-flash error
;             ldaa   #ErrNone      ;code for no errors (0xE0)
;             jmp    Prompt-ramMonitor+RamStart        ;ready for next command
;
;ErsPageErr1: ldaa   #ErrEeErase   ;Erase error code (0xE9)
;             jmp    Prompt-ramMonitor+RamStart        ;ready for next command
;
;*********************************************************************
;* Erase Command - Use repeated page erase commands to erase all flash
;*  except bootloader in protected block at the end of flash, and mass
;*  erase all EEPROM locations
;*
;* Eeprom erasure assumes no protection. (Mass command will fail)
;*********************************************************************
EraseAllCmd: jsr    abClr-ramMonitor+RamStart         ;abort commands and clear errors

;             brclr  MEMSIZ0,eep_sw1+eep_sw0,ErsBlk0  ;Check if device has EEprom
;             ldy   #EEpromStart   ; get device eeprom start
;             std    0,y           ; write to eeprom (latch address)
;                                  ; data is don't care (but needed)
;
;             movb  #MassErase,ECMD      ;mass erase command
;             movb  #CBEIF,ESTAT   ;register the command
;             nop                  ; wait a few cycles for
;             nop                  ; command to sync.
;             nop
;ChkDoneE1:   ldaa   ESTAT         ;wait for CBEIF=CCIF=1 (cmnd done)
;             bpl    ChkDoneE1     ;loop if command buffer full (busy)
;             asla                 ;moves CCIF to MSB (set/clear N bit)
;             bpl    ChkDoneE1     ;loop if CCIF=0 (not done)

;
; erase flash pages from RomStart to start of protected bootloader
; no need to check for errors because we cleared them before EE erase
;

ErsBlk0:                          ; sector erase all full blocks
             ldab   #PagesBlk     ; Get number of banks/blocks
             decb                 ; erase all but last
             stab   1,-sp         ; save counter
             ldaa   #0x3f          ; highest bank
             sba                  ; Compute lowest page-1
             staa   PPAGE         ; PPAGE for first 16K page of block 0
                                  ; (passed in the A accumulator).
             clr    FCNFG         ; set block select bits to 0.
ErsBlk0Lp:   ldx    #SectorSize   ; select sector size
             ldd    #0x4000        ; Window size
             idiv                 ; compute total number of sectors
             tfr    x,d           ; get number of sectors in B
             ldx   #Window        ; point to the start of the PPAGE window.
             bsr    ErsSectors    ; go erase the PPAGE window a sector at a time.
             inc    PPAGE         ; go to the next PPAGE.
             dec    0,sp          ; done with all full PPAGE blocks?
             bne    ErsBlk0Lp     ;   no? then erase more blocks.

             ldx    #SectorSize   ; select sector size
             ldd    #((BootStart-0xc000)) ; get size - protected amount
             idiv                 ; compute total number of sectors
                                  ; minus the bootblock.
             tfr    x,d           ; get number of sectors in B
             ldx   #Window        ; point to the start of the PPAGE window.
             bsr    ErsSectors    ; go erase the PPAGE window a sector at a time.
             pulb                 ; remove the page count from the stack.

; erase all sectors outside the bootblock.
;
;********************************************************************
;bulk erase all the rest
;********************************************************************

             ldab  #FlashBlks    ; select lowest page of the highest bank
             decb                 ;
             beq    EraseDone     ; if single block device quit
             ldab  #LowestPage    ; select lowest bank
BlockLoop:   stab   PPAGE         ; must match array selection
             lsrb                 ; calculate the value of the block select bits based
             lsrb                 ; on bits 3:2 of the PPAGE register value. (<256k)
             ldy   #SectorSize    ; get high byte of size
             cpy   #0x0200         ; if larger than 0x200 shift again
             beq    nBlockLoop    ; otherwise skip ahead
             lsrb                 ; on bits 4:3 of the PPAGE register value. (512k)
nBlockLoop:  comb
             andb  #0x03           ; mask off all but the lower 2 bits.
             stab   FCNFG         ; select the block to erase.
             bsr    BulkErase     ; erase it
             ldab   PPAGE         ;get ppage back
             addb  #PagesBlk      ;
             cmpb  #(0x3F-PagesBlk) ; see if last block
             bmi    BlockLoop

EraseDone:   movb  #0x3D,PPAGE     ;select bank in array0

OkCommand:   jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

BulkErase:   pshx                 ;save address
             ldx    #Window       ;must point into bank
             staa   ,x            ;latch address to erase
             movb  #MassErase,FCMD      ; Select mass erase
             movb  #CBEIF,FSTAT   ;register the command
             nop                  ;wait a few cycles for
             nop                  ;command to sync.
             nop
ChkDoneF:    ldaa   FSTAT         ;wait for CBEIF=CCIF=1 (cmnd done)
             bpl    ChkDoneF      ;loop if command buffer full (busy)
             asla                 ;moves CCIF to MSB (set/clear N bit)
             bpl    ChkDoneF      ;loop if CCIF=0 (not done)
             pulx                 ;get address back
             rts
;Erase 'b' (accumulator) sectors beginning at address 'x' (index register)
;
ErsSectors:  exg    b,y           ;put the sector count in y.
ErsSectLp:   std    ,x
             movb  #SecErase,FCMD ;perform a sector erase.
             jsr    SpSub-ramMonitor+RamStart     ;finish command from stack-based sub
             tsta                 ;check for 0=OK
             bne    ErsSectErr    ;back to prompt-flash erase error
             leax   SectorSize,x  ;point to the next sector.
             dbne   y,ErsSectLp   ;continue to erase remaining sectors.
             rts

ErsSectErr:  puld                 ; clear stack
             bra    ErsPageErr

ErsPage:     jsr    abClr-ramMonitor+RamStart         ; abort commands and clear errors
	         ldab   PPAGE         ; get current ppage

             lsrb                 ; calculate the value of the block select bits based
             lsrb                 ; on bits 3:2 of the PPAGE register value. (<256k)
             ldy   #SectorSize    ; get high byte of size
             cpy   #0x0200         ; if larger than 0x200 shift again
             beq    ErsPage1      ; otherwise skip ahead
             lsrb                 ; on bits 4:3 of the PPAGE register value. (512k)
ErsPage1:    comb
             andb  #0x03           ; mask off all but the lower 2 bits.
             stab   FCNFG         ; select the block to erase.
             ldab   PPAGE         ; get current ppage
             cmpb  #0x3F		      ; is it the page with the monitor
             bne   ErsFullPage    ; no then erase all of page
             ldx   #SectorSize    ; select sector size
             ldd   #((BootStart-0xc000)) ; get size - protected amount
             idiv                 ; compute total number of sectors
                                  ; minus the bootblock.
             tfr    x,d           ; get number of sectors in B
             ldx   #Window        ; point to the start of the PPAGE window.
             bsr    ErsSectors    ; go erase the PPAGE window a sector at a time.
             bra    EraPageStat   ; back to no error and prompt

ErsFullPage: ldx   #SectorSize    ; select sector size
             ldd   #0x4000         ; Window size
             idiv                 ; compute total number of sectors
             tfr    x,d           ; get number of sectors in B
             ldx   #Window        ; point to the start of the PPAGE window.
             bsr    ErsSectors    ; go erase the PPAGE window a sector at a time.
             bra    EraPageStat     ;back to no error and prompt

EraPageStat: ldaa   FSTAT
             anda  #0x30           ;mask all but PVIOL or ACCERR
             bne    ErsPageErr    ;back to prompt-flash error
             ldaa   #ErrNone      ;code for no errors (0xE0)
             jmp    Prompt-ramMonitor+RamStart        ;ready for next command

ErsPageErr: ldaa   #ErrFlash      ;code for Flash error (0xE6)
             jmp    Prompt-ramMonitor+RamStart        ;ready for next command

;*********************************************************************
;* Read Byte Command - read specified address and return the data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  8-bit data sent back to host through SCI0 TxD
;*********************************************************************
RdByteCmd:   jsr    getX-ramMonitor+RamStart          ;get address to read from
             ldaa   ,x            ;read the requested location
             jsr    PutChar-ramMonitor+RamStart       ;send it out SCI0
             jmp    CommandOK-ramMonitor+RamStart     ;ready for next command

;*********************************************************************
;* Read Word Command - read specified block of data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  16-bit number sent back to host through SCI0 TxD
;* Special case of block read.
;*********************************************************************
RdWordCmd:   jsr    getX-ramMonitor+RamStart          ;get address to read from
sendExit:    ldd    ,x            ;read the requested location
             jsr    PutChar-ramMonitor+RamStart       ;send it out SCI0
             tba
             jsr    PutChar-ramMonitor+RamStart       ;send it out SCI0
             jmp    CommandOK-ramMonitor+RamStart     ;ready for next command

;*********************************************************************
;* Read Command - read specified block of data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  8-bit number of bytes-1 to sent back to host through SCI0 TxD
;*********************************************************************
ReadCmd:     jsr    getX-ramMonitor+RamStart          ;get address to read from
             jsr    GetChar-ramMonitor+RamStart       ;get number of bytes to read
             tab
             incb                 ;correct counter (0 is actually 1)
ReadNext:    ldaa   ,x            ;read the requested location
			 jsr    PutChar-ramMonitor+RamStart       ;send it out SCI0
             inx
             decb
             bne    ReadNext
             ldaa  #ErrNone       ;code for no errors (0xE0)
xPrompt:     jmp    Prompt-ramMonitor+RamStart        ;ready for next command

;*********************************************************************
;* Write Command - write specified block of data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  8-bit number of bytes-1 to write from host to SCI0 TxD
;*  8-bit values to write
;* this function used Word writes whenever possible. This is:
;* a) when more than one byte is still to write
;* b) and the address is even
;*********************************************************************
WriteCmd:    jsr    getX-ramMonitor+RamStart          ;get address to write to
             jsr    GetChar-ramMonitor+RamStart       ;get number of bytes to read
             tab
             incb                 ;correct counter (0 is actually 1)
WriteNext:   cmpb   #1            ;if only one byte left
             pshb                 ;preserve byte counter
             beq    WriteByte     ;write it
             tfr    x,a           ;is address odd
             bita   #1
             bne    WriteByte     ;write a byte first

WriteWord:   jsr    GetChar-ramMonitor+RamStart       ;get high byte
             tab                  ;save in B
             dec    ,sp           ;decrement byte counter (on stack)
             jsr    GetChar-ramMonitor+RamStart       ;get low byte
             exg    a,b           ;flip high and low byte
             jsr    WriteD2IX-ramMonitor+RamStart     ;write or program data to address
             pulb                 ;restore byte counter
             bne    WriteError    ;error detected
             inx                  ;increment target address
             bra    Write1

WriteByte:   jsr    GetChar-ramMonitor+RamStart       ;get data to write
             jsr    WriteA2IX-ramMonitor+RamStart     ;write or program data to address
             pulb                 ;restore byte counter
             bne    WriteError    ;error detected
Write1:      inx                  ;increment target address
             decb                 ;decrement byte counter
             bne    WriteNext
             ldaa   #ErrNone      ;code for no errors (0xE0)
             bra    xPrompt       ;then back to prompt

SkipBytes:   jsr    GetChar-ramMonitor+RamStart       ;read remaining bytes
WriteError:  decb                 ;
             bne    SkipBytes
             ldaa   #ErrFlash     ;code for Flash error (0xE6)
WriteDone:   bra    xPrompt       ;then back to prompt

;*********************************************************************
;* Read Next Command - IX=IX+2; read m(IX,IX=1) and return the data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data sent back to host through SCI0 TxD
;*  uses current value of IX from user CPU regs stack frame
;*********************************************************************
RdNextCmd: ;  brclr  flagReg,RunFlag,notRun  ;do command if not run
           ;  clra                 ;data = 0x00 (can't read real data)
           ;  jsr    PutChar-ramMonitor+RamStart       ;send 0x00 instead of read_next data
           ;  jsr    PutChar-ramMonitor+RamStart       ;send 0x00 instead of read_next data
           ;  ldaa   #ErrRun       ;code for run mode error
;xCmnd:       jmp    Prompt-ramMonitor+RamStart        ;back to prompt; run error
notRun:      bsr    preInc        ;get, pre-inc, & update user IX
             jmp    sendExit-ramMonitor+RamStart      ;get data, send it, & back to prompt

;*********************************************************************
;* Write Byte Command - write specified address with specified data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  8-bit data from host to SCI0 RxD
;*********************************************************************
WtByteCmd:   jsr    getX-ramMonitor+RamStart          ;get address to write to
WriteNext2:  jsr    GetChar-ramMonitor+RamStart       ;get data to write
             jsr    CheckModule-ramMonitor+RamStart
             beq    isRAMbyte
             bra    WriteByteNVM  ;deny access (byte NVM access)

isRAMbyte:   staa   0,x           ;write to RAM or register
             clra                 ;force Z=1 to indicate OK

WriteExit:   ldaa  #ErrNone       ;code for no errors (0xE0)
             jmp    Prompt-ramMonitor+RamStart        ;ready for next command

WriteByteNVM: ldaa #ErrByteNVM    ;code for byte NVM error (0xE5)
             jmp    Prompt-ramMonitor+RamStart        ;ready for next command


;*********************************************************************
;* Write Word Command - write word of data
;*  8-bit command code from host to SCI0 RxD
;*  16-bit address (high byte first) from host to SCI0 RxD
;*  16-bit value to write
;*********************************************************************
WtWordCmd:   jsr    getX-ramMonitor+RamStart          ;get address to write to
			 ldab  #02            ;one word +1
			 pshb				  ;save it on stack
             bra    WriteWord     ;get & write data, & back to prompt

;*********************************************************************
;* Write Next Command - IX=IX+1; write specified data to m(IX)
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data from host to SCI0 RxD
;*
;*  uses current value of IX from user CPU regs stack frame
;*********************************************************************
WtNextCmd: ;  brclr  flagReg,RunFlag,notRunW  ;do command if not run
           ;  jsr    getX-ramMonitor+RamStart          ;clear data
           ;  ldaa   #ErrRun       ;code for run mode error
;xCmndW:      jmp    Prompt-ramMonitor+RamStart        ;back to prompt; run error

notRunW:     bsr    preInc        ;get, pre-inc, & update user IX
			 ldab  #02            ;one word +1
			 pshb				  ;save it on stack
             bra    WriteWord     ;get & write data, & back to prompt

;*********************************************************************
;* utility to get IX from stack frame and pre increment it by 2
;* assumes interrupts are blocked while in monitor
;*********************************************************************
preInc:      leas 2,sp
             ldx    UXreg,sp      ;get user X
             inx                  ;pre-increment
             inx                  ;pre-increment
             stx    UXreg,sp      ;put adjusted user X back on stack
             leas -2,sp
             rts                  ;pre-incremented IX still in IX

;*********************************************************************
;* Read Registers Command - read user's CPU register values
;*
;*  16-bit SP value (high byte first) sent to host through SCI0 TxD
;*  16-bit PC value (high byte first) sent to host through SCI0 TxD
;*  16-bit IY value (high byte first) sent to host through SCI0 TxD
;*  16-bit IX value (high byte first) sent to host through SCI0 TxD
;*  16-bit D  value (high byte first) sent to host through SCI0 TxD
;*   8-bit CCR value sent to host through SCI0 TxD
;*
;* User CPU registers stack frame...
;*
;*   +0  UCcr   <- Monitor's SP
;*   +1  UDreg   (B:A)
;*   +3  UXreg
;*   +5  UYreg
;*   +7  UPc
;*   +9  ---     <- User's SP
;*********************************************************************
RdRegsCmd:   tsx                  ;IX = Monitor SP +2
             leax   SPOffset,x    ;correct SP value
             jsr    put16-ramMonitor+RamStart         ;send user SP out SCI0
             ldx    UPc,sp        ;user PC to IX
             jsr    put16-ramMonitor+RamStart         ;send user PC out SCI0
             ldx    UYreg,sp      ;user IY to IX
             jsr    put16-ramMonitor+RamStart         ;send user IY out SCI0
             ldx    UXreg,sp      ;user IX to IX
             jsr    put16-ramMonitor+RamStart         ;send user IX out SCI0
             ldx    UDreg,sp      ;user D to IX
             exg    d,x
             exg    a,b           ;flip as D is stacked B:A
             exg    d,x
             jsr    put16-ramMonitor+RamStart         ;send user D out SCI0
             ldaa   UCcr,sp       ;user CCR to A
             jsr    PutChar-ramMonitor+RamStart       ;send user CCR out SCI0
             jmp    CommandOK-ramMonitor+RamStart     ;back to prompt

;*********************************************************************
;* Write CCR Command - write user's CCR register value
;*  8-bit command code from host to SCI0 RxD
;*  8-bit data for CCR from host to SCI0 RxD
;*********************************************************************
WriteCcrCmd: jsr    GetChar-ramMonitor+RamStart       ;read new CCR value
             staa   UCcr,sp       ;replace user CCR value
             jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

;*********************************************************************
;* Write D Command - write user's D register value
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data (high byte first) for D from host to SCI0 RxD
;*********************************************************************
WriteDCmd:   jsr    getX-ramMonitor+RamStart          ;read new D value
             exg    d,x
             exg    a,b           ;flip as D is stacked B:A
             exg    d,x
             stx    UDreg,sp      ;replace user D value
             jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

;*********************************************************************
;* Write IX Command - write user's IX register value
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data (high byte first) for IX from host to SCI0 RxD
;*********************************************************************
WriteIXCmd:  jsr    getX-ramMonitor+RamStart          ;read new IX value
             stx    UXreg,sp      ;replace user IX value
             jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

;*********************************************************************
;* Write IY Command - write user's IY register value
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data (high byte first) for IY from host to SCI0 RxD
;*********************************************************************
WriteIYCmd:  jsr    getX-ramMonitor+RamStart          ;read new IY value
             stx    UYreg,sp      ;replace user IY value
             jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

;*********************************************************************
;* Write PC Command - write user's PC register value
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data (high byte first) for PC from host to SCI0 RxD
;*********************************************************************
WritePcCmd:  jsr    getX-ramMonitor+RamStart          ;read new PC thru SCI0 to IX
             stx    UPc,sp       ;replace user PC value
             jmp    CommandOK-ramMonitor+RamStart     ;back to no error and prompt

;*********************************************************************
;* Write SP Command - write user's SP register value
;*  8-bit command code from host to SCI0 RxD
;*  16-bit data (high byte first) for SP from host to SCI0 RxD
;*
;*  Since other user CPU register values are stored on the stack, the
;*  host will need to re-write the other user registers after SP is
;*  changed. This routine just changes SP itself.
;*
;*  SP value is user's SP & it is adjusted (-10) to accommodate the
;*  user CPU register stack frame.
;*
;*  If the host attempts to set the user SP value <RamStart or >RamLast
;*  then the change is ignored, because such values would not support
;*  proper execution of the monitor firmware.
;*********************************************************************
WriteSpCmd:  bsr    getX         ;new SP value now in IX
             leax  -SPOffset,x   ;correct SP value
             cpx   #LowSPLimit   ;check against lower limit
             blo    spBad
             cpx   #HighSPLimit  ;check against upper limit
             bhi    spBad
             txs                 ;IX -> SP
             jmp    CommandOK-ramMonitor+RamStart    ;back to no error and prompt
spBad:       ldaa    #ErrWriteSP      ;error code for stack errors
;             bsr    PutChar      ;send error code
             jmp    Prompt-ramMonitor+RamStart       ;send status and >

;*********************************************************************
;* Trace 1 Command - trace one user instruction starting at current PC
;*  8-bit command code from host to SCI0 RxD
;*
;*  if an interrupt was already pending, the user PC will point at the
;*  ISR after the trace and the opcode at the original address will
;*  not have been executed. (because the interrupt response is
;*  considered to be an instruction to the CPU)
;*********************************************************************
;
;pagebits:   fcb      0x3D		;0x0000-0x3FFF is PPAGE 0x3D
;            fcb      0x3E		;0x4000-0x7FFF is PPAGE 0x3E
;            fcb      0x3F		;0xC000-0xFFFF is PPAGE 0x3F
;pagebitsaddr:
;            fdb     pagebits     ;0x0000-0x3FFF : Use constant 0x3D
;            fdb     pagebits+1   ;0x4000-0x7FFF : Use constant 0x3E (2nd last page)
;            fdb     0x0030        ;0x8000-0xBFFF : Use window PPAGE
;            fdb     pagebits+2   ;0xC000-0xFFFF : Use constant 0x3F (last page)
;Trace1Cmd:
;;            bset    flagReg,TraceFlag  ;so at SWI we know it was Trace
;            ldx     UPc,sp       ;PC of go address
;            inx                  ;IX points at go opcode +1
;            inx                  ;IX points at go opcode +2
; 	        xgdx
;            andb   #0xFE
;            std     DBGACH       ;(BKP0H) debugger trigger address
;            std     DBGBCH       ;(BKP1H) same for second address to have it initialized
;            rola
;            rolb
;            rola
;            rolb				 ;get ready to search pagebits table
;            andb   #0x03          ;what range 0-3FFF,4000-7FFF,8000-BFFF,or C000-FFFF?
;            clra
;            lsld
;            xgdx
;            ldx     pagebitsaddr,x
;            ldaa    0,x
;
;            staa    DBGACX       ;(BKP0X) set page byte of address
;            staa    DBGBCX       ;(BKP1X) same for second address to have it initialized
;            ldaa   #traceOne     ; enable, arm, CPU force
;            staa    DBGC2        ;(BKPCT0) arm DBG to trigger after 1 instr.
;            rti                  ; restore regs and go to user code

;*********************************************************************
;* Go Command - go to user's program at current PC address
;*  8-bit command code from host to SCI0 RxD
;* - no promt is issued
;*  typically, an SWI will cause control to pass back to the monitor
;*********************************************************************
;GoCmd:       bset SCI0CR2,RIE     ;need to enable SCI0 Rx interrupts to
;                                  ; enter monitor on any char received
;             bclr  flagReg,TraceFlag ; run flag clr
;             rti                  ;restore regs and exit
;*********************************************************************
;* Utility to send a 16-bit value out X through SCI0
;*********************************************************************
put16:       exg    d,x           ;move IX to A
             bsr    PutChar       ;send high byte
             tba                  ;move B to A
             bsr    PutChar       ;send low byte
             rts

;*********************************************************************
;* Utility to get a 16-bit value through SCI0 into X
;*********************************************************************
getX:        bsr    GetChar       ;get high byte
             tab                  ;save in B
             bsr    GetChar       ;get low byte
             exg    a,b           ;flip high and low byte
             exg    d,x           ;16-bit value now in IX
             rts
;*********************************************************************
;* GetChar - wait indefinitely for a character to be received
;*  through SCI0 (until RDRF becomes set) then read char into A
;*  and return. Reading character clears RDRF. No error checking.
;*
;* Calling convention:
;*            bsr    GetChar
;*
;* Returns: received character in A
;*********************************************************************
GetChar:     brset  SCI0SR1,RDRF,RxReady ;exit loop when RDRF=1
             bra    GetChar              ;loop till RDRF set
RxReady:     ldaa   SCI0DRL              ;read character into A
             rts                         ;return

;*********************************************************************
;* PutChar - sends the character in A out SCI0
;*
;* Calling convention:
;*            ldaa    data          ;character to be sent
;*            bsr    PutChar
;*
;* Returns: nothing (A unchanged)
;*********************************************************************
PutChar:     brclr   SCI0SR1,TDRE,PutChar ;wait for Tx ready
             staa    SCI0DRL       ;send character from A
             rts

;*********************************************************************
;* CheckModule - check in what memory type the address in IX points to
;*  The location may be RAM, FLASH, EEPROM, or a register
;*  if the vector table is addresses, IX is changed to point to the
;*  same vector in the pseudo vector table
;*  returns in B: 1 FLASH or EEPROM
;*                0 RAM or register (all the rest of the address space)
;*               -1 access denied (monitor or pseudo vector)
;*  all registers are preserved except B
;*********************************************************************
CheckModule: pshd                 ;preserve original data
             cpx    #RomStart
             blo    check4EE      ;skip if not flash
             cpx    #VectorTable
             bhs    isVector      ;is it in the real vector table
             cpx    #PVecTable
             blo    isToProgram   ;pseudo vector table or monitor area
             ldab   #0xFF          ;access denied (N=1, Z=0)
             puld                 ;restore original data (D)
             rts

isVector:    leax   BootStart,x   ;access pseudo vector table
             bra    isToProgram

check4EE:    brclr  MEMSIZ0,eep_sw1+eep_sw0,isRAM  ;Check if device has EEprom
			 cpx   #EEpromStart
             blo    isRAM         ;treat as RAM or registers
			 cpx   #EEpromEnd	  ;Greater than allocated EE space?
             bhi    isRAM         ;must be registers or RAM
isToProgram: ldab   #1            ;set flgs - signal FLASH (N=0, Z=0)
             puld                 ;restore original data (D)
             rts

isRAM:       clrb                 ;signal RAM  (N=0, Z=1)
             puld                 ;restore original data (D)
             rts

;*********************************************************************
;* WriteD2IX - Write the data in D (word) to the address in IX
;*  The location may be RAM, FLASH, EEPROM, or a register
;*  if FLASH or EEPROM, the operation is completed before return
;*  IX and A preserved, returns Z=1 (.EQ.) if OK
;*
;*********************************************************************
WriteD2IX:   pshx                 ;preserve original address
             pshd                 ;preserve original data
             bsr    CheckModule
             bmi    ExitWrite     ;deny access (monitor or pseudo vector)
             beq    isRAMword
             cpd    0,x           ;FLASH or EEPROM needs programming
             beq    ExitWrite     ;exit (OK) if already the right data
             pshd                 ;temp save data to program
             tfr    x,b           ;low byte of target address -> B
             bitb   #1            ;is B0 = 1?
             bne    oddAdrErr     ;then it's odd addr -> exit
             ldd    0,x           ;0xFFFF if it was erased
             cpd    #0xFFFF        ;Z=1 if location was erased first
oddAdrErr:   puld                 ;recover data, don't change CCR
             bne    ExitWrite     ;exit w/ Z=0 to indicate error
             bra    DoProgram

isRAMword:   std    0,x           ;write to RAM or register
             clra                 ;force Z=1 to indicate OK
             bra    ExitWrite

;*********************************************************************
;* WriteA2IX - Write the data in A (byte) to the address in IX
;*  The location may be RAM, FLASH, EEPROM, or a register
;*  if FLASH or EEPROM, the operation is completed before return
;*  IX and A preserved, returns Z=1 (.EQ.) if OK
;*
;* Note: Byte writing to the FLASH and EEPROM arrays is a violation
;*       of the HC9S12 specification. Doing so, will reduce long term
;*       data retention and available prog / erase cycles
;*
;*********************************************************************

WriteA2IX:   pshx                 ;preserve original address
             pshd                 ;preserve original data
             bsr    CheckModule
             bmi    ExitWrite     ;deny access (monitor or pseudo vector)
             beq    isWRAMbyte
             cmpa   0,x           ;FLASH or EEPROM needs programming
             beq    ExitWrite     ;exit (OK) if already the right data
             ldab   0,x           ;0xFF if it was erased
             incb                 ;Z=1 if location was erased first
             bne    ExitWrite     ;exit w/ Z=0 to indicate error

             tfr    x,b           ;test least significant bit
             bitb   #1            ;is B0 = 1?
             bne    isOddAdr      ;then it's odd addr.
isEvenAdr:   ldab   1,x           ;low byte of D (A:B) from memory
             bra    DoProgram
isOddAdr:    tab                  ;move to low byte of D (A:B)
             dex                  ;point to even byte
             ldaa   ,x            ;high byte of D (A:B) from memory
             bra    DoProgram

isWRAMbyte:  staa   0,x           ;write to RAM or register
             clra                 ;force Z=1 to indicate OK
             bra    ExitWrite

; Programs D to IX in either FLASH or EEPROM
DoProgram:   bsr    abClr         ;abort commands and clear errors
             cpx    #RomStart     ;simple test only
             blo    itsEE         ; details already verified
             bsr    ProgFlash     ;program the requested location
             bra    ExitWrite     ;exit (Z indicates good or bad)
itsEE:     ;disabled       bsr    ProgEE        ;program the requested location
; exit Write?2IX functions (Z indicates good or bad)
ExitWrite:   puld                 ;restore original data (D)
             pulx                 ;restore original address (IX)
             rts


;
; utility sub to abort previous commands in flash and EEPROM
; and clear any pending errors
;
abClr:       psha
             ldaa    #PVIOL+ACCERR ;mask
;             staa    ESTAT         ;abort any command and clear errors
             staa    FSTAT         ;abort any command and clear errors
             pula
             rts

;*********************************************************************
;* Progflash - programs one byte of HCS9S12 FLASH
;*  This routine waits for the command to complete before returning.
;*  assumes location was blank. This routine can be run from FLASH
;*
;* On entry... IX - points at the FLASH byte to be programmed
;*             A holds the data for the location to be programmed
;*
;* Calling convention:
;*           bsr    Prog1flash
;*
;* Uses: DoOnStack which uses SpSub
;* Returns: IX unchanged and A = FSTAT bits PVIOL and ACCERR only
;*  Z=1 if OK, Z=0 if protect violation or access error
;*********************************************************************
ProgFlash:   pshd
             cpx   #0x8000         ; if <0x8000 then bank 3E
             blo    its3E         ;set ppage to 3E
             cpx   #0xC000         ; if > 0xBFFF then bank 3F
             blo    ProgFlash1    ;set ppage 3F
             movb  #0x3F,PPAGE     ;
             bra   ProgFlash1
its3E:       movb  #0x3E,PPAGE     ;

ProgFlash1:  ldab   PPAGE
             lsrb                 ; calculate the value of the block select bits based
             lsrb                 ; on bits 3:2 of the PPAGE register value. (<256k)
             ldy   #SectorSize   ; get high byte of size
             cpy   #0x0200         ; if larger than 0x200 shift again
             beq    nBlockLoopb
             lsrb                 ; on bits 4:3 of the PPAGE register value. (512k)

nBlockLoopb: comb
             andb  #0x03           ; mask off all but the lower 2 bits.
             stab   FCNFG         ; select the block to program.
             cmpb  #0x00           ; if block zero use DoOnStack method
             puld
             beq    ProgFlashSP

ProgFlshRom: std    ,x            ;latch address & data to program
             ldaa  #ProgWord         ;Select program word command
             staa   FCMD          ;issue byte program command
;             ldaa  #CBEIF
             bsr    SpSub         ;register command & wait to finish
             ldaa   FSTAT
             anda  #0x30           ;mask all but PVIOL or ACCERR
             rts

ProgFlashSP: std    ,x            ;latch address and data
             ldaa  #ProgWord         ;Select program word command
             staa   FCMD          ;issue byte program command
;
; DoOnStack will register the command then wait for it to finish
;  in this unusual case where DoOnStack is the next thing in program
;  memory, we don't need to call it. The rts at the end of DoOnStack
;  will return to the code that called Prog1flash.
;

;*********************************************************************
;* DoOnStack not required as all of this is running from RAM, so simply use
;* SpSub - register flash command and wait for Flash CCIF
;*********************************************************************
SpSub:
	     tfr    ccr,b		  ;get copy of ccr
	     orcc  #0x10			  ;disable interrupts
             movb   #CBEIF,FSTAT   ;[PwO] register command
             nop                  ;[O] wait min 4~ from w cycle to r
             nop                  ;[O]
             nop                  ;[O]
SpSub2:      brclr  FSTAT,CCIF,SpSub2 ;[rfPPP] wait for queued commands to finish
             tfr	b,ccr		  ;restore ccr and int condition
             ldaa   FSTAT         ;get result of operation
             anda  #0x30           ;and mask all but PVIOL or ACCERR
             rts                  ;back into DoOnStack in flash
SpSubEnd:

;*********************************************************************
;* ISRHandler this routine checks for unprogrammed interrupt
;*  vectors and returns an 0xE3 error code if execution of an
;*  unprogrammed vector is attempted
;*********************************************************************
;ISRHandler:  pulx    ;pull bsr return address off stack
;XXX             ldy     (PVecTable-BSRTable-2),X
;             cpy     #0xFFFF
;             beq     BadVector
;             jmp     ,Y

ramEndMonitor:

.nolist                      ;skip the symbol table

;*****************************************************************

