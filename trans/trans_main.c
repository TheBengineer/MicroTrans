/* $Id: trans_main.c,v 1.30 2015/05/16 21:12:18 jsm Exp $ */
/*************************************************************************
 **************************************************************************
 **   M E G A S Q U I R T  II - 2 0 0 4 - V1.000
 **
 **   (C) 2003 - B. A. Bowling And A. C. Grippo
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

/*************************************************************************
 **************************************************************************
 **   GCC Port
 **
 **   (C) 2004,2005 - Philip L Johnson
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

/*************************************************************************
 **************************************************************************
 **   MS2/Extra
 **
 **   (C) 2006 - Ken Culver, James Murray (in either order)
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

/*************************************************************************
 **************************************************************************
 **     Trans controller for MS2 or Microsquirt, derived from MS2/Extra code
 **     (c) 2007,2008,2009,2010,2011,2012,2013 James Murray
 **
 **   This header must appear on all derivatives of this code.
 **   Only for use on approved hardware.
 **
 ***************************************************************************
 **************************************************************************/

#include "trans.h"
#include "cltfactor.inc"
#include "matfactor.inc"
#include "trans_main_vectors.h"
#include "trans_main_decls.h"
#include "trans_main_defaults.h"
#include "trans_main_vars.h"

int main(void) {
    unsigned char utmp1;
    unsigned int t, t1;

    // in this application we can run tuning tables from ram
    pg4_ptr = (page4_data *)&ram_data;
    memcpy(ram_data, &flash4, 1024);
    page = 4; // confirm that page 4 is in ram

    main_init(); // set up all timers, initialise stuff

    t = TCNT;
    //  main loop
    for (;;)  {
        t1 = TCNT - t;
        t = TCNT;
//        outpc.status6 = t1;

        if (conf_err) {
            configerror();
        }

        //reset COP timer (That's Computer Operating Properly, not Coil On Plug...)
        ARMCOP = 0x55;
        ARMCOP = 0xAA;

        if ((flash4.setting1 & SETTING1_CPUMASK) == 0) {
            goto SKIP_IT_ALL;
        }

        serial();

	    if ((lmms - adc_lmms) > 78)  {          // every 10 ms (78 x .128 ms clk)
	        adc_lmms = lmms;
	        // read 10-bit ADC results, convert to engineering units and filter
	        next_adc++;
	        if (next_adc > 7) {
			    unsigned char val;
                signed char tmp_selector = 11;

           		next_adc = 0;
                val = 0;

                tmp_selector = 99; // invalid setting to start with

                if (gearsel == 0)  {
                  	// check selector switch inputs
              	    if ((*port_gearsw[0] & pin_gearsw[0]) == pin_gearsw_match[0]) { /* switch A */
               	    	val |= 0x04;
	          	    }
              	    if ((*port_gearsw[1] & pin_gearsw[1]) == pin_gearsw_match[1]) { /* switch B */
               	    	val |= 0x02;
	          	    }
              	    if ((*port_gearsw[2] & pin_gearsw[2]) == pin_gearsw_match[2]) { /* switch C */
               	    	val |= 0x01;
	          	    }
              	    if ((*port_gearsw[3] & pin_gearsw[3]) == pin_gearsw_match[3]) { /* switch P */
               	    	val |= 0x08;
	          	    }

                    outpc.gearselin = val;

                    if (trans == TRANS_4T40E) {
    /*
                      	if (val == 0) {
                    		tmp_selector = 3;
                      	} else if (val == 1) {
                    		tmp_selector = -1;
                      	} else if (val == 2) {
                    		tmp_selector = 2;
                       	} else if (val == 3) {
                    		tmp_selector = 0;
                       	} else if (val == 4) {
                    		tmp_selector = 4;
                       	} else if (val == 5) {
                    		tmp_selector = 0;
                       	} else if (val == 6) {
                    		tmp_selector = 1;
                       	// 7 invalid
                       	}
    */
                        /* Per Jeff Linfert 2014-12-11 - only B&C used. */
                        val &= 0x03;
                      	if (val == 0) {
                    		tmp_selector = 0;
                      	} else if (val == 1) {
                    		tmp_selector = -1;
                      	} else if (val == 2) {
                    		tmp_selector = 4;
                       	// others invalid
                       	}

                    } else if (trans == TRANS_5L40E) {
                      	if (val == 12) {
                    		tmp_selector = 0;
                      	} else if (val == 6) {
                    		tmp_selector = -1;
                      	} else if (val == 10) {
                    		tmp_selector = 0;
                       	} else if (val == 2) {
                    		tmp_selector = 5;
                       	} else if (val == 15) {
                    		tmp_selector = 4;
                       	} else if (val == 5) {
                    		tmp_selector = 3;
                       	} else if (val == 9) {
                    		tmp_selector = 2;
                       	}

                    } else if (trans == TRANS_E4ODS) {
                      	if (val == 8) { /* in D and OD pressed */
                    		tmp_selector = 4;
                      	} else if ((val & 7) == 7) { /* P */
                    		tmp_selector = 0;
                      	} else if ((val & 7) == 6) { /* R */
                    		tmp_selector = -1;
                       	} else if ((val & 7) == 5) { /* N */
                    		tmp_selector = 0;
                       	} else if ((val & 7) == 0) {
                    		tmp_selector = 3;
                       	} else if ((val & 7) == 2) {
                    		tmp_selector = 2;
                       	} else if ((val & 7) == 1) {
                    		tmp_selector = 1;
                       	}

                    } else { /* 4L80E, 4L60E work the same */
                      	if ((val & 7) == 0) {
                    		tmp_selector = 2;
                      	} else if ((val & 7) == 1) {
                    		tmp_selector = 3;
                      	} else if ((val & 7) == 2) {
                    		tmp_selector = 0;
                       	} else if ((val & 7) == 3) {
                    		tmp_selector = 4;
                       	} else if ((val & 7) == 4) {
                    		tmp_selector = 1;
                       	// 5 invalid
                       	} else if ((val & 7) == 6) {
                    		tmp_selector = -1;
                       	// 7 invalid
                       	}
                    }

                } else { /* analogue */
                    unsigned int gear_v, x, g;
                    // in this mode the 0-5V signal tells us the gear we are in

                    __asm__ __volatile__ (
                    "ldy #500\n"
                    "emul\n"
                    "ldx #1023\n"
                    "ediv\n"
                    : "=y" (gear_v)
                    : "d" (*port_gearsel)
                    );

                    // loop to selection value
                   
    // array is park, reverse, neutral, 1, 2, 3, 4
                    g = max_gear + 2;

                    for (x = 0; x <= g ; x++) {
                        int v_low, v_high;
                        // +/- 0.1V
                        v_low = pg4_ptr->gearv[x] - 20;
                        v_high = pg4_ptr->gearv[x] + 20;

                        if (((int)gear_v > v_low) && ((int)gear_v < v_high)) {
                            if (x == 0) {
                                tmp_selector = 0;
                            } else if (x == 1) {
                                tmp_selector = -1;
                            } else if (x == 2) {
                                tmp_selector = 0;
                            } else {
                                tmp_selector = x - 2;
                            }
                            break;
                        }
                    } // end for
                }

                if ((tmp_selector == outpc.selector) && (tmp_selector == old_selector)) { /* no true change */
                    gearsel_count = 0;
			    } else if (tmp_selector == 99) {
                   gearsel_count++;
                   if (gearsel_count > 4) {    // if count>4 then something is wrong with inputs
                        outpc.selector = -5; // stupid value to flag something wrong
			       } /* else ignore the reading on this pass */

                } else if ( (tmp_selector != outpc.selector) || (tmp_selector != old_selector) ) {
                   gearsel_count++;
                   if (gearsel_count > 4) { /* if count>4 then accept it */
                        old_selector = tmp_selector;
                        outpc.selector = tmp_selector;
			       } /* else ignore the reading on this pass */
			    }

                if ((*port_brake & pin_brake) == pin_brake_match) { // brake switch input active
                    outpc.status1 |= STATUS1_BRAKE;
                } else {
                    outpc.status1 &= ~STATUS1_BRAKE;
                }
     		}
		    get_adc(next_adc,next_adc);    // get one channel on each pass

            /* Check for OD enabling modes */
            if (flagbyte15 & FLAGBYTE15_OD_ENABLE) {
                /* In penultimate gear and OD is un-cancelled */
                if ((outpc.selector == (max_gear - 1)) && (!(outpc.status1 & STATUS1_ODCANCEL))) {
                    outpc.selector = max_gear;
                }
            }
        
	    }
		if (resetholdoff) {
			goto SKIP_IT_ALL;  // don't change gear or show new speeds etc based on data just after a burn
		}

        /***************************************************************************
        **
        **  Test mode part 1
        **
        ** check that the lock code is in RAM but not in flash. If it somehow gets
        ** burned to flash then don't run the test mode. Also need the enable
        ** setting.
        **************************************************************************/
        if ((flagbyte1 & flagbyte1_tstmode) && (datax1.testmodelock == 12345)) {
            if (testmode_glob & 1) {
                outpc.gear = testmode_gear;
            }
            if (testmode_glob & 2) {
                outpc.epc = pg4_ptr->tst_epc;
            }
            if (testmode_glob & 4) {
                outpc.tcc = pg4_ptr->tst_tcc;
            }
            goto SKIP_GEAR;
        }

        /***************************************************************************
        **
        **  Main piece of code - gear selection
        **
        **************************************************************************/

        if ((shift_phase != 0) || shift_delay) {
            // don't change gear while already changing gear
            goto SKIP_GEAR;
        }

        last_gear = outpc.gear; // old gear

        if (pin_paddle_en && ((*port_paddle_en & pin_paddle_en) == pin_paddle_en_match)) {
            unsigned char pad_mode = 0;
            int v_low, v_high, paddle_v;

            __asm__ __volatile__ (
            "ldy #500\n"
            "emul\n"
            "ldx #1023\n"
            "ediv\n"
            : "=y" (paddle_v)
            : "d" (*port_paddle_in)
            );

            // +/- 0.1V
            v_low = pg4_ptr->paddle_upv - 20;
            v_high = pg4_ptr->paddle_upv + 20;

            if ((paddle_v > v_low) && (paddle_v < v_high)) {
                pad_mode = 1; 
            }

            // +/- 0.1V
            v_low = pg4_ptr->paddle_downv - 20;
            v_high = pg4_ptr->paddle_downv + 20;

            if ((paddle_v > v_low) && (paddle_v < v_high)) {
                pad_mode = 2; 
            }

            // +/- 0.1V
            v_low = pg4_ptr->paddle_outv - 20;
            v_high = pg4_ptr->paddle_outv + 20;

            if ((paddle_v > v_low) && (paddle_v < v_high)) {
                pad_mode = 3;
            }

            if (pin_paddle_out) {
                if (pad_mode == 3) {
                    *port_paddle_out |= pin_paddle_out;
                } else {
                    *port_paddle_out &= ~pin_paddle_out;
                }
            }

            if (outpc.selector > 0) {
                // in forward gear and paddle mode enabled - ignore selector and just use paddles.
                if (outpc.gear == 0) {
                    outpc.gear = 1; // N->1
                    paddle_phase = 0;
                    paddle_timer = 0;
                } else {
                    // in a gear, so allow paddles
                    if (paddle_phase == 0) {

                        if ((pad_mode == 1) && (outpc.gear < max_gear)) {
                            outpc.gear++;
                            paddle_timer = 3906; // 500ms timeout/re-shift
                            paddle_phase = 1;
                        }

                        if ((pad_mode == 2) && (outpc.gear > 1)) {
                            outpc.gear--;
                            paddle_timer = 3906;
                            paddle_phase = 2;
                        }
                    } else if ((paddle_phase == 1) || (paddle_phase == 2)) {
                        if ((pad_mode != 1) && (pad_mode != 2)) { // was pressed, now released
                            paddle_timer = 78; // 10ms debounce
                            paddle_phase += 2;
                        }
                        paddle_phase = 0;
                    } else {
                        if (paddle_timer) {
                            if ((pad_mode == 1) || (pad_mode == 2)) {
                                // button re-pressed, hold-off again
                                paddle_timer = 3906;
                                paddle_phase -= 2;
                            }
                        } else {
                            paddle_phase = 0; // all done, user can press button again
                        }
                    }
                }
            }

        } else if ( ((flash4.manual & MANUAL_MASK) == MANUAL_MAN) /* full manual mode */
                || ( ((flash4.manual & MANUAL_MASK) == MANUAL_MANSW)
                    && (pin_manual && ((*port_manual & pin_manual) == 0)) ) /* switched manual mode and on */
                ) {
            outpc.gear = outpc.selector;

        } else {
            /* Regular mode. (not manual or paddle) */
            unsigned char tmp_maxgear;
            if ((outpc.status1 & STATUS1_ODCANCEL) && (!(flagbyte15 & FLAGBYTE15_OD_ENABLE))) {
            /* Using a cancel button and OD is cancelled */
                tmp_maxgear = max_gear - 1;
            } else {
                tmp_maxgear = max_gear;
            }

            if (outpc.selector == -1) { // R
    // doesn't seem safe to select 1st gear with selector in neutral
    // What happens if neutral selected at high speed and then selector moved to OD?
                if (outputmode == OUTPUTMODE_41TE) {
                    if (outpc.gear > 0) {
                        outpc.gear--;
                    } else {
                        outpc.gear = -1;
                    }
                } else {
			        if (outpc.vss1 < pg4_ptr->shiftvss[0][0][5]) {   // ignore the request if above 1st gear top speed
                		outpc.gear = 1;
			        }
                }
		    } else if (outpc.selector == 0) { // N
                if (outputmode == OUTPUTMODE_41TE) {
                    if (outpc.gear > 0) {
                        outpc.gear--;
                    } else {
                        outpc.gear = 0;
                    }
                } else {
			        if (outpc.vss1 < pg4_ptr->shiftvss[0][0][5]) {   // ignore the request if above 1st gear top speed
                		outpc.gear = 1;
			        }
                }
		    } else if (outpc.selector == -5) { // error
			    ;
		    } else { // 1,2,3,4...
                if (outpc.gear < 0) {
                    outpc.gear = 0;
                } else {
                    unsigned int rpmlim_curgear, rpmlim_gearbelow;

                    rpmlim_curgear = flash4.maxrpm[outpc.gear - 1];

			        // upshifting checks
                    if ((outpc.gear < tmp_maxgear) && (outpc.selector > outpc.gear)) { /* shift possible */
                        if ((flash4.setting1 & SETTING1_RPM_SHIFT) && (outpc.tps > flash4.rpm_shift_tps)) { /* RPM-based up-shifting */
                            if (outpc.rpm >= rpmlim_curgear) { /* only check max RPM for this gear */
                                outpc.gear++;
                            }

			            } else { /* regular upshift check */
				            if ( (outpc.vss1 > (unsigned int)shift_lookup(outpc.gear-1, 0)) /* exceeded upshift VSS */
                                || (outpc.rpm >= rpmlim_curgear) /*  or max RPM for this gear */
                                ) {
					            outpc.gear++;
				            }
                        }
			        }
#define DOWNSHIFT_RPM_TOL 200 /* No point in kicking down if need to immediately upshift */
			        // downshifting
			        if (outpc.gear > 1) {
				        int mph_diff;
                        unsigned int calcrpm;
                        rpmlim_gearbelow = flash4.maxrpm[outpc.gear - 2];

                        calcrpm = ((unsigned long)outpc.rpm * gear_ratio[outpc.gear - 2]) / gear_ratio[outpc.gear - 1];

			            // manual ranges
			            // force a downshift if requested by selector or OD cancel and safe
			            if ( ((outpc.gear > outpc.selector) || (outpc.gear > tmp_maxgear))
                            && (calcrpm < rpmlim_gearbelow) ) {
                   			outpc.gear--;
			            } else {

				            mph_diff = shift_lookup(outpc.gear-2, 1);
				            if (mph_diff < 0) {
					            mph_diff = 0;
				            }
				            outpc.status2 = (unsigned char)mph_diff;
                            /* Shift curve says to downshift, but check for over-rev in lower gear */
				            if ((outpc.vss1 < (unsigned int)mph_diff) 
                                && (calcrpm < (rpmlim_gearbelow - DOWNSHIFT_RPM_TOL)) ) {
					            outpc.gear--;
				            }
			            }
                    }
                }
		    }
        }

        if (outpc.gear != last_gear) {
            shift_delay = flash4.shift_pause;

            shiftret_timer = flash4.shiftret_time;
            flagbyte0 |= FLAGBYTE0_SHIFTRET | FLAGBYTE0_SHIFTRETTX;
            outpc.shift_retard =  intrp_1ditable(outpc.load, NUM_SRETS, (int *)pg4_ptr->sret_load, 1,
                 (int *)pg4_ptr->sret_retard);

            if (!( (outpc.status1 & STATUS1_RACE) && (pg4_ptr->lockrace & 0x08))) {
                // have changed gear and not in race mode with locked shifts
                lockupmode = 0; // return to lockupmode zero
                outpc.tcc = 0; // turn off lockup immediately
            }
        }

SKIP_GEAR:;
        if ((shiftret_timer == 0) && (flagbyte0 & FLAGBYTE0_SHIFTRET)) {
            flagbyte0 &= ~FLAGBYTE0_SHIFTRET;
            flagbyte0 |= FLAGBYTE0_SHIFTRETTX;
            outpc.shift_retard = 0;
        }

        /***************************************************************************
        **
        **  3-2 solenoid (4L60E only)
        **
        **************************************************************************/
		if (outputmode == OUTPUTMODE_4L60E) { // 4L60E
	        if ((outpc.gear == -1) /*|| (outpc.gear == 0)*/ || (outpc.gear == 1)) {
				outpc.sol32 = 0;
			} else if (outpc.gear == 2) {
				outpc.sol32 = 230; // 90%
			} else if (outpc.gear == 3) {
				outpc.sol32 = 230; // 90%
			} else {
				outpc.sol32 = 230; // 90%
			}
        }

        /***************************************************************************
        **
        **  3-2 testing override
        **
        **************************************************************************/
        if ((flagbyte1 & flagbyte1_tstmode) && (datax1.testmodelock == 12345)) {
            if (testmode_glob & 8) {
                outpc.sol32 = pg4_ptr->tst_sol32;
            }
        }

        /***************************************************************************
        **
        **  Send gear signal, EPC, TCC to trans
        **
        **************************************************************************/
// Presently!!
// 4L80E, 4L60E, A340 only support uS, MS2 platform
// 41TE only supports GPIO

// For now use 4th gear in P/N ranges.
		if (outputmode == OUTPUTMODE_4L80E) { // 4L80E
	        if (outpc.gear == 1) {
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0]; // sol A
				*port_sol[1] &= ~pin_sol[1];  // sol B
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 2) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 3) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
      		*pwm_sol[2] = ~outpc.epc;  // GM trans work that 0 duty - full pressure, 40% = min pressure.
            ENABLE_INTERRUPTS;

		} else if (outputmode == OUTPUTMODE_4L60E) { // 4L60E
	        if ((outpc.gear == -1) /*|| (outpc.gear == 0)*/ || (outpc.gear == 1)) {
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 2) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 3) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
      		*pwm_sol[2] = ~outpc.epc;  // GM trans work that 0 duty - full pressure, 40% = min pressure.
            *pwm_sol[4] = outpc.sol32;
            ENABLE_INTERRUPTS;

            if (flash4.setting3 & SETTING3_LUF) { /* Handle lockup apply solenoid */
                DISABLE_INTERRUPTS;
                if (outpc.tcc) {
                    *port_sol[5] |= pin_sol[5];
                } else {
                    *port_sol[5] &= ~pin_sol[5];
                }
                ENABLE_INTERRUPTS;
            }

		} else if (outputmode == OUTPUTMODE_A340E) { // A340E
	        if (outpc.gear == 1) {
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0]; // sol A
				*port_sol[1] &= ~pin_sol[1];  // sol B
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 2) {
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 3) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
       		*pwm_sol[2] = ~outpc.epc;
            *pwm_sol[4] = outpc.sol32;
            ENABLE_INTERRUPTS;

		} else if (outputmode == OUTPUTMODE_41TE) { // 41TE

            if (shift_phase == 0) {
                if (outpc.gear != last_gear) { // just shifted
                    shift_timer = 0;
                }
                // decide which shift state machine to use
                if ((last_gear == 1) && (outpc.gear == 2)) {
                    shift_phase = 10;
                } else if ((last_gear == 2) && (outpc.gear == 3)) {
                    shift_phase = 20;
                } else if ((last_gear == 3) && (outpc.gear == 4)) {
                    shift_phase = 30;
                } else if ((last_gear == 4) && (outpc.gear == 3)) {
                    shift_phase = 40;
                } else if ((last_gear == 3) && (outpc.gear == 2)) {
                    shift_phase = 50;
                } else if ((last_gear == 2) && (outpc.gear == 1)) {
                    shift_phase = 60;
                } else if ((last_gear == 1) && (outpc.gear == 0)) {
                    shift_phase = 70;
                } else if ((last_gear == 0) && (outpc.gear == -1)) {
                    shift_phase = 80;
                } else {
                    // none of the above shift patterns, so hard shift
                    if (outpc.gear == 1) {
                        gear_41te = 0x03;
                    } else if (outpc.gear == 2) {
                        gear_41te = 0x00;
                    } else if (outpc.gear == 3) {
                        gear_41te = 0x0a;
                    } else if (outpc.gear == 4) {
                        gear_41te = 0x0c;
                    } else if (outpc.gear == -1) { // reverse
                        gear_41te = 0x00;
                    } else { // park/ neutral
                        gear_41te = 0x03;
                    }
                }

            } else {
                if (shift_phase == 10) {
                    // 1st to 2nd
                    //               L/R 2/4  UD  OD
                    // 1st pattern is 1   1   0   0
                    // 2nd pattern is 0   0   0   0
                    if (shift_timer < shift_delay_calc(outpc.tps, 0, 0)) {
                        gear_41te |= 0x01;
                    } else {
                        gear_41te &= ~0x01;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps, 0, 1)) {
                        gear_41te |= 0x02;
                    } else {
                        gear_41te &= ~0x02;
                    }
                    gear_41te &= ~0x0c; // always off
                    if ((shift_timer > shift_delay_calc(outpc.tps, 0, 0)) && (shift_timer > shift_delay_calc(outpc.tps, 0, 1))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 20) {
                    // 2nd to 3rd
                    //               L/R 2/4  UD  OD
                    // 2nd pattern is 0   0   0   0
                    // 3rd pattern is 0   1   0   1
                    if (shift_timer < shift_delay_calc(outpc.tps, 1, 1)) {
                        gear_41te &= ~0x02;
                    } else {
                        gear_41te |= 0x02;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps, 1, 3)) {
                        gear_41te &= 0x08;
                    } else {
                        gear_41te |= 0x08;
                    }
                    gear_41te &= ~0x05; // always off

                    if ((shift_timer > shift_delay_calc(outpc.tps, 1, 1)) && (shift_timer > shift_delay_calc(outpc.tps, 1, 3))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 30) {
                    // 3rd to 4th
                    //               L/R 2/4  UD  OD
                    // 3rd pattern is 0   1   0   1
                    // 4th pattern is 0   0   1   1
                    if (shift_timer < shift_delay_calc(outpc.tps, 2, 1)) {
                        gear_41te |= 0x02;
                    } else {
                        gear_41te &= ~0x02;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps, 2, 2)) {
                        gear_41te &= 0x04;
                    } else {
                        gear_41te |= 0x04;
                    }
                    gear_41te &= ~0x01; // always off
                    if ((shift_timer > shift_delay_calc(outpc.tps, 2, 1)) && (shift_timer > shift_delay_calc(outpc.tps, 2, 2))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 40) {
                    // 4th to 3rd
                    //               L/R 2/4  UD  OD
                    // 4th pattern is 0   0   1   1
                    // 3rd pattern is 0   1   0   1
                    if (shift_timer < shift_delay_calc(outpc.tps,3, 1)) {
                        gear_41te &= ~0x02;
                    } else {
                        gear_41te |= 0x02;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps,3, 2)) {
                        gear_41te |= 0x04;
                    } else {
                        gear_41te &= 0x04;
                    }
                    gear_41te &= ~0x01; // always off
                    if ((shift_timer > shift_delay_calc(outpc.tps,3, 1)) && (shift_timer > shift_delay_calc(outpc.tps, 3, 2))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 50) {
                    // 3rd to 2nd
                    //               L/R 2/4  UD  OD
                    // 3rd pattern is 0   1   0   1
                    // 2nd pattern is 0   0   0   0
                    if (shift_timer < shift_delay_calc(outpc.tps, 4, 1)) {
                        gear_41te |= 0x02;
                    } else {
                        gear_41te &= ~0x02;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps, 4, 3)) {
                        gear_41te |= 0x08;
                    } else {
                        gear_41te &= 0x08;
                    }
                    gear_41te &= ~0x05; // always off

                    if ((shift_timer > shift_delay_calc(outpc.tps, 4, 1)) && (shift_timer > shift_delay_calc(outpc.tps, 4, 3))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 60) {
                    // 2nd to 1st
                    //               L/R 2/4  UD  OD
                    // 2nd pattern is 0   0   0   0
                    // intermediate - 0   0   1   0
                    // 1st pattern is 1   1   0   0
                    if (shift_timer < shift_delay_calc(outpc.tps,5, 0)) {
                        gear_41te &= ~0x01;
                    } else {
                        gear_41te |= 0x01;
                    }
                    if (shift_timer < shift_delay_calc(outpc.tps,5, 1)) {
                        gear_41te &= ~0x02;
                    } else {
                        gear_41te |= 0x02;
                    }

                    if (shift_timer >= shift_delay_calc(outpc.tps,5, 2)) {
                        if (shift_timer < shift_delay_calc(outpc.tps,5, 3)) {
                            gear_41te |= 0x04;
                        } else {
                            gear_41te &= 0x04;
                        }
                    }

                    gear_41te &= ~0x08; // always off
                    if ((shift_timer > shift_delay_calc(outpc.tps, 5, 0)) && (shift_timer > shift_delay_calc(outpc.tps, 5, 1))
                        && (shift_timer > shift_delay_calc(outpc.tps, 5, 2)) && (shift_timer > shift_delay_calc(outpc.tps, 5, 3))) {
                        shift_phase = 0;
                    }

                } else if (shift_phase == 70) {
                    // 1st to neutral
                    //               L/R 2/4  UD  OD
                    // 1st pattern is 1   1   0   0
                    // N < 8mph    is 1   1   0   0
                    // N > 8mph    is 0   1   0   0

                    // monitor this when in neutral

                    // change immediately
                    if (outpc.vss1 > 8) {
                        gear_41te &= ~0x01;
                    } else {
                        gear_41te |= 0x01;
                    }
                    gear_41te |= 0x02;
                    gear_41te &= ~0x0c; // always off
                    shift_phase = 0;

                } else if (shift_phase == 80) {
                    // neutral to reverse
                    //               L/R 2/4  UD  OD
                    // N < 8mph    is 1   1   0   0
                    // N > 8mph    is 0   1   0   0
                    // R   pattern is 0   0   0   0
                    // R   block   is 0   1   0   0

                    // change immediately
                    if (outpc.vss1 > 8) {
                        gear_41te |= 0x02;
                    } else {
                        gear_41te &= ~0x02; //block
                    }
                    gear_41te &= ~0x0d; // always off
                    shift_phase = 0;

                } else {
                    //shouldn't happen
                    shift_phase = 0;
                }
            }

            // after state machine, implement pulsed solenoids etc.

            if (outpc.gear == 0) { // neutral
                if (outpc.vss1 > 8) {
                    gear_41te &= ~0x01;
                } else {
                    gear_41te |= 0x01;
                }
                gear_41te |= 0x02;
                gear_41te &= ~0x0c; // always off
            }

            // L/R needs to consider convertor lockup too
            if ((gear_41te & 0x01) || ((outpc.gear > 1) && (outpc.tcc > 50)))  {
                if (timer_41te < pg4_ptr->peak_time) {
                    *pwm_sol[0] = 255;
                } else {
                    *pwm_sol[0] = pg4_ptr->hold_duty;
                }
            } else {
                *pwm_sol[0] = 0;
            }

            outpc.solstat = (gear_41te & 0xfe) | (*pwm_sol[0] != 0); // 41TE only

            if (gear_41te & 0x02) {
                if (timer_41te < pg4_ptr->peak_time) {
                    *pwm_sol[1] = 255;
                } else {
                    *pwm_sol[1] = pg4_ptr->hold_duty;
                }
            } else {
                *pwm_sol[1] = 0;
            }

            if (gear_41te & 0x04) {
                if (timer_41te < pg4_ptr->peak_time) {
                    *pwm_sol[2] = 255;
                } else {
                    *pwm_sol[2] = pg4_ptr->hold_duty;
                }
            } else {
                *pwm_sol[2] = 0;
            }

            if (gear_41te & 0x08) {
                if (timer_41te < pg4_ptr->peak_time) {
                    *pwm_sol[3] = 255;
                } else {
                    *pwm_sol[3] = pg4_ptr->hold_duty;
                }
            } else {
                *pwm_sol[3] = 0;
            }
            
            if (timer_41te > pg4_ptr->refresh_period) {
                timer_41te = 0;
            }

        } else if (outputmode == OUTPUTMODE_5L40E) {
	        if (outpc.gear == 1) {
                if (outpc.selector < 5) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] &= ~pin_sol[0]; // sol A
				    *port_sol[1] |= pin_sol[1];  // sol B
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] &= ~pin_sol[0]; // sol A
				    *port_sol[1] |= pin_sol[1];  // sol B
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else if (outpc.gear == 2) {
                if (outpc.selector < 5) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else if (outpc.gear == 3) {
                if (outpc.selector < 5) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] &= ~pin_sol[1];
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] &= ~pin_sol[1];
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else if (outpc.gear == 5) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                *pwm_sol[4] = 0;
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == -1) { /* reverse */
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
/* no data on other two solenoids */
//				*port_sol[1] &= ~pin_sol[1];
//                *pwm_sol[4] = 0;
                ENABLE_INTERRUPTS;
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                *pwm_sol[4] = 255;
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
      		*pwm_sol[2] = ~outpc.epc;  // assume the same as GM?
            ENABLE_INTERRUPTS;

        } else if (outputmode == OUTPUTMODE_E4OD) {
	        if (outpc.gear < 1) {
                DISABLE_INTERRUPTS;
			    *port_sol[0] |= pin_sol[0]; // sol A
			    *port_sol[1] &= ~pin_sol[1];  // sol B
                *pwm_sol[4] = 0;
                ENABLE_INTERRUPTS;
	        } else if (outpc.gear == 1) {
                if (outpc.selector < 4) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0]; // sol A
				    *port_sol[1] &= ~pin_sol[1];  // sol B
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0]; // sol A
				    *port_sol[1] &= ~pin_sol[1];  // sol B
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else if (outpc.gear == 2) {
                if (outpc.selector < 4) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] |= pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else if (outpc.gear == 3) {
                if (outpc.selector < 4) { /* use engine-braking */
                    DISABLE_INTERRUPTS;
				    *port_sol[0] &= ~pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 255;
                    ENABLE_INTERRUPTS;
                } else {
                    DISABLE_INTERRUPTS;
				    *port_sol[0] &= ~pin_sol[0];
				    *port_sol[1] |= pin_sol[1];
                    *pwm_sol[4] = 0;
                    ENABLE_INTERRUPTS;
                }
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                *pwm_sol[4] = 0;
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
      		*pwm_sol[2] = ~outpc.epc;  // assume the same as GM?
            ENABLE_INTERRUPTS;

        } else if (outputmode == OUTPUTMODE_W4A33) {
	        if (outpc.gear == 1) {
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0]; // sol A
				*port_sol[1] |= pin_sol[1];  // sol B
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 2) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] |= pin_sol[1];
                ENABLE_INTERRUPTS;
			} else if (outpc.gear == 3) {
                DISABLE_INTERRUPTS;
				*port_sol[0] &= ~pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			} else {   // failsafe of 4th - might make driving a problem but won't over-rev
                DISABLE_INTERRUPTS;
				*port_sol[0] |= pin_sol[0];
				*port_sol[1] &= ~pin_sol[1];
                ENABLE_INTERRUPTS;
			}

            DISABLE_INTERRUPTS;
      		*pwm_sol[3] = outpc.tcc;
      		*pwm_sol[2] = ~outpc.epc; // works like GM
            ENABLE_INTERRUPTS;

        } /* end of output modes */

        if (outputmode != OUTPUTMODE_41TE) { /* all other trans */
            unsigned char solstat_tmp;
            solstat_tmp = 0;
            if (*port_sol[0] & pin_sol[0]) {
                solstat_tmp |= 1;
            }
            if (*port_sol[1] & pin_sol[1]) {
                solstat_tmp |= 2;
            }
//            if (*port_sol[2] & pin_sol[2]) {
            if (*pwm_sol[2] > 128) {
                solstat_tmp |= 4;
            }
//            if (*port_sol[3] & pin_sol[3]) {
            if (*pwm_sol[3] > 128) {
                solstat_tmp |= 8;
            }
//            if (*port_sol[4] & pin_sol[4]) {
            if (*pwm_sol[4] > 128) {
                solstat_tmp |= 0x10;
            }
            outpc.solstat = solstat_tmp;
        }

        /***************************************************************************
        **
        **  Test mode part2
        **
        **************************************************************************/
        if ((flagbyte1 & flagbyte1_tstmode) && (datax1.testmodelock == 12345) && (testmode_glob & 2)) {
            goto SKIP_EPC;
        }

        /***************************************************************************
        **
        **  Line pressure (EPC) based on 'load' and gear
        **
        **************************************************************************/
		if ((outpc.gear == -1) || ((pg4_ptr->setting1 & SETTING1_LINEP) == 0)) {
			utmp1 = 0;
		} else {
			utmp1 = outpc.gear;
		}

    	// bound input arguments
		if (outpc.rpm < 100) {
			outpc.epc = 255; // full on = rest position
    	} else {
            outpc.epc = intrp_1ditable(outpc.load, NUM_LINEP, (int *)pg4_ptr->lineload, 0,
                 (int *)pg4_ptr->linep[utmp1]);
		}
