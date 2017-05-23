/* $Id: trans_init.c,v 1.18 2015/02/13 23:10:39 jsm Exp $ */
#include "trans.h"

void main_init(void) {
int ix;
long ltmp;

  // initalize PLL - reset default is Oscillator clock
  // 8 MHz oscillator, PLL freq = 48 MHz, 24 MHz bus,
  //  divide by 16 for timer of 2/3 usec tic
  PLLCTL &= 0xBF;     // Turn off PLL so can change freq
  SYNR = 0x02;        // set PLL/ Bus freq to 48/ 24 MHz
  REFDV = 0x00;
  PLLCTL |= 0x40;     // Turn on PLL
  // wait for PLL lock
  while (!(CRGFLG & 0x08));
  CLKSEL = 0x80;      // select PLL as clock

  // wait for clock transition to finish
  for (ix = 0; ix < 60; ix++);

  // open flash programming capability
  Flash_Init();

  if((int)RamBurnPgm & 0x0001)	{   // odd address - cpy to even one
    (void)memcpy((void *)RamBurnPgm,NoOp,1);         // cpy noop to 1st location
    (void)memcpy((void *)&RamBurnPgm[1],SpSub,32);   // cpy flashburn core pgm to ram
  } else {
    (void)memcpy((void *)RamBurnPgm,SpSub,32);       // cpy flashburn core pgm to ram
  }

  page = 0;
  pg4_ptr = (page4_data *)&flash4;

    memset((unsigned char*)&outpc, 0, sizeof(outpc)); /* set all to zero unless set specifically later */
    memset((unsigned char*)&datax1, 0, sizeof(datax1)); /* same */

  conf_err = 0; // no config errors yet
  flagbyte0 = 0; // must do these before ign_reset as it sets various bits
  flagbyte1 = 0;
  flagbyte3 = 0;
  resetholdoff = 1; // force a slight delay after start to allow data to be collected

  // set up i/o ports
  //    - port M2 is fast idle solenoid
  //    - port M3 is inj led
  //    - port M4 is accel led
  //    - port M5 is warmup led
  //    - port E0 is flex fuel sensor input
  //    - port E4 is fuel pump
  //    - port P5 is bootload pin (input)
  //    - port T6 is IAC Coil A
  //    - port T7 is IAC Coil B
  //    - port B4 is IAC Enable
  //    - port A0 is Knock Enable (if set, means retard timing)
  DDRM |= 0xFC;    // port M - all outputs, full drive by default
  DDRE = 0x10;	   // port E4 - output, E0 is input
  DDRB |= 0x10;    // port B4 - output
  IRQCR &= ~0x40;	// remove External IRQ pins from interrupt logic
  // can be input (flex fuel) or output (misc port)  // check for clashes in a minute


  // set all unused (even unbonded) ports to   with pullups
  DDRA &= 0x01; // PTA0 is input
  DDRB &= 0x10;
  PUCR |= 0x13;    // enable pullups for ports E, B and A
  DDRP = 0x00;
  PERP = 0xFF;     // enable pullup resistance for port P
  DDRJ &= 0x3F;
  PERJ |= 0xC0;    // enable pullup resistance for port J6,7
  DDRS &= 0xF3;
  PERS |= 0x0C;    // enable pullup resistance for port S2,3

  // set up CRG RTI Interrupt for .128 ms clock. CRG from 8MHz oscillator.
  mms = 0;        // .128 ms tics
  millisec = 0;   // 1.024 ms clock (8 tics) for adcs
  tenthsec = 0;
  lmms = 0;
  cansendclk = 390;
  cansendclk2 = 390;
  RTICTL = 0x10;   // load timeout register for .128 ms (smallest possible)
  CRGINT |= 0x80;  // enable interrupt
  CRGFLG = 0x80;   // clear interrupt flag (0 writes have no effect)
//  COPCTL = 0x44; // Enable long C.O.P. timeout 2^20 ~0.125s   XXXX disabled

  // Set up SCI (rs232): SCI BR reg= BusFreq(=24MHz)/16/baudrate
  SCI0BDL = (unsigned char)(1500000/115200);
  ltmp = (150000000/115200) - ((long)SCI0BDL*100);
  if(ltmp > 50) {
      SCI0BDL++;   // round up
  }
  SCI0CR1 = 0x00;
  SCI0CR2 = 0x24;   // TIE=0,RIE = 1; TE=0,RE =1
    txcnt = 0;
    rxmode = 0;
    txmode = 0;
    txgoal = 0;
    rcv_timeout = 0xFFFFFFFF;

    trans = flash4.setting2 & 0x0f; /* Global var to make things clearer */

/* Inputs and outputs

Microsquirt
===========
AN0
AN1
AN2
AN3
AN4
AN5
AN6
AN7

MS2
===
AN0 MAP
AN1 trans temp
AN2 engine CLT
AN3 TPS
AN4 batt - not used
AN5 SW A
AN6 SW B
AN7 SW C
PA0 Brake pedal switch - active low

PE4 (FP) Sol A
PM2 (Idle) Sol B
PT1, PWM2 (Inj1) EPC
PT3, PWM4 (Inj2) TCC
PM3 (LED14) 3-2 sol

GPIO
====
Not developed yet..

A340E on GPIO
============
AN0 SW A
AN1 SW B
AN2 trans temp
AN3 SW C
AN4 
AN5 Analogue Gear Selector
AN6 
AN7 Brake pedal switch - active low


PE4 Sol A
PM2 Sol B
PT2 EPC
PT3 TCC
PT1  3-2 sol
PT0 VSS

41TE on GPIO
============
PT4 is "PWM1" - L/R
PT3 is "PWM2" - 2/4
PT2 is "PWM3" - UD
PT1 is "PWM4" - OD

GPI1 is AD0 which is the shift lever sense
GPI2 is AD1 which is the Paddle lever/horn button sense
GPI4 is AD7 which is the brake sense
GPI3 is AD2 which is the trans temp sensor

VR1 is PT0 output speed sensor
VR2 is PT5 input speed sensor

*/

  // Set prescaler to 16. This divides bus clk (= PLLCLK/2 = 24 MHz)
  //   by 16. This gives 1.5 MHz timer, 1 tic = 2/3 us.
	TSCR1 = 0x00; // disable timer while we set it up
	TSCR2 = 0x84; // setup timer, enable overflow int
	TCTL1 = 0;    //
	TCTL2 = 0;    // not using timer outputs

    /* From 2014-10-08 use software PWM for TCC and 3-2 */
    /* Setup software PWM arrays with defaults */
    for (ix = 0; ix < NUM_SWPWM ; ix++) {
        port_swpwm[ix] = (volatile unsigned char *) &dummyReg;
        pin_swpwm[ix] = 0;
        gp_clk[ix] = 1;
        gp_max_on[ix] = 1;
        gp_max_off[ix] = 7812;
        gp_stat[ix] = 0;
    }

    outputmode = trans; /* normally */

    if (trans == TRANS_41TE) {
        gearsel = 1; /* analogue input */
    } else if (trans == TRANS_4R70WA) {
        gearsel = 1; /* analogue input */
        outputmode = OUTPUTMODE_4L80E; /* outputs work like 4L80E */
    } else if (trans == TRANS_4R70WS) {
        gearsel = 0; /* switch input */
        outputmode = OUTPUTMODE_4L80E; /* outputs work like 4L80E */
    } else if (trans == TRANS_4T40E) {
        gearsel = 0; /* switch input */
        outputmode = OUTPUTMODE_4L80E; /* outputs work like 4L80E */
    } else if (trans == TRANS_E4ODA) {
        gearsel = 1; /* analog input */
        outputmode = OUTPUTMODE_E4OD;
    } else if (trans == TRANS_E4ODS) {
        gearsel = 0; /* switch input */
        outputmode = OUTPUTMODE_E4OD;
    } else if (trans == TRANS_W4A33) {
        gearsel = 1; /* analog input */
        outputmode = OUTPUTMODE_W4A33; /* Looks the same as 4L60E but no 3-2 sol */
        flagbyte15 |= FLAGBYTE15_OD_ENABLE; /* Functions as enable on this trans */
    } else {
        gearsel = 0; /* switch inputs */
    }

    /* Definition to allow matching trans to be changed in one place */
    #define TRANS_SELECTOR_D (trans == TRANS_E4ODA) || (trans == TRANS_E4ODS) || (trans == TRANS_W4A33)

	if ((flash4.setting1 & SETTING1_CPUMASK) == SETTING1_MS2) {
		// MS2 CPU
   		TIOS |= 0xDE; // Timer ch 0,5 = IC, ch 1-4 = OC & PWM,
                   // ch 6,7 = I/O output
    	DDRT = 0xDE;// PTT5 and PTT0 are inputs, others are outputs
     	TCTL3 = 0x04; // TC5 captures on rising edge
     	TCTL4 = 0x01; // TC0 captures on rising edge
     	TIE = 0x21;   // enable IC interrupts
	    TSCR1 = 0x80; // turn timer on
        ATD0DIEN = 0xe0; // digi inputs on PTAD7,6,5 for SWA,B,C 
        DDRAD = 0; // all inputs
        PERAD = 0xe0; // enable pull devices
        PPSAD = 0;  // all pullup
        // Set up EPC PWM outputs - PWM2
        MODRR = 0x04;     // Make Port T pin 4 be PWM
        PWME = 0;         // disable pwms initially
        PWMPOL = 0x04;    // polarity
        PWMCLK = 0x04;    // select scaled clocks SB, SA       
        PWMPRCLK = 0x03;  // B = 24MHz  A = 24MHz/8
        if (((flash4.setting1 & SETTING1_EPCFREQ) && (trans == TRANS_4L80E))
            || (outputmode == OUTPUTMODE_E4OD)) {
            PWMSCLB = 79;   // SB = 256 * 595Hz ish
        } else {
            PWMSCLB = 159;   // SB = 256 * 292.5Hz ish
        }

        PWMCAE = 0x00;    // left align pulse
        PWMPER2 = 255;   // set PWM period EPC
        PWME = 0x04; // turn on PWM

        port_sol[0] = (unsigned char*)&PORTE;
        pin_sol[0] = 0x10;
        pwm_sol[0] = (unsigned char*)&dummyReg;

        port_sol[1] = (unsigned char*)&PTM;
        pin_sol[1] = 0x04;
        pwm_sol[1] = (unsigned char*)&dummyReg;

        port_sol[2] = (unsigned char*)&dummyReg;
        pin_sol[2] = 0x00;
        pwm_sol[2] = (unsigned char*)&PWMDTY2;

        port_sol[4] = (unsigned char*)&dummyReg;
        pin_sol[4] = 0x00;
        pwm_sol[4] = (unsigned char*)&sol32_pwm;
        port_swpwm[1] = (unsigned char *)&PTM;
        pin_swpwm[1] = 0x08;

        if ((trans == TRANS_4L60E) && (flash4.setting3 & SETTING3_LUF)) {
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTM;
            pin_swpwm[0] = 0x20;

            port_sol[5] = (unsigned char*)&PTT;
            pin_sol[5] = 0x10;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        } else { /* normal */
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTT;
            pin_swpwm[0] = 0x10;

            port_sol[5] = (unsigned char*)&PTM; /* PM5 D16 */
            pin_sol[5] = 0x20;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        }

        port_gearsel = (unsigned short*)&ATD0DR5;

        port_gearsw[0] = (unsigned char*)&PORTAD0;
        pin_gearsw[0] = 0x80;
        port_gearsw[1] = (unsigned char*)&PORTAD0;
        pin_gearsw[1] = 0x40;
        port_gearsw[2] = (unsigned char*)&PORTAD0;
        pin_gearsw[2] = 0x20;
        if (TRANS_SELECTOR_D) {
            port_gearsw[3] = (unsigned char*)&PORTE; /* PE0 */
            pin_gearsw[3] = 0x01;
        } else {
            port_gearsw[3] = (unsigned char*)&dummyReg; /* none */
            pin_gearsw[3] = 0x00;
        }
        port_enginetemp = (unsigned short*)&outpc.adc[2];
        port_tps = (unsigned short*)&outpc.adc[3];
        port_transtemp = (unsigned short*)&outpc.adc[1];

        if (flash4.brakesw & BRAKESW_ON) {
            port_brake = (unsigned char*)&PORTA;
            pin_brake = 0x01;
            if (flash4.brakesw & BRAKESW_POL) {
                pin_brake_match = pin_brake;
            } else {
                pin_brake_match = 0;
            }
        } else {
            port_brake = (unsigned char*)&dummyReg;
            pin_brake = 0x00;
            pin_brake_match = 0xff;
        }

	} else if ((flash4.setting1 & SETTING1_CPUMASK) == SETTING1_MICROSQUIRTV1) {
		// Microsquirt CPU
  		TIOS |= 0xFA; // Timer ch 0,2 = IC, ch 1-4 = OC & PWM,
                   // ch 6,7 = I/O output
     	DDRT = 0xFA;// PTT2 and PTT0 are inputs, others are outputs
     	TCTL3 = 0x00; // no captures on these timers
     	TCTL4 = 0x11; // TC0+TC2 capture on rising edge
     	TIE = 0x05;   // enable IC interrupts
    	TSCR1 = 0x80; // turn timer on
        ATD0DIEN = 0x26; // digi inputs on PTAD2,3,5 for SWA,B,C
        DDRAD = 0; // all inputs
        PERAD = 0x26; // enable pull devices
        PPSAD = 0;  // all pullup
        // Set up EPC PWM output - PWM3
        MODRR = 0x08;     // Make Port T pin 3 be PWM
        PWME = 0;         // disable pwms initially
        PWMPOL = 0x08;    // polarity
        PWMCLK = 0x1a;    // select scaled clocks SB, SA       
        PWMPRCLK = 0x03;  // B = 24MHz   A = 24MHz/8
        if (((flash4.setting1 & SETTING1_EPCFREQ) && (trans == TRANS_4L80E))
            || (outputmode == OUTPUTMODE_E4OD)) {
            PWMSCLB = 79;   // SB = 256 * 595Hz ish
        } else {
            PWMSCLB = 159;   // SB = 256 * 292.5Hz ish
        }
        PWMCAE = 0x00;    // left align pulse
        PWMPER3 = 255;   // set PWM period
        PWME = 0x08; // turn on PWM

        port_sol[0] = (unsigned char*)&PORTE;
        pin_sol[0] = 0x10;
        pwm_sol[0] = (unsigned char*)&dummyReg;

        port_sol[1] = (unsigned char*)&PTM;
        pin_sol[1] = 0x04;
        pwm_sol[1] = (unsigned char*)&dummyReg;

        port_sol[2] = (unsigned char*)&dummyReg;
        pin_sol[2] = 0x00;
        pwm_sol[2] = (unsigned char*)&PWMDTY3;

        port_sol[4] = (unsigned char*)&dummyReg;
        pin_sol[4] = 0x00;
        pwm_sol[4] = (unsigned char*)&sol32_pwm;
        port_swpwm[1] = (unsigned char *)&PTT;
        pin_swpwm[1] = 0x10;

        if ((trans == TRANS_4L60E) && (flash4.setting3 & SETTING3_LUF)) {
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTT;
            pin_swpwm[0] = 0x20;

            port_sol[5] = (unsigned char*)&PTT;
            pin_sol[5] = 0x02;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        } else { /* normal */
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTT;
            pin_swpwm[0] = 0x02;

            port_sol[5] = (unsigned char*)&PTT; /* PT5 SPKA */
            pin_sol[5] = 0x20;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        }

        port_gearsel = (unsigned short*)&ATD0DR5;

        port_gearsw[0] = (unsigned char*)&PORTAD0;
        pin_gearsw[0] = 0x02;
        port_gearsw[1] = (unsigned char*)&PORTAD0;
        pin_gearsw[1] = 0x04;
        port_gearsw[2] = (unsigned char*)&PORTAD0;
        pin_gearsw[2] = 0x20;
        if (TRANS_SELECTOR_D) {
            port_gearsw[3] = (unsigned char*)&PORTAD0; /* PTAD7 */
            pin_gearsw[3] = 0x80;
            ATD0DIEN |= 0x80;
            PERAD |= 0x80;
        } else {
            port_gearsw[3] = (unsigned char*)&dummyReg; /* none */
            pin_gearsw[3] = 0x00;
        }
        port_enginetemp = (unsigned short*)&outpc.adc[6];
        port_tps = (unsigned short*)&outpc.adc[3];
        port_transtemp = (unsigned short*)&outpc.adc[0];

        if (flash4.brakesw & BRAKESW_ON) {
            port_brake = (unsigned char*)&PORTE;
            pin_brake = 0x01;
            if (flash4.brakesw & BRAKESW_POL) {
                pin_brake_match = pin_brake;
            } else {
                pin_brake_match = 0;
            }
        } else {
            port_brake = (unsigned char*)&dummyReg;
            pin_brake = 0x00;
            pin_brake_match = 0xff;
        }

	} else if ((flash4.setting1 & SETTING1_CPUMASK) == SETTING1_GPIO) {
		// GPIO
   		TIOS |= 0xDE; // Timer ch 0,5 = IC, ch 1-4 = OC & PWM,
                   // ch 6,7 = I/O output
    	DDRT = 0xDE;// PTT5 and PTT0 are inputs, others are outputs
     	TCTL3 = 0x04; // TC5 captures on rising edge
     	TCTL4 = 0x01; // TC0 captures on rising edge
     	TIE = 0x21;   // enable IC interrupts
	    TSCR1 = 0x80; // turn timer on
        ATD0DIEN = 0x8b; // AD7 is brake digi input, AD3, AD1, AD0 are gearpos switch inputs
        DDRAD = 0; // all inputs
        if (trans == TRANS_A341E) {
            PERAD = 0x80; // enable pull devices. Just brake.
        } else {
            PERAD = 0x8b; // enable pull devices. Brake and gearsw
        }
        PPSAD = 0;  // all pullup
        PWME = 0;         // disable pwms initially
        PWMCAE = 0x00;    // left align pulse
        if (trans == TRANS_41TE) {
            MODRR = 0x1e;     // Make Port T pins 1,2,3,4 be PWM
            PWMCLK = 0x1e;    // select scaled clocks for all       
            PWMPRCLK = 0x33;  // A = 24MHz/8  B = 24MHz / 8 = 3MHz
            PWMSCLA = 3;   // SA = 3MHz / (2 * 3) = 500kHz. /256 -> ~2kHz
            PWMSCLB = 3;   // SB = 256 * 31Hz
            PWMPER1 = 255;
            PWMPER2 = 255;
            PWMPER3 = 255;
            PWMPER4 = 255;
            PWMPOL = 0x1e;    // polarity
            PWME = 0x1e; // turn on PWM
        } else { /* everything else */
            MODRR = 0x04;     // Make Port T pins 2 be PWM
            PWMCLK = 0x1a;    // select scaled clocks SB, SA       
            PWMPRCLK = 0x03;  // B = 24MHz   A = 24MHz/8
            if (((flash4.setting1 & SETTING1_EPCFREQ) && (trans == TRANS_4L80E))
                || (outputmode == OUTPUTMODE_E4OD)) {
                PWMSCLB = 79;   // SB = 256 * 595Hz ish
            } else {
                PWMSCLB = 159;   // SB = 256 * 292.5Hz ish
            }
            PWMPER2 = 255;   // EPC running at 732Hz from 'B' - over double the book spec. 
            PWMPOL = 0x04;    // polarity
			PWME = 0x04; // turn on PWM
        }

        /* I/O re-arranged 2014-09-11
            May conflict if any existing users on GPIO?? */

        if (trans == TRANS_41TE) {
            /* different pin allocation here, developed with Una */
            port_sol[0] = (unsigned char*)&dummyReg;
            pin_sol[0] = 0x00;
            pwm_sol[0] = (unsigned char*)&PWMDTY4;

            port_sol[1] = (unsigned char*)&dummyReg;
            pin_sol[1] = 0x00;
            pwm_sol[1] = (unsigned char*)&PWMDTY3;

            port_sol[2] = (unsigned char*)&dummyReg;
            pin_sol[2] = 0x00;
            pwm_sol[2] = (unsigned char*)&PWMDTY2;

            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;

            port_sol[4] = (unsigned char*)&dummyReg;
            pin_sol[4] = 0x00;
            pwm_sol[4] = (unsigned char*)&dummyReg;

            port_sol[5] = (unsigned char*)&dummyReg;
            pin_sol[5] = 0x00;
            pwm_sol[5] = (unsigned char*)&dummyReg;

            port_gearsel = (unsigned short*)&ATD0DR5;

            port_gearsw[0] = (unsigned char*)&PORTAD0;
            pin_gearsw[0] = 0x04;
            port_gearsw[1] = (unsigned char*)&PORTAD0;
            pin_gearsw[1] = 0x02;
            port_gearsw[2] = (unsigned char*)&PORTAD0;
            pin_gearsw[2] = 0x01;
            port_gearsw[3] = (unsigned char*)&dummyReg;
            pin_gearsw[3] = 0x00;
            port_enginetemp = (unsigned short*)&dummyReg;
            port_tps = (unsigned short*)&outpc.adc[3];
            port_transtemp = (unsigned short*)&outpc.adc[2];

        } else {
            /* Common pin usage for the rest */
            port_sol[0] = (unsigned char*)&PORTE;
            pin_sol[0] = 0x10;
            pwm_sol[0] = (unsigned char*)&dummyReg;

            port_sol[1] = (unsigned char*)&PTM;
            pin_sol[1] = 0x04;
            pwm_sol[1] = (unsigned char*)&dummyReg;

            port_sol[2] = (unsigned char*)&dummyReg;
            pin_sol[2] = 0x00;
            pwm_sol[2] = (unsigned char*)&PWMDTY2;

            port_sol[4] = (unsigned char*)&dummyReg;
            pin_sol[4] = 0x00;
            pwm_sol[4] = (unsigned char*)&sol32_pwm;
            port_swpwm[1] = (unsigned char *)&PTM;
            pin_swpwm[1] = 0x08;

            if ((trans == TRANS_4L60E) && (flash4.setting3 & SETTING3_LUF)) {
                port_sol[3] = (unsigned char*)&dummyReg;
                pin_sol[3] = 0x00;
                pwm_sol[3] = (unsigned char*)&tcc_pwm;
                port_swpwm[0] = (unsigned char *)&PTM;
                pin_swpwm[0] = 0x10;

                port_sol[5] = (unsigned char*)&PTT;
                pin_sol[5] = 0x08;
                pwm_sol[5] = (unsigned char*)&dummyReg;
            } else { /* normal */
                port_sol[3] = (unsigned char*)&dummyReg;
                pin_sol[3] = 0x00;
                pwm_sol[3] = (unsigned char*)&tcc_pwm;
                port_swpwm[0] = (unsigned char *)&PTT;
                pin_swpwm[0] = 0x08;

                port_sol[5] = (unsigned char*)&PTM; /* PM4 */
                pin_sol[5] = 0x10;
                pwm_sol[5] = (unsigned char*)&dummyReg;
            }

            port_gearsel = (unsigned short*)&ATD0DR5;

            port_gearsw[0] = (unsigned char*)&PORTAD0;
            pin_gearsw[0] = 0x01;
            port_gearsw[1] = (unsigned char*)&PORTAD0;
            pin_gearsw[1] = 0x02;
            port_gearsw[2] = (unsigned char*)&PORTAD0;
            pin_gearsw[2] = 0x08;
            if (TRANS_SELECTOR_D) {
                port_gearsw[3] = (unsigned char*)&PORTAD0; /* PTAD6 */
                pin_gearsw[3] = 0x40;
                ATD0DIEN |= 0x40;
                PERAD |= 0x40;
            } else {
                port_gearsw[3] = (unsigned char*)&dummyReg; /* none */
                pin_gearsw[3] = 0x00;
            }
            port_enginetemp = (unsigned short*)&dummyReg;
            port_tps = (unsigned short*)&outpc.adc[5];
            port_transtemp = (unsigned short*)&outpc.adc[2];
        }

        if (flash4.brakesw & BRAKESW_ON) {
            port_brake = (unsigned char*)&PORTAD0;
            pin_brake = 0x80;
            if (flash4.brakesw & BRAKESW_POL) {
                pin_brake_match = pin_brake;
            } else {
                pin_brake_match = 0;
            }
        } else {
            port_brake = (unsigned char*)&dummyReg;
            pin_brake = 0x00;
            pin_brake_match = 0xff;
        }

	} else if ((flash4.setting1 & SETTING1_CPUMASK) == SETTING1_MICROSQUIRTV3) {
		// Microsquirt CPU V3 wiring version - not yet updated
  		TIOS |= 0xFA; // Timer ch 0,2 = IC, ch 1-4 = OC & PWM,
                   // ch 6,7 = I/O output
     	DDRT = 0xca;// PTT5,4,2,0 are inputs, others are outputs
     	TCTL3 = 0x00; // no captures on these timers
     	TCTL4 = 0x11; // TC0+TC2 capture on rising edge
     	TIE = 0x05;   // enable IC interrupts
    	TSCR1 = 0x80; // turn timer on
        ATD0DIEN = 0x40; // digi inputs on PTAD6 for SWC
        DDRAD = 0; // all inputs
        PERAD = 0x40; // enable pull devices
        PPSAD = 0;  // all pullup
        PERT |= 0x30;
        PPST &= ~0x30;
        // Set up EPC PWM outputs - PWM3
        MODRR = 0x08;     // Make Port T pin 3 be PWM
        PWME = 0;         // disable pwms initially
        PWMPOL = 0x08;    // polarity
        PWMCLK = 0x1a;    // select scaled clocks SB, SA       
        PWMPRCLK = 0x03;  // B = 24MHz   A = 24MHz/8
        if (((flash4.setting1 & SETTING1_EPCFREQ) && (trans == TRANS_4L80E))
            || (outputmode == OUTPUTMODE_E4OD)) {
            PWMSCLB = 79;   // SB = 256 * 595Hz ish
        } else {
            PWMSCLB = 159;   // SB = 256 * 292.5Hz ish
        }
        PWMCAE = 0x00;    // left align pulse
        PWMPER1 = 255;   // set PWM period
        PWMPER3 = 255;   // set PWM period
        PWME = 0x08; // turn on PWM

        port_sol[0] = (unsigned char*)&PORTE;
        pin_sol[0] = 0x10;
        pwm_sol[0] = (unsigned char*)&dummyReg;

        port_sol[1] = (unsigned char*)&PTM;
        pin_sol[1] = 0x04;
        pwm_sol[1] = (unsigned char*)&dummyReg;

        port_sol[2] = (unsigned char*)&dummyReg;
        pin_sol[2] = 0x00;
        pwm_sol[2] = (unsigned char*)&PWMDTY3;

        port_sol[4] = (unsigned char*)&dummyReg; /* PM5 WLED */
        pin_sol[4] = 0x00;
        pwm_sol[4] = (unsigned char*)&sol32_pwm;
        port_swpwm[1] = (unsigned char *)&PTM;
        pin_swpwm[1] = 0x20;

        if ((trans == TRANS_4L60E) && (flash4.setting3 & SETTING3_LUF)) {
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTM;
            pin_swpwm[0] = 0x10;

            port_sol[5] = (unsigned char*)&PTT;
            pin_sol[5] = 0x02;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        } else { /* normal */
            port_sol[3] = (unsigned char*)&dummyReg;
            pin_sol[3] = 0x00;
            pwm_sol[3] = (unsigned char*)&tcc_pwm;
            port_swpwm[0] = (unsigned char *)&PTT;
            pin_swpwm[0] = 0x02;

            port_sol[5] = (unsigned char*)&PTM; /* PM4 ALED */
            pin_sol[5] = 0x10;
            pwm_sol[5] = (unsigned char*)&dummyReg;
        }

        if (gearsel) {
            port_gearsel = (unsigned short*)&ATD0DR6;
        } else {
            ATD0DIEN |= 0x40; // digi inputs on PTAD5 for SWP
            PERAD |= 0x40; // enable pull devices
        }

        port_gearsw[0] = (unsigned char*)&PTT;
        pin_gearsw[0] = 0x20;
        port_gearsw[1] = (unsigned char*)&PTT;
        pin_gearsw[1] = 0x10;
        port_gearsw[2] = (unsigned char*)&PORTAD0;
        pin_gearsw[2] = 0x40;
        if (TRANS_SELECTOR_D) {
            port_gearsw[3] = (unsigned char*)&PORTAD0; /* PTAD5 */
            pin_gearsw[3] = 0x20;
            ATD0DIEN |= 0x20;
            PERAD |= 0x20;
        } else {
            port_gearsw[3] = (unsigned char*)&dummyReg; /* none */
            pin_gearsw[3] = 0x00;
        }
        port_enginetemp = (unsigned short*)&dummyReg;
        port_tps = (unsigned short*)&outpc.adc[3];
        port_transtemp = (unsigned short*)&outpc.adc[1];

        if (flash4.brakesw & BRAKESW_ON) {
            port_brake = (unsigned char*)&PORTE;
            pin_brake = 0x01;
            if (flash4.brakesw & BRAKESW_POL) {
                pin_brake_match = pin_brake;
            } else {
                pin_brake_match = 0;
            }
        } else {
            port_brake = (unsigned char*)&dummyReg;
            pin_brake = 0x00;
            pin_brake_match = 0xff;
        }

	} else {
		// undefined CPU
     	DDRT = 0xD5;// PTT5, PTT2 and PTT0 are inputs, others are outputs
     	TCTL3 = 0x00; // no captures on these timers
     	TCTL4 = 0x00; // or these
     	TIE = 0x00;   // disable IC interrupts

        port_sol[0] = (unsigned char*)&dummyReg;
        pin_sol[0] = 0x00;
        pwm_sol[0] = (unsigned char*)&dummyReg;

        port_sol[1] = (unsigned char*)&dummyReg;
        pin_sol[1] = 0x00;
        pwm_sol[1] = (unsigned char*)&dummyReg;

        port_sol[2] = (unsigned char*)&dummyReg;
        pin_sol[2] = 0x00;
        pwm_sol[2] = (unsigned char*)&dummyReg;

        port_sol[3] = (unsigned char*)&dummyReg;
        pin_sol[3] = 0x00;
        pwm_sol[3] = (unsigned char*)&dummyReg;

        port_sol[4] = (unsigned char*)&dummyReg;
        pin_sol[4] = 0x00;
        pwm_sol[4] = (unsigned char*)&dummyReg;

        port_sol[5] = (unsigned char*)&dummyReg;
        pin_sol[5] = 0x00;
        pwm_sol[5] = (unsigned char*)&dummyReg;

        port_gearsel = (unsigned short*)&dummyReg;
        port_brake = (unsigned char*)&dummyReg;
        pin_brake = 0;
        pin_brake_match = 0xff;
        port_gearsw[0] = (unsigned char*)&dummyReg;
        pin_gearsw[0] = 0x00;
        port_gearsw[1] = (unsigned char*)&dummyReg;
        pin_gearsw[1] = 0x00;
        port_gearsw[2] = (unsigned char*)&dummyReg;
        pin_gearsw[2] = 0x00;
        port_gearsw[3] = (unsigned char*)&dummyReg;
        pin_gearsw[3] = 0x00;
        port_enginetemp = (unsigned short*)&dummyReg;
        port_tps = (unsigned short*)&outpc.adc[3];
        port_transtemp = (unsigned short*)&dummyReg;
	    TSCR1 = 0x80; // turn timer on for debug use
	}
    
    /* TCC software PWM */
    sw_pwm_duty[0] = (unsigned char*)&tcc_pwm;
    sw_pwm_freq[0] = 32; /* fixed frequency */
    gp_stat[0] = 0x80; /* use 0-255 scale, frequency, positive */
    /* 3-2 sol software PWM */
    sw_pwm_duty[1] = (unsigned char*)&sol32_pwm;
    sw_pwm_freq[1] = 50; /* fixed frequency */
    gp_stat[1] = 0x80; /* use 0-255 scale, frequency, positive */
    sw_pwm_num = 2;

    if (trans == TRANS_A341E) {
        /* Active high */
        pin_gearsw_match[0] = pin_gearsw[0];
        pin_gearsw_match[1] = pin_gearsw[1];
        pin_gearsw_match[2] = pin_gearsw[2];
        pin_gearsw_match[3] = pin_gearsw[3];
    } else {
        /* Normally ground switches */
        pin_gearsw_match[0] = 0;
        pin_gearsw_match[1] = 0;
        pin_gearsw_match[2] = 0;
        pin_gearsw_match[3] = 0;
    }

/* paddle shifting */
    if (flash4.paddle_en & 0x0f) {
        unsigned char tmp_opt, mask;
        tmp_opt = flash4.paddle_en & 0x0f;
        if (flash4.paddle_en & 0x80) {
            mask = 0xff;
        } else {
            mask = 0;
        }
//    paddle_en       = bits,    U08,  312, [0:3], "Off", "PE0", "PE1", "PA0", "INVALID", "INVALID", "INVALID", "INVALID", "ADC0", "ADC1", "ADC2", "ADC3", "ADC4", "ADC5", "ADC6", "ADC7"

        if (tmp_opt == 1) {
            port_paddle_en = (unsigned char*)&PORTE;
            pin_paddle_en = 1;
            pin_paddle_en_match = 1 & mask;
        } else if (tmp_opt == 2) {
            port_paddle_en = (unsigned char*)&PORTE;
            pin_paddle_en = 2;
            pin_paddle_en_match = 2 & mask;
        } else if (tmp_opt == 3) {
            port_paddle_en = (unsigned char*)&PORTA;
            pin_paddle_en = 1;
            pin_paddle_en_match = 1 & mask;
        } else if (tmp_opt == 8) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 1;
            pin_paddle_en_match = 1 & mask;
            ATD0DIEN |= 0x01; // digi inputs 
        } else if (tmp_opt == 9) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 2;
            pin_paddle_en_match = 2 & mask;
            ATD0DIEN |= 0x02; // digi inputs 
        } else if (tmp_opt == 10) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 4;
            pin_paddle_en_match = 4 & mask;
            ATD0DIEN |= 0x04; // digi inputs 
        } else if (tmp_opt == 11) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 8;
            pin_paddle_en_match = 8 & mask;
            ATD0DIEN |= 0x01; // digi inputs 
        } else if (tmp_opt == 12) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 0x10;
            pin_paddle_en_match = 0x10 & mask;
            ATD0DIEN |= 0x10; // digi inputs 
        } else if (tmp_opt == 13) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 0x20;
            pin_paddle_en_match = 0x20 & mask;
            ATD0DIEN |= 0x20; // digi inputs 
        } else if (tmp_opt == 14) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 0x40;
            pin_paddle_en_match = 0x40 & mask;
            ATD0DIEN |= 0x40; // digi inputs 
        } else if (tmp_opt == 15) {
            port_paddle_en = (unsigned char*)&PORTAD0;
            pin_paddle_en = 0x80;
            pin_paddle_en_match = 0x80 & mask;
            ATD0DIEN |= 0x01; // digi inputs 
        } else {
            port_paddle_en = (unsigned char*)&dummyReg;
            pin_paddle_en = 0;
            pin_paddle_en_match = 0xff;
        }
    } else {
        port_paddle_en = (unsigned char*)&dummyReg;
        pin_paddle_en = 0;
        pin_paddle_en_match = 0xff;
    }

    if (pin_paddle_en) {
        // really enabled
        port_paddle_in = (unsigned short*)(&ATD0DR0 + (flash4.paddle_in & 0x0f) - 8);

        if (flash4.paddle_out & 0x0f) {
            unsigned char tmp_opt;
            tmp_opt = flash4.paddle_out & 0x0f;
    //    paddle_out      [0:3], "Off", "PB4", "PM2", "PM3", "PM4", "PM5", "PA0", "PE4", "PT0", "PT1", "PT2", "PT3", "PT4", "PT5", "PT6", "PT7"
            if (tmp_opt == 1) {
                port_paddle_out = (unsigned char*)&PORTB;
                pin_paddle_out = 0x10;
            } else if (tmp_opt == 2) {
                port_paddle_out = (unsigned char*)&PTM;
                pin_paddle_out = 0x04;
            } else if (tmp_opt == 3) {
                port_paddle_out = (unsigned char*)&PTM;
                pin_paddle_out = 0x08;
            } else if (tmp_opt == 4) {
                port_paddle_out = (unsigned char*)&PTM;
                pin_paddle_out = 0x10;
            } else if (tmp_opt == 5) {
                port_paddle_out = (unsigned char*)&PTM;
                pin_paddle_out = 0x20;
            } else if (tmp_opt == 6) {
                port_paddle_out = (unsigned char*)&PORTA;
                pin_paddle_out = 1;
            } else if (tmp_opt == 7) {
                port_paddle_out = (unsigned char*)&PORTE;
                pin_paddle_out = 0x10;
            } else if (tmp_opt == 8) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x01;
                PWME &= ~0x01;
                TIOS |= 0x10;
            } else if (tmp_opt == 9) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x02;
                PWME &= ~0x02;
                TIOS |= 0x20;
            } else if (tmp_opt == 10) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x04;
                PWME &= ~0x04;
                TIOS |= 0x04;
            } else if (tmp_opt == 11) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x08;
                PWME &= ~0x08;
                TIOS |= 0x08;
            } else if (tmp_opt == 12) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x10;
                PWME &= ~0x10;
                TIOS |= 0x10;
            } else if (tmp_opt == 13) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x20;
                PWME &= ~0x20;
                TIOS |= 0x20;
            } else if (tmp_opt == 14) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x40;
                PWME &= ~0x40;
                TIOS |= 0x40;
            } else if (tmp_opt == 15) {
                port_paddle_out = (unsigned char*)&PTT;
                pin_paddle_out = 0x80;
                PWME &= ~0x80;
                TIOS |= 0x80;
            }
        }
    }
    paddle_timer = 0;
    paddle_phase = 0;

