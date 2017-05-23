;*********************************************************************
; ISR_TimerOverflow
;*********************************************************************
; $Id: isr_timerovf.s,v 1.1 2013/01/16 23:48:29 jsm Exp $

.sect .text
.globl ISR_TimerOverflow
             nolist               ;turn off listing
             include "s12asmdefs.inc"
             include "ms2extrah.inc"
             include "ms2extra_structs.inc"
             list                 ;turn listing back on
;*********************************************************************

ISR_TimerOverflow:
;  acknowledge interrupt and clear flag
   movb	#128, TFLG2

   ldx    swtimer  ; // top word to make 32 bit timer. software:hardware
   inx
   stx    swtimer
   bclr   flagbyte1, #flagbyte1_ovfclose

   rti
