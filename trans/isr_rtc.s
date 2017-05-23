;*********************************************************************
; ISR_Timer_Clock
;*********************************************************************
; $Id: isr_rtc.s,v 1.10 2014/12/27 21:25:18 jsm Exp $

.sect .text
.globl ISR_Timer_Clock

             nolist               ;turn off listing
             include "ms2extrah.inc"
             list                 ;turn listing back on

;**************************************************************************
; "0.1ms" section periodic interrupt
; maintains clocks amongst other functions
; **************************************************************************

ISR_Timer_Clock:
;    // .128 ms clock interrupt - clear flag immediately to resume count
   movb    #128, CRGFLG ; clear RTI interrupt flag

;    // also generate 1.024 ms, .10035 sec and 1.0035 sec clocks
   ldd	    lmms+0x2 ; free running clock(.128 ms tics) good for ~ 110 hrs
   ldx	    lmms
   addd     #1
   bcc      lm3a
   inx
lm3a:
   std      lmms+0x2
   stx      lmms

;    mms++;    // in .128 ms tics - reset every 8 tics = 1.0024 ms
   inc      mms
; "stall" timers
   ldd      vss1_stall
   addd     #1
   std      vss1_stall

   ldd      stall_ess
   addd     #1
   std      stall_ess

   ldd      timer_41te
   addd     #1
   std      timer_41te

    ldx     paddle_timer
    beq     pad_tim_zero
    dex
    stx     paddle_timer
pad_tim_zero:

;vss out
    ldab    pin_vssout
    beq     end_vssout
    ldab    flash4.vssout_opt
    andb    #0xc0
    cmpb    #0xc0
    bne     vssout_timed
    ldab    PORTT
    bitb    #1
    bne     end_vssout
    ldx     port_vssout
    ldab    pin_vssout
    eorb    #0xff
    andb    0,x
    stab    0,x
    bra     end_vssout

vssout_timed:
    ldx     vssout_match
    beq     end_vssout
    ldx     vssout_cnt
    inx
    cpx     vssout_match
    blo     vssout_stx
    ldy     port_vssout
    ldab    0,y
    eorb    pin_vssout  ; flip the bit
    stab    0,y
    ldx     #0
vssout_stx:
    stx     vssout_cnt
end_vssout:

; swpwm outputs
    ldy     #0
generic_pwm:
    ldaa     pin_swpwm,y
    beq      end_pwm
    cmpa     #255 ; see if the magic number
    bne      gp1
;CANPWMs handled in mainloop
    bra      end_pwm

gp1:
    tfr     y,x
;    lslx                 ; Y doubled
    xgdx
    lslb
    rola
    xgdx
;    decw    gp_clk,x
    ldd     gp_clk,x
    subd    #1
    std     gp_clk,x
    bne     end_pwm

; now decide whether this is an on or off event
    brclr   gp_stat,y, #2, gen_pwm_off

gen_pwm_on:
    ldd     gp_max_on,x
    beq     gen_pwm_off  ; 0% duty
    std     gp_clk,x ; next target timer
    bclr    gp_stat,y, #2

    ldx     port_swpwm,x
    ldaa    0,X
    oraa    pin_swpwm,y
    staa    0,X
    bra     end_pwm

gen_pwm_on_sanity:
    ldd     gp_max_on,x
    bne     gen_pwm_on
    bra     end_pwm  ; BOTH are zero...oops

gen_pwm_off:
    ldd     gp_max_off,x
    beq     gen_pwm_on_sanity  ; 100% duty
    std     gp_clk,x ; next target timer
    bset    gp_stat,y, #2

    ldx     port_swpwm,x
    ldab    pin_swpwm,y
    comb
    andb    0,X 
    stab    0,X

end_pwm:
    iny
    cmpy    #NUM_SWPWM
    bne     generic_pwm

;    // check for re-enabling IC interrupt
; other method for timers
   brclr   flagbyte0,#flagbyte0_vss,LM26
   movb    #1,TFLG1  ; clear vss interrupt
   bset    TIE,#1  ; re-enable vss interrupt
LM26:
   brclr   flagbyte0,#flagbyte0_tach5,LM27
   movb    #0x10,TFLG1  ; clear tach5 interrupt
   bset    TIE,#0x10  ; re-enable tach5 interrupt
LM27:
   brclr   flagbyte0,#flagbyte0_tach2,LM28
   movb    #4,TFLG1  ; clear tach2 interrupt
   bset    TIE,#4  ; re-enable tach2 interrupt
LM28:

L47:

;;;; start of clocks section

rtc_clocks:
; are we within 2 rtc ticks of the overflow? if so, let IC know
   ldd TCNT
   cpd #0xFE7F
   blo  really_rtc_clocks
   bset flagbyte1,#flagbyte1_ovfclose

really_rtc_clocks:
;create 0.128ms period timer like MS1 did. Used for tacho out
   ldx     lowres_ctr
   cpx     #0xffff
   beq     no_low
   inx
   stx     lowres_ctr
no_low:

; Check for CAN transmit instead of interrupts
    tst     can_tx_num
    beq     end_cantxchk
    ldab    CANTFLG
    andb    #CANTFLG_MASK
    beq     end_cantxchk
    STACK_SOFT
    jsr    can_do_tx ; not far
    UNSTACK_SOFT
end_cantxchk:

;    // check mms to generate other clocks
   ldaa	   mms
   cmpa    #7   ; reset every 8 ticks
   bls     CLK_DONE
   clr     mms

   ldaa    millisec
   adda    #1    ; // actually 1.024 ms
   staa    millisec

   ldd      shift_timer
   addd     #1
   std      shift_timer

   ldx      shift_delay
   beq      no_shift_del
   dex
   stx      shift_delay
no_shift_del:

   ldx      shiftret_timer
   beq      no_shift_ret
   dex
   stx      shiftret_timer
no_shift_ret:

   ldab    adc_ctr
   addb    #1
   cmpb    #10   ; every 10.24ms

   bne     milli_cont

; 10ms clock for lockup counter
   tst    lockupcount
   beq    nolckc
   dec    lockupcount
nolckc:

   clrb

milli_cont:
   stab    adc_ctr
   ldaa    millisec
   cmpa    #98
   bls     CLK_DONE

LM116:
   cmpa    #99
   bls     CLK_DONE
   clr     millisec

;(1/10) tenth of second section
   tst    resetholdoff
   beq    norho
   dec    resetholdoff
norho:

   ldaa     tenthsec
   adda	    #1
   cmpa	    #2   ; 200ms
   bne	    tenthsec_cont

   clr      tenthsec
   bra	    CLK_DONE

tenthsec_cont:
   staa	    tenthsec
   clra

CLK_DONE:

   ldx    TC_ovflow  ;    // Get display seconds from continuously running TCNT
   inx
   stx    TC_ovflow
   cpx    #7813
   blo    DONE1s
   clr    TC_ovflow
   clr    TC_ovflow+1
   ldx    outpc.seconds ;        // update seconds to send back to PC
   inx
   stx    outpc.seconds

DONE1s:

   rti

