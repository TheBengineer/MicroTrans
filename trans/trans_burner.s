; $Id: trans_burner.s,v 1.3 2014/09/11 14:48:06 jsm Exp $
.sect .text
.globl burntbl

;.nolist
;include "s12asmdefs.inc"
include "ms2extrah.inc"
;.list

;*********************************************************************
; burn a ram table to flash
; ram table always at same place ram_data
; flash location in lookup table
; argument in D gives offset into flash table (used for calibration tables)
;
;*********************************************************************
burntbl:
       pshd
       ldaa    flocker
       cmpa    #0xcc
       bne     burnend
burnok:
       ldab    burn_idx
       ldaa    #6
       mul
       tfr      d,x
       ldd      tables+2,x ; get flash address
       addd     0,sp ; original D value
       cpd      #0
       beq      burnend
       tfr      d,x

       brclr   FSTAT,#CBEIF,.   ; loop until no command in action 
;erase the 1k sector
       ldaa    FSTAT
       anda    #0x30
       beq     burnerase
       movb    #ACCERR|PVIOL,FSTAT
burnerase:
       ldd     #0xffff
       std     0,x
       movb    #SecErase,FCMD
       ldaa    #CBEIF
       jsr     RamBurnPgm

       ldy     #0  ; burn loop counter

burnlp:
       movb    #ACCERR|PVIOL,FSTAT
       ldd     ram_data,y  ; load/write a word at a time
       std     0,x
       movb    #ProgWord,FCMD
       ldaa    #CBEIF
       jsr     RamBurnPgm

nop  ; provide BP ability in here
       inx
       inx
       iny
       iny
       cmpy    #0x400
       bne     burnlp
burnend:
nop  ; provide BP ability in here
       puld
       rts
