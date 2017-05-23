/* $Id: trans_misc.c,v 1.17 2015/03/11 14:52:05 jsm Exp $ */
#include "trans.h"
/*
 * generic_pwm_out()
    Origin: James Murray
    Majority: James Murray
*/
//get_adc should be placed in 0x3c page along with the clt, mat, ego, maf data tables
void get_adc(char chan1, char chan2)
{
    char chan;

    for (chan = chan1; chan <= chan2; chan++)  {
	    if (chan == 0) {
	        outpc.adc[0] = (ATD0DR0 + outpc.adc[0])>>1;
	    } else if (chan == 1) {
	        outpc.adc[1] = ATD0DR1; // switch A input - some trans
	    } else if (chan == 2) {
	        outpc.adc[2] = ATD0DR2; // switch B input - some trans
	    } else if (chan == 3) {
	        outpc.adc[3] = (ATD0DR3 + outpc.adc[3])>>1;
	    } else if (chan == 4) {
	        outpc.adc[4] = (ATD0DR4 + outpc.adc[4])>>1;
	    } else if (chan == 5) {
	        outpc.adc[5] = ATD0DR5; // switch C input - some trans
	    } else if (chan == 6) {
	        outpc.adc[6] = (ATD0DR6 + outpc.adc[6])>>1;
	    } else if (chan == 7) {
	        outpc.adc[7] = (ATD0DR7 + outpc.adc[7])>>1;

	    }			 // end of switch
    }				 // end of for loop

    if (pg4_ptr->setting1 & SETTING1_LOADMAP) {
        outpc.load = outpc.map;
    } else {
        outpc.load = outpc.tps;
    }

    return;
}

//*****************************************************************************
//* Function Name: Flash_Init
//* Description : Initialize Flash NVM for HCS12 by programming
//* FCLKDIV based on passed oscillator frequency, then
//* uprotect the array, and finally ensure PVIOL and
//* ACCERR are cleared by writing to them.
//*
//*****************************************************************************
void Flash_Init()  {
  /* Next, initialize FCLKDIV register to ensure we can program/erase */
  FCLKDIV = 39;
  FSTAT = PVIOL|ACCERR;/* Clear any errors */
  return;
}

void calc_coeff() {
    unsigned int fdratio;
    Rpm_Coeff = 180000000 / flash4.divider;
    inpRpm_Coeff = 90000000 / flash4.inteeth;

    /* VSS input */
    if ((flash4.vss_pos & 0x03) == 0x02) {
        /* pulses per mile */
        vss1_coeff = 321868800UL / flash4.vss1_can_scale;
        vss1_coeff *= 75; /* convert from MS3 units (50us) to trans units (2/3us) */
    } else if ((flash4.vss_pos & 0x03) == 0x03) {
        /* pulses per km */
        vss1_coeff = 200000000UL / flash4.vss1_can_scale;
        vss1_coeff *= 75; /* convert from MS3 units (50us) to trans units (2/3us) */
    } else {
        /* use ratios, wheel diameter etc. */
        // scaling factor to give m/s * 10 (required so rolling average works)
        vss1_coeff = 4710584; //6281100;

        if (flash4.vss_pos & 1) { // vss1 on driveline
            fdratio = flash4.fdratio;
        } else {
            fdratio = 100;
        }

        vss1_coeff = vss1_coeff / fdratio; // to avoid overflow
        vss1_coeff = (vss1_coeff * (unsigned long) flash4.wheeldia) /
             (unsigned long) flash4.reluctorteeth;
    }
    vss1_stall = VSS_STALL_TIMEOUT + 1;

    /* Output shaft RPM */
    if ((flash4.vss_pos & 0x03) == 0x02) { /* ppm or ppk */
        outRpm_Coeff = 307362063UL / flash4.vss1_can_scale; /* *3/PI */
        outRpm_Coeff *= 1500UL; /* convert from MS3 units (50us) to trans units (2/3us) * 20 */
        outRpm_Coeff /= flash4.wheeldia;
        outRpm_Coeff *= flash4.fdratio;
    } else if ((flash4.vss_pos & 0x03) == 0x03) { /* ppk */
        outRpm_Coeff = 190985932UL / flash4.vss1_can_scale; /* *3/PI */
        outRpm_Coeff *= 1500UL; /* convert from MS3 units (50us) to trans units (2/3us) * 20 */
        outRpm_Coeff /= flash4.wheeldia;
        outRpm_Coeff *= flash4.fdratio;
    } else if ((flash4.vss_pos & 0x03) == 0x01) { /* driveline */
        outRpm_Coeff = 90000000UL / flash4.reluctorteeth;
    } else { /* wheel speed */
        outRpm_Coeff = (900000UL * flash4.fdratio) / flash4.reluctorteeth;
    }
}