/* Manual mode */
    if ((flash4.manual & MANUAL_MASK) == MANUAL_MANSW) {
        unsigned char tmp_opt;
        tmp_opt = flash4.manual & 0x0f;
//    paddle_en       = bits,    U08,  312, [0:3], "Off", "PE0", "PE1", "PA0", "INVALID", "INVALID", "INVALID", "INVALID", "ADC0", "ADC1", "ADC2", "ADC3", "ADC4", "ADC5", "ADC6", "ADC7"

        if (tmp_opt == 1) {
            port_manual = (unsigned char*)&PORTE;
            pin_manual = 1;
        } else if (tmp_opt == 2) {
            port_manual = (unsigned char*)&PORTE;
            pin_manual = 2;
        } else if (tmp_opt == 3) {
            port_manual = (unsigned char*)&PORTA;
            pin_manual = 1;
        } else if (tmp_opt == 8) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 1;
            ATD0DIEN |= 0x01; // digi inputs 
        } else if (tmp_opt == 9) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 2;
            ATD0DIEN |= 0x02; // digi inputs 
        } else if (tmp_opt == 10) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 4;
            ATD0DIEN |= 0x04; // digi inputs 
        } else if (tmp_opt == 11) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 8;
            ATD0DIEN |= 0x01; // digi inputs 
        } else if (tmp_opt == 12) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 0x10;
            ATD0DIEN |= 0x10; // digi inputs 
        } else if (tmp_opt == 13) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 0x20;
            ATD0DIEN |= 0x20; // digi inputs 
        } else if (tmp_opt == 14) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 0x40;
            ATD0DIEN |= 0x40; // digi inputs 
        } else if (tmp_opt == 15) {
            port_manual = (unsigned char*)&PORTAD0;
            pin_manual = 0x80;
            ATD0DIEN |= 0x01; // digi inputs 
        } else {
            port_manual = (unsigned char*)&dummyReg;
            pin_manual = 0;
        }
    } else {
        port_manual = (unsigned char*)&dummyReg;
        pin_manual = 0;
    }

