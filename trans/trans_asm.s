;*********************************************************************
; Misc asm
;*********************************************************************
; $Id: trans_asm.s,v 1.1 2013/01/18 00:57:06 jsm Exp $

.sect .text
.globl UnimplementedISR, g_read_copy

             nolist               ;turn off listing
             include "s12asmdefs.inc"
             include "ms2extrah.inc"
             include "ms2extra_structs.inc"
             list                 ;turn listing back on

;**************************************************************************
; Miscellaneous code in asm
; **************************************************************************
UnimplementedISR:
   ; Unimplemented ISRs trap
   rti

g_read_copy:
; (src, count, dest)
; copies from flash memory to local memory
; on entry D is source address
; first word off stack is count in bytes
; second word off stack is dest address
; returns 1 if changed
; sets/clears interrupt flag
    tfr     d,x  ; source
    ldy     4,sp ; dest
    clr     5,sp ; stat
grc_lp:
    ldd     2,sp ; count
    cmpd    #4
    blo     grc2

;4 bytes
    subd    #4
    std     2,sp

    ldd     2,x+
    cmpd    0,y
    beq     grc1k
    movb    #1,5,sp
grc1k:
    sei
    std     2,y+

    ldd     2,x+
    cmpd    0,y
    beq     grc1k2
    movb    #1,5,sp
grc1k2:
    std     2,y+
    cli
    bra     grc_lp

grc2:
    cmpd    #2
    blo     grc3

;2 bytes
    subd    #2
    std     2,sp

    ldd     2,x+
    cmpd    0,y
    beq     grc2k
    movb    #1,5,sp
grc2k:
    std     2,y+
    bra     grc_lp

grc3:
    cmpd    #1
    blo     grc4
;1 byte
    subd    #1
    std     2,sp

    ldaa    1,x+
    cmpa    0,y
    beq     grc3k
    movb    #1,5,sp
grc3k:
    staa    1,y+
    bra     grc_lp

grc4:
    ldab    5,sp
    rts