unsigned int shift_delay_calc(int load, unsigned char a, unsigned char b)
{
    int delay, delay1, delay2;
    
    delay1 = (int) pg4_ptr->shift_delay[0][a][b];
    delay2 = (int) pg4_ptr->shift_delay[1][a][b];
    if (load > 1000) {
        load = 1000;
    } else if (load < 0) {
        load = 0;
    }

    delay = delay2 - delay1;
    delay = ((long)delay * load) / 1000;
    delay += delay1;

    return (unsigned int)delay;
}

unsigned int do_testmode(void)
{
    /* return codes :
        0 = ok
        1 = BUSY (not allowed)
        2 = invalid mode
     */

    if (datax1.testmodemode == 0) {
        flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
        outpc.status3 &= ~STATUS3_TESTMODE;
        testmode_glob = 0;
    } else if (datax1.testmodemode == 1) {
        flagbyte1 |= flagbyte1_tstmode; /* enable test mode */
        outpc.status3 |= STATUS3_TESTMODE;
        testmode_glob = 0;
        // accept the test mode and return ok
        return 0;
    } else if (flagbyte1 & flagbyte1_tstmode) {
        if (datax1.testmodemode == 2) { // Gear test
            // disable the mode
            testmode_glob &= ~1;
            return 0;
        } else if (datax1.testmodemode == 3) { // 1 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 1;
            return 0;
        } else if (datax1.testmodemode == 4) { // 2 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 2;
            return 0;
        } else if (datax1.testmodemode == 5) { // 3 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 3;
            return 0;
        } else if (datax1.testmodemode == 6) { // 4 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 4;
            return 0;
        } else if (datax1.testmodemode == 7) { // EPC test
            // disable the mode
            testmode_glob &= ~2;
            return 0;
        } else if (datax1.testmodemode == 8) {
            // enable the mode
            testmode_glob |= 2;
            return 0;
        } else if (datax1.testmodemode == 9) { // LU test
            // disable the mode
            testmode_glob &= ~4;
            return 0;
        } else if (datax1.testmodemode == 10) {
            // enable the mode
            testmode_glob |= 4;
            return 0;
        } else if (datax1.testmodemode == 11) { // 3-2 test
            // disable the mode
            testmode_glob &= ~8;
            return 0;
        } else if (datax1.testmodemode == 12) {
            // enable the mode
            testmode_glob |= 8;
            return 0;
        } else if (datax1.testmodemode == 13) { // 5 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 5;
            return 0;
        } else if (datax1.testmodemode == 14) { // 6 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 6;
            return 0;
        } else if (datax1.testmodemode == 15) { // 7 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 7;
            return 0;
        } else if (datax1.testmodemode == 16) { // 8 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 8;
            return 0;
        } else if (datax1.testmodemode == 17) { // 9 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 9;
            return 0;
        } else if (datax1.testmodemode == 18) { // 10 test
            // enable the mode
            testmode_glob |= 1;
            testmode_gear = 10;
            return 0;
        }
    }

    return 0;
}