//VSS out
    port_vssout = (unsigned char*)&dummyReg;
    pin_vssout = 0x00;
    if (flash4.vssout_opt & 0x0f) {
        unsigned char tmp_opt;
        tmp_opt = flash4.vssout_opt & 0x0f;
//    vssout      [0:3], "Off", "PB4", "PM2", "PM3", "PM4", "PM5", "PA0", "PE4", "PT0", "PT1", "PT2", "PT3", "PT4", "PT5", "PT6", "PT7"
        if (tmp_opt == 1) {
            port_vssout = (unsigned char*)&PORTB;
            pin_vssout = 0x10;
        } else if (tmp_opt == 2) {
            port_vssout = (unsigned char*)&PTM;
            pin_vssout = 0x04;
        } else if (tmp_opt == 3) {
            port_vssout = (unsigned char*)&PTM;
            pin_vssout = 0x08;
        } else if (tmp_opt == 4) {
            port_vssout = (unsigned char*)&PTM;
            pin_vssout = 0x10;
        } else if (tmp_opt == 5) {
            port_vssout = (unsigned char*)&PTM;
            pin_vssout = 0x20;
        } else if (tmp_opt == 6) {
            port_vssout = (unsigned char*)&PORTA;
            pin_vssout = 1;
        } else if (tmp_opt == 7) {
            port_vssout = (unsigned char*)&PORTE;
            pin_vssout = 0x10;
        } else if (tmp_opt == 8) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x01;
            PWME &= ~0x01;
            TIOS |= 0x10;
        } else if (tmp_opt == 9) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x02;
            PWME &= ~0x02;
            TIOS |= 0x20;
        } else if (tmp_opt == 10) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x04;
            PWME &= ~0x04;
            TIOS |= 0x04;
        } else if (tmp_opt == 11) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x08;
            PWME &= ~0x08;
            TIOS |= 0x08;
        } else if (tmp_opt == 12) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x10;
            PWME &= ~0x10;
            TIOS |= 0x10;
        } else if (tmp_opt == 13) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x20;
            PWME &= ~0x20;
            TIOS |= 0x20;
        } else if (tmp_opt == 14) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x40;
            PWME &= ~0x40;
            TIOS |= 0x40;
        } else if (tmp_opt == 15) {
            port_vssout = (unsigned char*)&PTT;
            pin_vssout = 0x80;
            PWME &= ~0x80;
            TIOS |= 0x80;
        }
    }
    vssout_match = 0;

    //OD cancel
    port_odout = (unsigned char*)&dummyReg;
    pin_odout = 0x00;
    if ((trans == TRANS_E4ODA) || (trans == TRANS_E4ODS) || (trans == TRANS_W4A33)) {
        od_state = 1; /* OD cancel is going to be used */
        if (flash4.od_out & 0x0f) {
            unsigned char tmp_opt;
            tmp_opt = flash4.od_out & 0x0f;
            if (tmp_opt == 1) {
                port_odout = (unsigned char*)&PORTB;
                pin_odout = 0x10;
            } else if (tmp_opt == 2) {
                port_odout = (unsigned char*)&PTM;
                pin_odout = 0x04;
            } else if (tmp_opt == 3) {
                port_odout = (unsigned char*)&PTM;
                pin_odout = 0x08;
            } else if (tmp_opt == 4) {
                port_odout = (unsigned char*)&PTM;
                pin_odout = 0x10;
            } else if (tmp_opt == 5) {
                port_odout = (unsigned char*)&PTM;
                pin_odout = 0x20;
            } else if (tmp_opt == 6) {
                port_odout = (unsigned char*)&PORTA;
                pin_odout = 1;
            } else if (tmp_opt == 7) {
                port_odout = (unsigned char*)&PORTE;
                pin_odout = 0x10;
            } else if (tmp_opt == 8) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x01;
                PWME &= ~0x01;
                TIOS |= 0x10;
            } else if (tmp_opt == 9) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x02;
                PWME &= ~0x02;
                TIOS |= 0x20;
            } else if (tmp_opt == 10) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x04;
                PWME &= ~0x04;
                TIOS |= 0x04;
            } else if (tmp_opt == 11) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x08;
                PWME &= ~0x08;
                TIOS |= 0x08;
            } else if (tmp_opt == 12) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x10;
                PWME &= ~0x10;
                TIOS |= 0x10;
            } else if (tmp_opt == 13) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x20;
                PWME &= ~0x20;
                TIOS |= 0x20;
            } else if (tmp_opt == 14) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x40;
                PWME &= ~0x40;
                TIOS |= 0x40;
            } else if (tmp_opt == 15) {
                port_odout = (unsigned char*)&PTT;
                pin_odout = 0x80;
                PWME &= ~0x80;
                TIOS |= 0x80;
            }
        }
    } else {
        od_state = 0;
    }

    /* configure gear ratios */
    if (flash4.setting3 & SETTING3_GEARCUSTOM) {
        for (ix = 0 ; ix < 10; ix++) {
            gear_ratio[ix] = flash4.customgear[ix];
        }
    } else { /* use built-in ratios */
        for (ix = 0 ; ix < 10; ix++) {
            gear_ratio[ix] = builtingears[trans][ix];
        }
    }

    // for testing
    PORTT = 0xff; //enable all outputs  // check required default setting

    OC7M = 0; // don't want to use OC7 feature

    // set up ADC processing
    next_adc = 0;	   	// specifies next adc channel to be read
    ATD0CTL2 = 0x40;  // leave interrupt disabled, set fast flag clear
    ATD0CTL3 = 0x00;  // do 8 conversions/ sequence
    ATD0CTL4 = 0x67;  // 10-bit resoln, 16 tic cnvsn (max accuracy),
                    // prescaler divide by 16 => 2/3 us tic x 18 tics
    ATD0CTL5 = 0xB0;  // right justified,unsigned, continuous cnvsn,
                    // sample 8 channels starting with AN0  <-- change this for MS2?
    ATD0CTL2 |= 0x80;  // turn on ADC0
    // wait for ADC engine charge-up or P/S ramp-up
    for (ix = 0; ix < 160; ix++)  {
        while (!(ATD0STAT0 >> 7));	 // wait til conversion complete
        ATD0STAT0 = 0x80;
    }
    // get all adc values
    first_adc = 1;
    get_adc(0,7);
    first_adc = 0;

    adc_lmms = lmms;
    // Initialize variables
    flocker = 0;
    calc_coeff();

    /* Initialize CAN comms */
    // can_reset flag set above when flagbyte3 cleared
    CANid = flash4.mycan_id;
    CanInit();

