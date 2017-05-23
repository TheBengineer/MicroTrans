;*********************************************************************
; MS2 Extra equates for asm   (C) 2006 James Murray
;*********************************************************************
; This file used to allow linker to understand structs until we find
; a better way. Pretty horrible.
; $Id: ms2extra_structs.inc,v 1.4 2014/12/27 21:25:18 jsm Exp $

.globl outpc
.equ outpc.seconds,        outpc+0x0
.equ outpc.status1,        outpc+0x1e
.equ outpc.status4,        outpc+35
.equ outpc.status5,        outpc+38
.equ outpc.status6,        outpc+42

.globl flash4
.equ flash4.mycan_id,      flash4+0x6c
.equ flash4.setting1,      flash4+0x6e
.equ flash4.vssout_opt,    flash4+315

.globl ram_data
.equ ram_data.tst_set,     ram_data+113
.equ ram_data.tst_lock,    ram_data+114