void generic_pwm_outs()
{
    /**************************************************************************
     ** Generic PWM open-loop outputs - calculate on/off times for RTC
     ** handled here so other features (e.g. ALS) can re-use the outputs
     **************************************************************************/
    /* This is a trimmed down version. Original came from MS3. */
    int i;
    for (i = 0; i < sw_pwm_num ; i++) {
        if (pin_swpwm[i]) { // non CANPWMs
            /* figure out PWM parameters for isr_rtc.s */
            unsigned char mult, duty;
            unsigned int max, trig;

            /* Only work in fixed freq, variable duty, 0-255 mode. */
            mult = sw_pwm_freq[i];
            duty = *sw_pwm_duty[i];
            if (mult == 50) {
                max = 156;
                trig = (duty * 156U) / 255U;
            } else {
                /* work at 30.5Hz */
                max = 255;
                trig = duty;
            }
        
            if (gp_stat[i] & 0x40) { // negative polarity
                gp_max_off[i] = trig; // off time
                gp_max_on[i] = max - trig; // on time
            } else { // positive polarity
                gp_max_on[i] = trig; // on time
                gp_max_off[i] = max - trig; // off time
            }

            if ((gp_stat[i] & 1) == 0) {
                gp_clk[i] = 1; // bring it back to earth
                gp_stat[i] |= 1; // enabled
            }
        } else {
            gp_stat[i] &= ~3; // clear bits 0,1
        }
    }
}

void do_lockup(void)
{
        /***************************************************************************
        **
        **  Torque Converter Lockup
        **
        **************************************************************************/
    int itmp1, itmp2;

#define RPM_HYST 300

    /* Determine index in VSS/RPM arrays */
    itmp1 = outpc.gear - 1;
    if (itmp1 < 0) {
        itmp1 = 0;
    } else if (itmp1 > (NUM_TCC_RPMVSS - 1)) {
        itmp1 = NUM_TCC_RPMVSS - 1;
    }
    itmp2 = pg4_ptr->tcc_opt & twopow[itmp1]; /* Whether lockup is allowed in this gear or not */

    if ((pg4_ptr->lockrace & 0x01) && (outpc.tps > pg4_ptr->lockrace_tps)) {
/* Race lockup mode */
        unsigned char allowed_gear;

        outpc.status1 |= STATUS1_RACE;
        if ((pg4_ptr->lockrace & 0x06) == 0x02) {
            allowed_gear = 3;
        } else if ((pg4_ptr->lockrace & 0x06) == 0x04) {
            allowed_gear = 2;
        } else {
            allowed_gear = 4;
        }

        if (outpc.gear >= allowed_gear) {
            itmp2 = 1; /* Over-ride allow flag (may change in future release) */
        } else {
            itmp2 = 0;
        }

	    if ((itmp2 == 0) || (outpc.status1 & STATUS1_BRAKE) || (outpc.rpm < 500)) { // arbitrary low rpm level
		    // cancel lockup immediately (also when braking)
		    lockupmode = 0;
		    outpc.tcc = 0;
	    } else if (itmp2 && (outpc.vss1 > pg4_ptr->tcc_vss[itmp1])
            && (outpc.rpm > pg4_ptr->tcc_rpm[itmp1]) && (lockupmode == 0)) {
       		//initiate lockup wait time
		    lockupmode = 1;
		    lockupcount = pg4_ptr->lockrace_delay;
        } else if ((lockupmode == 1) && (lockupcount == 0)) {
			lockupmode = 2;
			outpc.tcc = 255; // full on
        }
        // Lockup will end by the driver letting off the throttle or a gear shift.

/* Normal lockup mode */

	} else if ((itmp2 == 0) || (outpc.status1 & STATUS1_BRAKE) || (outpc.rpm < 500)) { // arbitrary low rpm level
		// cancel lockup immediately (also when braking)
		lockupmode = 0;
		outpc.tcc = 0;
        outpc.status1 &= ~STATUS1_RACE;
	} else if (itmp2 && (outpc.vss1 > pg4_ptr->tcc_vss[itmp1]) && (outpc.rpm > pg4_ptr->tcc_rpm[itmp1]) && (lockupmode == 0) && (outpc.tps >= pg4_ptr->lockupmintps) && (outpc.tps < pg4_ptr->lockuptps)) {
   		//initiate lockup wait time
		lockupmode = 1;
		lockupcount = pg4_ptr->lockupontime;
		// don't touch value yet. Ensure we stay above rpm long enough to avoid irritating on/off			
	} else if (itmp2 && lockupmode && (lockupmode < 4)
        && ((outpc.vss1 < (pg4_ptr->tcc_vss[itmp1] - pg4_ptr->lockuphyst))
        || (outpc.rpm < (pg4_ptr->tcc_rpm[itmp1] - RPM_HYST))
		|| (outpc.tps < pg4_ptr->lockupmintps)
        || (outpc.tps > pg4_ptr->lockuptps)) ) {
            outpc.status1 &= ~STATUS1_RACE;
			// In an allowed gear, presently in lockup, but below VSS or TPS out of range - start unlocking
			if (lockupmode > 1) {
				lockupmode = 4;
				lockupcount = pg4_ptr->lockupofftime;
			} else {
				// never got started
				lockupmode = 0; 
				outpc.tcc = 0; // should already be zero
				lockupcount = 0;
			}
	} else {
        outpc.status1 &= ~STATUS1_RACE;
    }

	// Handle lockup state machine
	if (lockupmode == 0) {
		// off
		outpc.tcc = 0;

	} else if ((lockupmode == 1) && (lockupcount == 0)) {
		// completed wait phase
		lockupmode = 2;
		outpc.tcc = pg4_ptr->lockupstart;
		lockupcount = pg4_ptr->lockupontime;

	} else if (lockupmode == 2) { /* Transition to on */
		if (lockupcount == 0) {
			lockupmode = 3; // we got there
			outpc.tcc = pg4_ptr->lockupfull; // (interp should have done this already)
		} else {
			unsigned long interp1, interp2;
			if (pg4_ptr->lockupfull < pg4_ptr->lockupstart) {
				outpc.tcc = pg4_ptr->lockupfull; // user entered silly values, turn it on now
			} else {
				// upward slope
				interp1 = (pg4_ptr->lockupontime - lockupcount) * 100;
				interp1 = interp1 / pg4_ptr->lockupontime;
				interp2 = pg4_ptr->lockupfull - pg4_ptr->lockupstart;
				interp2 = (interp2 * interp1) / 100;
				outpc.tcc = pg4_ptr->lockupstart + (unsigned char)interp2;
			}
		}

    /* lockupmode == 3 means on */

	} else if (lockupmode == 4) { /* Transition to off */
		if (lockupcount == 0) {
			lockupmode = 0; // we got there
			outpc.tcc = 0;
		} else {
			unsigned long interp1, interp2;
			if (pg4_ptr->lockupfull < pg4_ptr->lockupend) {
				outpc.tcc = pg4_ptr->lockupend; // user entered silly values, turn it off now
			} else {
				// downward slope
				interp1 = lockupcount * 100;
				interp1 = interp1 / pg4_ptr->lockupontime;
				interp2 = pg4_ptr->lockupfull - pg4_ptr->lockupend;
				interp2 = (interp2 * interp1) / 100;
				outpc.tcc = pg4_ptr->lockupend + (unsigned char)interp2;
			}
		}
	}

    if (lockupmode == 3) {
        outpc.status1 |= STATUS1_LOCKUP;
    } else {
        outpc.status1 &= ~STATUS1_LOCKUP;
    }
}

