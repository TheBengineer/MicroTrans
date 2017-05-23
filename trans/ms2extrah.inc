;*********************************************************************
; MS2 Extra equates for asm   (C) 2006 James Murray
;*********************************************************************
;global vars used in asm
; $Id: ms2extrah.inc,v 1.2 2013/01/17 13:20:32 jsm Exp $

.globl pPTMpin, PTT, PTM, CRGFLG, PORTA, PORTB, PORTM, PORTT

.globl next_txmode, txmode, txgoal, rcv_timeout, rd_wr, txcnt, txbuf, CANid, tbl_idx
.globl page, tables, ram_data
.globl vfy_fail, rd_wr, CANid, ibuf, next_txmode, stall_timeout, TC_ovflow
.globl flagbyte2, lmms
.globl lowres, lowres_ctr, tacho_targ

.globl  stall_ess,stall_vss

.equ MAX_CANBOARDS, 16
.equ NO_TBLES, 10
.equ SIZEOFTXBUF, 52  ; be sure to update ms2_extra.h as well
.equ flagbyte0_vss,        0x01
.equ flagbyte0_tach5,      0x02
.equ flagbyte0_tach2,      0x04

.equ flagbyte1_tstmode,     0x02
.equ flagbyte1_ovfclose,    0x04

.equ flagbyte2_MV2,         0x80

.equ flagbyte3_kill_cat,    0x02
.equ flagbyte3_getcandat,   0x08
.equ flagbyte3_sndcandat,   0x10

.macro STACK_SOFT
;these are the minimum to stack
movw 0x3000, 2, -SP
movw 0x3002, 2, -SP
movw 0x3004, 2, -SP
.endm

.macro UNSTACK_SOFT
;these are the minimum to stack
movw 2, SP+, 0x3004
movw 2, SP+, 0x3002
movw 2, SP+, 0x3000
.endm

.include "trans_defines.inc"
.include "s12asmdefs.inc"
.include "ms2extra_structs.inc"
