; ISR_SCI_COMM
;*********************************************************************
; $Id: isr_sci.s,v 1.4 2014/12/20 19:59:53 jsm Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
; *
; * This file is derived from Megasquirt-3.
; *
; * Origin: Al Grippo
; * Major: Recode in ASM, new features.
; * Trash most of code and move to mainloop: James Murray
; * Majority: James Murray
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .text
.globl ISR_SCI_Comm

             nolist               ;turn off listing
             include "ms2extrah.inc"
             list                 ;turn listing back on

; Serial communications. Revised for MS3 1.1
; All serial is packet based.
; Interrupt RX code fills buffer
; Interrupt TX code sends buffer
; All processing is handled by serial() in mainloop code
; Envelope format is
; [2 bytes big endian size of payload] [payload] [big-endian CRC32]

ISR_SCI_Comm:
    ; if RDRF register not set, => transmit interrupt
    ldab	SCI0SR1
    bitb    #0x0f
    bne     rx_err   ; Overrun, noise, framing error, parity fail -> kill serial
    bitb    #32      ; check RDRF to see if receive data available
    bne     is1
    bitb    #128      ; check TDRE to see if space to xmit
    lbne    XMT_INT

; dunno... exit
    ldab    SCI0DRL     ; read assumed garbage data to clear flags
    bra     is1_rti

;shouldn't get here

rx_err:
    bitb    #1
    bne     rx_err_pf
    bitb    #2
    bne     rx_err_fe
    bitb    #4
    bne     rx_err_nf
    bitb    #8
    bne     rx_err_or
    clr     next_txmode ; shouldn't happen..
    bra     srl_abort

rx_err_pf:
    movb    #1, next_txmode
    bra     srl_abort
rx_err_fe:
    movb    #2, next_txmode
    bra     srl_abort
rx_err_nf:
    movb    #3, next_txmode
    bra     srl_abort
rx_err_or:
    movb    #4, next_txmode
    bra     srl_abort

xsdata:
    movb    #6, next_txmode
    inc     srl_err_cnt
    bra     srl_abort

srl_abort:
    clr    rxmode
    clr    txmode
    ldab   SCI0DRL     ; read assumed garbage data to clear flags
    bclr   SCI0CR2,#0xAC;   // rcv, xmt disable, interrupt disable
    bset   flagbyte3,#flagbyte3_kill_srl
    movw   lmms+2, srl_timeout
    jmp    isL4           ; bail out
;-------------------------------
is1:
    ; Receive Interrupt
    ; Clear the RDRF bit by reading SCISR1 register (done above), then read data
    ;  (in SCIDRL reg).
    ; Check if we are receiving new input parameter update
    ldab   SCI0DRL   ; always read the data first

    ldaa   txmode
    bmi    rxerrtxmode   ; should not be receiving when txmode >= 128 - i.e. transmitting
    bra    is1ckmode

rxerrtxmode:
;    movb    #5, next_txmode
;  swallow the erroneous byte, then bail
    jmp    isL4

;--------------
is1ckmode:
    movw   #0xffff, rcv_timeout+0x2
    movw   #0xffff, rcv_timeout

is1ckm1:
    ldaa   rxmode
    beq    is1case0
    cmpa   #1
    beq    is1case1
    cmpa   #2
    beq    is1case2
;should not be reached
    jmp    srl_abort

is1case0:
;    movw    lmms+2, outpc.istatus5  ; debug for timing burn time
    clr     txgoal
    clr     txgoal+1
    stab    srlbuf
    bra     is1itx

is1case1:
    stab    srlbuf+1
    ldd     srlbuf
    beq     xsdata              ; zero length is invalid
    cmpd    #0x6100             ; start of "a 00 06"
    beq     mv1ok
    cmpd    #0x7200             ; start of "r 00 04"
    beq     mv1ok
    cmpd    #SRLDATASIZE + SRLHDRSIZE - 6    ; check for buffer overflow (-6 because the size number does not include the size or CRC)
    bhi     xsdata
mv1ok:
    clr     rxoffset
    clr     rxoffset+1
    bra     is1itx

is1case2:
    ldx     rxoffset
    stab    srlbuf+2,x

    cpx     #SRLDATASIZE + SRLHDRSIZE -1
    bhs     is1c3 ; prevent overflow

    inx
    stx     rxoffset
is1c3:

    ldd     srlbuf ; the length header we've been sent
    addd    #4
    cpd     rxoffset    
    bhi     isL4
    clr     rxmode
    bset    flagbyte14, #FLAGBYTE14_SERIAL_PROCESS

    jmp     is1_rti

is1itx:   ; common piece of code
    inc     rxmode

isL4:
    ldd     lmms+0x2
    ldx     lmms
    addd    #195 ; 25ms ; was 781 = 100ms
    bcc     isL4a
    inx
isL4a:
    std     rcv_timeout+0x2
    stx     rcv_timeout
    jmp     is1_rti

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;    // Transmit Interrupt
XMT_INT:

;    // Clear the TDRE bit by reading SCISR1 register (done), then write data
    ldx     txcnt
    inx
    stx     txcnt

    cpx     txgoal
    bcc    txmcl

    ldab    txmode

    beq     err_txmode0 ; txmode = 0,   shouldn't be here
    bpl     err_txmode12 ; txmode < 128, shouldn't be here

    cpx     #SRLDATASIZE + SRLHDRSIZE
    blo     xmt2
    ldx     #SRLDATASIZE + SRLHDRSIZE - 1 ; rail at last byte

xmt2:
    ; send back from buffer
    ldaa    srlbuf,x
    staa    SCI0DRL

    bra     is1_rti

err_txmode0:
    bra     is1_rti

err_txmode12:
    movb    #5, next_txmode
    jmp     srl_abort

txmcl:
    clr     txmode
    clr     txcnt
    clr     txcnt+1
    bclr    SCI0CR2, #0x88 ; xmit disable & xmit interrupt disable
    bset    SCI0CR2, #0x24   ; re-enable received interrupt and receiver

is1_rti:
    rti