void od_button(void)
{
    unsigned char tmp;

#define DEBOUNCE 390 /* 50ms */
    if (od_state) {
        /* States
            0 : not used OD cancel at all
            1 : Enabled, but off
            10 : Enabled, active
        */
        if (od_state == 1) {
            if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]) == 0)) {
                if ((flash4.od_mode & OD_MODE_MOMENTARY) == 0) { /* latching */
                    od_state = 10; /* OD cancel */
                } else {
                    od_state = 2;
                    od_timer = (unsigned int)lmms;
                }
            }

        } else if (od_state == 2) { /* check real press */
            if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]))) {
                /* no longer grounded, bail */
                od_state = 1;
            } else if (((unsigned int)lmms - od_timer) > DEBOUNCE) {
                od_state = 9;
            }

        } else if (od_state == 9) { /* wait for release */
            if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]))) {
                /* no longer grounded, deon */
                od_state = 10;
            }

        } else if (od_state == 10) {
            if ((flash4.od_mode & OD_MODE_MOMENTARY) == 0) { /* latching */
                if (pin_gearsw[3] && (*port_gearsw[3] & pin_gearsw[3])) {
                    od_state = 1; /* OD non-cancel */
                }
            } else { /* momentary */
                if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]) == 0)) {
                    od_state = 11;
                    od_timer = (unsigned int)lmms;
                }
            }

       } else if (od_state == 11) {
            if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]))) {
                /* no longer grounded, bail */
                od_state = 10;
            } else if (((unsigned int)lmms - od_timer) > DEBOUNCE) {
                od_state = 5;
            }

        } else if (od_state == 5) { /* wait for release */
            if (pin_gearsw[3] && ((*port_gearsw[3] & pin_gearsw[3]))) {
                /* no longer grounded, done */
                od_state = 1;
            }
       }
    }

    tmp = (od_state >= 9); /* Positive means cancelled */

    if (flagbyte15 & FLAGBYTE15_OD_ENABLE) { /* Opposite meaning */
        tmp = !tmp;
    }

    if (tmp) {
        outpc.status1 |= STATUS1_ODCANCEL;
    } else {
        outpc.status1 &= ~STATUS1_ODCANCEL;
    }

    if (pin_odout) {
        tmp = (outpc.status1 & STATUS1_ODCANCEL); /* Positive if cancelled */
        
        if (flagbyte15 & FLAGBYTE15_OD_ENABLE) { /* Opposite meaning */
            tmp = !tmp;
        }

        DISABLE_INTERRUPTS;
        if (tmp) {
            *port_odout |= pin_odout;
        } else {
            *port_odout &= ~pin_odout;
        }
        ENABLE_INTERRUPTS;
    }
}