//  stall_timeout_ess = 7812/flash4.divider; // calculate stall timeout for engine speed
    stall_timeout_ess = 7812;
    stall_ess = stall_timeout_ess+1;
//  stall_timeout_vss = 7812/flash4.reluctorteeth; // calculate stall timeout for vss
    stall_timeout_vss = 7812; // about 1 second
    vss1_stall = VSS_STALL_TIMEOUT + 1;
    adc_ctr = 0;
    lowres=0;
    lowres_ctr=0;
    burn = 0;
    /* all the rest of outpc set to zero earlier */
    outpc.gear = 4; // failsafe of 4th
    vss = 0;
    ess = 0;
    shift_timer = 0;
    shift_phase = 0;
    vss1_time_sum = 0;
    vss1_teeth = 0;
    shift_delay = 0;
    testmode_glob = 0;
    testmode_gear = 0;
    shiftret_timer = 0;
    gearsel_count = 0;
    old_selector = 0;

  // ignoring GM docs - surely safest to select 4th to prevent over-rev situation
// solA&B happen to be the same pins on uS, MS2 and GPIO
    if (outputmode == OUTPUTMODE_4L80E) {
        max_gear = 4;
  	    *port_sol[0] |= pin_sol[0]; // sol A
      	*port_sol[1] |= pin_sol[1]; // sol B
        if (pwm_sol[4]) {
            *pwm_sol[4] = 0;   // 3-2 not used
        }
    } else if (outputmode == OUTPUTMODE_4L60E) {
        max_gear = 4;
      	*port_sol[0] |= pin_sol[0];
      	*port_sol[1] &= ~pin_sol[1];
        if (pwm_sol[4]) {
            *pwm_sol[4] = 230; // 3-2 sol @ 90%
        }
    } else if (outputmode == OUTPUTMODE_A340E) {
        max_gear = 4;
      	*port_sol[0] &= ~pin_sol[0];
      	*port_sol[1] &= ~pin_sol[1];
        if (pwm_sol[4]) {
            *pwm_sol[4] = 0;   // 3-2 not used
        }
    } else if (outputmode == OUTPUTMODE_41TE) {
        max_gear = 4;
        *pwm_sol[0] = 0;
        *pwm_sol[1] = 0;
        *pwm_sol[2] = 255;
        *pwm_sol[3] = 255;
        gear_41te = 0x03; // to allow mainloop to do funny modulation stuff
        timer_41te = 0;
    } else if (outputmode == OUTPUTMODE_5L40E) {
        max_gear = 5;
      	*port_sol[0] &= ~pin_sol[0];
      	*port_sol[1] &= ~pin_sol[1];
        if (pwm_sol[4]) {
            *pwm_sol[4] = 0;   // 3-2 not used
        }
    } else if (outputmode == OUTPUTMODE_E4OD) {
        max_gear = 4;
      	*port_sol[0] &= ~pin_sol[0];
      	*port_sol[1] &= ~pin_sol[1];
        if (pwm_sol[4]) {
            *pwm_sol[4] = 0; // Coast clutch off
        }
    } else if (outputmode == OUTPUTMODE_W4A33) {
        max_gear = 4;
      	*port_sol[0] &= ~pin_sol[0];
      	*port_sol[1] &= ~pin_sol[1];
        if (pwm_sol[4]) {
            *pwm_sol[4] = 0;   // 3-2 not used
        }
    }

    // make trans speed sensor the highest priority interrupt
    HPRIO = 0xEE;

    // enable global interrupts
    ENABLE_INTERRUPTS

    return;
}