SKIP_EPC:;

        /***************************************************************************
        **
        **  Test mode part3
        **
        **************************************************************************/
        if ((flagbyte1 & flagbyte1_tstmode) && (datax1.testmodelock == 12345) && (testmode_glob & 4)) {
            goto SKIP_LU;
        }


        do_lockup();

SKIP_LU:;
        /***************************************************************************
        **
        **  Gauge calcs
        **
        **************************************************************************/
//		PWMPOL = pg4_ptr->pwmpol; // temp until correct values determined

        if (flash4.can_poll & 0x03) { // use data grabbed from CAN
            int tps_tmp;
            tps_tmp = datax1.tps_can;
            if (tps_tmp < 0) {
                tps_tmp = 0;
            }

            if (tps_tmp > 1000) {
                tps_tmp = 1000;
            }
            outpc.tps = tps_tmp;
            outpc.enginetemp = datax1.clt_can; // use CAN copy
            outpc.map = datax1.map_can;

        } else { // if not doing CAN then read TPS directly
            unsigned int et_tmp = 0;
            unsigned int raw_tps, raw_map;

            raw_tps = *port_tps;

            __asm__ __volatile__ (
                "ldd %3\n"
                "subd %2\n"
                "tfr  d,y\n"
                "ldd  %1\n"
                "subd %2\n"
                "pshd\n"
                "ldd #1000\n"
                "emuls\n"
                "pulx\n"
                "edivs\n"
                : "=y"(outpc.tps)
                : "m"(flash4.tpsmax),
                "m"(flash4.tps0),
                "m"(raw_tps)
                : "d", "x");

            /* now MAP */

            raw_map = outpc.adc[0];
            __asm__ __volatile__ (
                "ldd    %1\n"
                "subd   %2\n"
                "ldy    %3\n"
                "emul\n"
                "ldx    #1023\n"
                "ediv\n"
                "tfr    y,d\n"
                "addd   %2\n"
                : "=d"(outpc.map)
                : "m"(flash4.mapmax),
                "m"(flash4.map0),
                "m"(raw_map)
                : "y", "x");

            et_tmp = *port_enginetemp;

            // engine temp factor tables in degF
            if (pg4_ptr->can_poll & CANPOLL_DEGC) { // degC
        	    outpc.enginetemp = (cltfactor_table[et_tmp]-320)*5/9;
		    } else { // degF
        	    outpc.enginetemp = cltfactor_table[et_tmp];
		    }
        }

        unsigned int tt_tmp = 0;
        tt_tmp = *port_transtemp;
        // trans temp.  factor tables in degF
        if (pg4_ptr->can_poll & CANPOLL_DEGC) { // degC
        	outpc.transtemp = (matfactor_table[tt_tmp]-320)*5/9;
		} else { // degF
        	outpc.transtemp = matfactor_table[tt_tmp];
		}

		if (burn) {
			calc_coeff();
			burn = 0;
		}
        /**************************************************************************/

    	serial();

        calc_vss();

        od_button();

        SKIP_IT_ALL:;
        serial();
        can_poll();

        /***************************************************************************
        **
        **  Check for  CAN receiver timeout
        **
        **************************************************************************/
// This is not implemented presently.
//        DISABLE_INTERRUPTS
//        ultmp2 = ltch_CAN;
//        ENABLE_INTERRUPTS
//        if(ultmp > ultmp2)  {
//          flagbyte3 &= ~flagbyte3_getcandat;    // break out of current receive sequence
//          ltch_CAN = 0xFFFFFFFF;
//        }
        /***************************************************************************
         **
         **  Check for CAN reset
         **
         **************************************************************************/
        if (flagbyte3 & flagbyte3_can_reset)  {
            /* Re-initialize CAN comms */
            CanInit();
            flagbyte3 &= ~flagbyte3_can_reset;
        }
        generic_pwm_outs(); /* calculate software PWMs */

        /* Put any debug code in here .. */

        /* .. end debug */
    }     //  END Main while(1) Loop
}