void calc_vss(void)
{
    /***************************************************************************
    **
    **  calculate vehicle and engine speed
    **
    **************************************************************************/
    if ((flash4.can_poll & 0x03) == 0) {
		if (stall_ess < stall_timeout_ess) {
			if (ess>0) {
				unsigned long save_ess;
				DISABLE_INTERRUPTS;
				save_ess = ess;
				ENABLE_INTERRUPTS;
      		    outpc.rpm = (unsigned int)(Rpm_Coeff / save_ess); // only if not using CAN
			}
		} else {
			stall_ess = stall_timeout_ess + 1; // keep it from rolling over
			outpc.rpm = 0;
			ess = 0;
		}
    } else {
// as rpm fetched via CAN, that input signal can mean input shaft rpm
		if (stall_ess < stall_timeout_ess) {
			if (ess>0) {
				unsigned long save_ess;
				DISABLE_INTERRUPTS;
				save_ess = ess;
				ENABLE_INTERRUPTS;
      		    outpc.inprpm = (unsigned int)(inpRpm_Coeff / save_ess);
			}
		} else {
			stall_ess = stall_timeout_ess + 1; // keep it from rolling over
			outpc.inprpm = 0;
			ess = 0;
		}
    }

    /* VSS and output shaft speed */
    if (vss1_stall < VSS_STALL_TIMEOUT) {
        if (flagbyte0 & FLAGBYTE0_SAMPLE_VSS1) {
            unsigned int tmp_speed;
            volatile unsigned int tmp_vss_teeth;
            volatile unsigned long tmp_vss_time_sum;
            DISABLE_INTERRUPTS;
            tmp_vss_teeth = vss1_teeth;
            tmp_vss_time_sum = vss1_time_sum;
            vss1_teeth = 0;
            vss1_time_sum = 0;
            flagbyte0 &= ~FLAGBYTE0_SAMPLE_VSS1;
            ENABLE_INTERRUPTS;

            if (tmp_vss_time_sum > 0) {
                tmp_speed = (unsigned int) ((vss1_coeff * tmp_vss_teeth) / tmp_vss_time_sum);

#define VSS1LF 30
                outpc.vss1 += (((int)tmp_speed - (int)outpc.vss1) * (long)VSS1LF) / 100;

                if ((outpc.vss1 < 20) && (tmp_speed == 0)) {
                    outpc.vss1 = 0;
                }

                /* Output shaft RPM */
                tmp_speed = (unsigned int) ((outRpm_Coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                outpc.outrpm += (((int)tmp_speed - (int)outpc.outrpm) * (long)VSS1LF) / 100;

                if ((outpc.outrpm < 100) && (tmp_speed == 0)) {
                    outpc.outrpm = 0;
                }
            }
        }
    } else {
        DISABLE_INTERRUPTS;
        vss1_stall = VSS_STALL_TIMEOUT + 1; // keep it from rolling over
        outpc.vss1 = 0;
        outpc.outrpm = 0;
        vss1_teeth = 0;
        vss1_time_sum = 0;
        ENABLE_INTERRUPTS;
    }

    /* Calculate trans+convertor slip factor */
    if (outpc.rpm && outpc.outrpm) {
        /* If we have an input shaft RPM reading, use that, else use engine speed */
        int inrpm, calcin, diff, tmp_slip, gear_rat;
        if (outpc.inprpm) {
            inrpm = outpc.inprpm; /* Slip across trans only */
        } else {
            inrpm = outpc.rpm; /* Slip across convertor and trans */
        }

        /* Now work out what we think the input speed should be */
        /* FIXME: Need to handle gear-shifts */
        if (outpc.gear >= 1) {
            gear_rat = gear_ratio[outpc.gear - 1];
        } else {
            gear_rat = gear_ratio[0]; /* use 1st gear ratio when in neutral or reverse */
        }

        calcin = (gear_rat * (unsigned long)outpc.outrpm) / 100;

        if ((trans == TRANS_4L80E) && (outpc.gear == 4)) {
            /* 4L80E has ISS downstream of OD gears, so requires adjustment when in 4th */
            /* This also means that any slip in the OD clutches will show as convertor slip */
            inrpm = ((unsigned long)inrpm * gear_rat) / 100;
        }

        /* Calculate convertor slip */
        diff = outpc.rpm - inrpm;
        if (diff < 0) { /* input shaft faster than engine */
            tmp_slip = (diff * 100L) / outpc.outrpm;
        } else { /* engine faster than input shaft */
            tmp_slip = (diff * 100L) / inrpm;
        }
        if (tmp_slip > 120) {
            tmp_slip = 120;
        } else if (tmp_slip < -120) {
            tmp_slip = -120;
        }
        outpc.slip_conv = (char)tmp_slip;

        /* Calculate trans slip */
        diff = inrpm - calcin;
        if (diff < 0) { /* output shaft faster than input */
            tmp_slip = (diff * 100L) / inrpm;
        } else { /* input faster than output shaft */
            tmp_slip = (diff * 100L) / calcin;
        }
        if (tmp_slip > 120) {
            tmp_slip = 120;
        } else if (tmp_slip < -120) {
            tmp_slip = -120;
        }
        outpc.slip_trans = (char)tmp_slip;

    } else {
        outpc.slip_conv = 0;
        outpc.slip_trans = 0;
    }

    /* Scaling units made to match MS3 
        MS3 uses 50us ticks (comments match this)
        trans uses 128us ticks
        ppm was 160934400UL
        ppk was 100000000UL
    */
    if (pin_vssout) {
        unsigned int vssout_match_tmp;
        if (outpc.vss1 == 0) {
            vssout_match_tmp = 0; // don't flip output
        } else {
            if ((pg4_ptr->vssout_opt & 0xc0) == 0x00) {
                /* time */
                vssout_match_tmp = (pg4_ptr->vssout_scale * 50UL) / (outpc.vss1 * 128UL);
            } else if ((pg4_ptr->vssout_opt & 0xc0) == 0x40) {
                /* pulses per mile
                factor is 16093.44 * 20000 / 2 
                (miles to metresX10, 20000 ticks per second, but need half a period) */
                vssout_match_tmp = (62865000UL / pg4_ptr->vssout_scale) / outpc.vss1;
            } else if ((pg4_ptr->vssout_opt & 0xc0) == 0x80) {
                /* pulses per km
                factor is 10000.00 * 20000 / 2 
                (miles to metresX10, 20000 ticks per second, but need half a period) */
                vssout_match_tmp = (39062500UL / pg4_ptr->vssout_scale) / outpc.vss1;
            } else {
                /* mimic - port clearing handled in isr_rtc.s */
                vssout_match_tmp = 0;
            }
        }
        vssout_match = vssout_match_tmp;
    } else {
        vssout_match = 0;
    }
}
