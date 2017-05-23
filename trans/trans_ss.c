/* $Id: trans_ss.c,v 1.3 2014/12/27 21:25:18 jsm Exp $ */
#include "trans.h"

// speed sensor ISR - vehicle and engine

INTERRUPT void ISR_vss(void)  {

    unsigned int TC0this;
    unsigned long  TC0_32bits, vss2;
    static unsigned int TC0_last, swtimer_last;
    static unsigned long  TC0_32last;

    TFLG1 = 0x01;	             // clear IC interrupt flag
    TIE |= 0x01;		     // re-enable IC interrupt
    TC0this = TC0;

    TC0_32bits = ((unsigned long)swtimer<<16) | TC0this;
    if ((flagbyte1 & flagbyte1_ovfclose) && ((TC0this < 0x1000) ||
	    ((TC0this < TC0_last) && (swtimer == swtimer_last)))) {
	TC0_32bits += 0x10000;
    }

    vss2 = TC0_32bits - TC0_32last;

    if (vss2 < 20) {
        /* looks like a noise pulse, too short */
        return;
    }
    TC0_32last = TC0_32bits;

    TC0_last = TC0this;
    swtimer_last = swtimer;
    vss1_teeth++;
	vss1_time_sum += vss2;

    if (vss1_time_sum > VSS_TIME_THRESH) {
        flagbyte0 |= FLAGBYTE0_SAMPLE_VSS1;
    }

    if (pin_vssout && ((pg4_ptr->vssout_opt & 0xc0) == 0xc0)) {
        *port_vssout |= pin_vssout; /* interrupts already masked */
    }
	vss1_stall = 0;
    return;
}

INTERRUPT void ISR_tach5(void)  {

    unsigned int TC5this;
    unsigned long  TC5_32bits, ess2;
    static unsigned int TC5_last, swtimer2_last;
    static unsigned long  TC5_32last;

    TFLG1 = 0x20;	             // clear IC interrupt flag
//	TIE |= 0x20;		     // re-enable IC interrupt
	TC5this = TC5;

    TC5_32bits = ((unsigned long)swtimer<<16) | TC5this;
    if ((flagbyte1 & flagbyte1_ovfclose) && ((TC5this < 0x1000) ||
	((TC5this < TC5_last) && (swtimer == swtimer2_last)))) {
	TC5_32bits += 0x10000;
    }

    ess2 = TC5_32bits - TC5_32last;
    TC5_last = TC5this;
   	TC5_32last = TC5_32bits;
    swtimer2_last = swtimer;
	ess = (ess + ess2) >>1;

	if (ess2 < 20) { // unbelievably fast, must be noise, so block interrupt for a short period of time
		flagbyte0 |= flagbyte0_tach5;
	} else {
		TIE |= 0x20;
	}
	stall_ess = 0;
    return;
}

INTERRUPT void ISR_tach2(void)  {

    unsigned int TC2this;
    unsigned long  TC2_32bits, ess2;
    static unsigned int TC2_last, swtimer3_last;
    static unsigned long  TC2_32last;

	TFLG1 = 0x04;	             // clear IC interrupt flag
//	TIE |= 0x04;		     // re-enable IC interrupt
	TC2this = TC2;

    TC2_32bits = ((unsigned long)swtimer<<16) | TC2this;
    if ((flagbyte1 & flagbyte1_ovfclose) && ((TC2this < 0x1000) ||
	((TC2this < TC2_last) && (swtimer == swtimer3_last)))) {
	TC2_32bits += 0x10000;
    }

    ess2 = TC2_32bits - TC2_32last;
    TC2_last = TC2this;
   	TC2_32last = TC2_32bits;
    swtimer3_last = swtimer;
	ess = (ess + ess2) >>1;

	if (ess2 < 20) { // unbelievably fast, must be noise, so block interrupt for a short period of time
		flagbyte0 |= flagbyte0_tach2;
	} else {
		TIE |= 0x04;
	}
	stall_ess = 0;
    return;
}
