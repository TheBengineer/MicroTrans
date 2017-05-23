/* $Id: serial.c,v 1.5 2014/12/22 19:38:06 jsm Exp $
 * Copyright 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is derived from Megasquirt-3.
 *
 * Origin: James Murray
 * Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */

/* mainloop portion of serial processing */

#include "trans.h"
#define DB_BUFADDR (unsigned int)&ram_data
#define DB_BUFSIZE 256 /* Limited to 256 due to serial send buffer */
#define DB_ENABLE 0

void chknewpage(unsigned char new_page)
{
    if ((new_page == 6) || (new_page == 7)) {
        return;
    }

    if (page != new_page) {
        if (outpc.status1 & STATUS1_NEEDBURN) {
            outpc.status1 |= STATUS1_LOSTDATA;
            outpc.status1 &= ~STATUS1_NEEDBURN;
        }
        page = 0; // in case an interrupt uses ram data. Beware of a fight with CAN.
        pg4_ptr = (page4_data *)&flash4;
        
        if (new_page == 4) { /* just the one */
            memcpy(tables[new_page].addrRam, tables[new_page].addrFlash, tables[new_page].n_bytes);
        }
        page = new_page;
        if (page == 4) {
            pg4_ptr = (page4_data *)&ram_data;
        }
    }
}

void serial()
{
    unsigned int size, ad, x, r_offset, r_size;
    unsigned long crc = 0; // =0 temp
    unsigned char cmd, r_canid, r_table, compat = 0;

    chk_crc();

    ad = 2;

    if (flagbyte3 & flagbyte3_kill_srl) {
        unsigned int lmms_tmp, timeout;
        if (next_txmode == 7) {
            timeout = 7812; // 1 second
        } else {
            timeout = 390; // about 50ms seconds of timeout
        }
        lmms_tmp = (unsigned int)lmms;
        if ((lmms_tmp - srl_timeout) > timeout) {
            unsigned char dummy;
            if (SCI0SR1 & 0x20) { // data available
                dummy = SCI0DRL; // gobble it up and wait again            
                srl_timeout = (unsigned int)lmms;
                return;
            }
            dummy = SCI0SR1; // step 1, clear any pending interrupts
            dummy = SCI0DRL; // step 2
            SCI0CR2 |= 0x24;        // rcv, rcvint re-enable

            rcv_timeout = 0xFFFFFFFF;
            flagbyte3 &= ~flagbyte3_kill_srl;
            if (next_txmode == 1) {
                srlbuf[2] = 0x8c; // parity error
            } else if (next_txmode == 2) {
                srlbuf[2] = 0x8d; // framing error
            } else if (next_txmode == 3) {
                srlbuf[2] = 0x8e; // noise
            } else if (next_txmode == 4) {
                srlbuf[2] = 0x81; // overrun
            } else if (next_txmode == 5) {
                srlbuf[2] = 0x8f; // Transmit txmode range
            } else if (next_txmode == 6) {
                srlbuf[2] = 0x84; // Out of range
            } else if (next_txmode == 7) {
                const char errstr[] = "Too many bad requests! Stop doing that!";
                srlbuf[2] = 0x91; // Too many!

                memcpy((unsigned char*)&srlbuf[3], (unsigned char*)&errstr, sizeof(errstr));
                txgoal = sizeof(errstr);
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            } else {
                srlbuf[2] = 0x90; // unknown
            }
            txgoal = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
    }
    
    /* check for under-run or old commands */
    if (rxmode) {
        unsigned long lmms_tmp;

        DISABLE_INTERRUPTS;
        lmms_tmp = lmms;
        ENABLE_INTERRUPTS;

        if (lmms_tmp > rcv_timeout) {
            /* had apparent under-run, check for old commands */
            cmd = srlbuf[0];
            if ((rxmode == 1) && (cmd == 'Q')) {
                ad = 0;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'S')) {
                ad = 0;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'C')) {
                ad = 0;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'F')) {
                ad = 0;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'I')) {
                ad = 0;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'A')) {
                ad = 0;
                size = 1;
                compat = 1; // full A
                rxmode = 0;
                goto SERIAL_OK;

#if DB_ENABLE
            } else if ((rxmode == 1) && (cmd == 'd')) {
                chknewpage(99); // force swap to fake page
                debug_init();
                rxmode = 0;
                flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            } else if ((rxmode == 1) && (cmd == 'D') && (page == 99)) {
                unsigned int cnt, cnt2, pt1;
                txgoal = 0;
                /* debug mode */
                if (flagbyte15 & FLAGBYTE15_DB_WRAP) {
                    cnt2 = DB_BUFSIZE;
                    pt1 = db_ptr;
                } else {
                    cnt2 = db_ptr;
                    pt1 = 0;
                }
                for (cnt = 0 ; cnt < cnt2 ; cnt++) {
                    unsigned char c;
                    c = *(unsigned char*)(DB_BUFADDR + pt1);
                    pt1++; /* gcc makes better code with this on its own line */
                    if (pt1 >= DB_BUFSIZE) {
                        pt1 = 0;
                    }
                    /* change 0x0a into 0x0d 0x0a */
                    if (c) {
                        if (c == 0x0a) {
                            srlbuf[txgoal] = 0x0d;
                            txgoal++;
                        } else if ((c != 0x0a) && (c != 0x0d) && ((c < 32) || (c > 127))) {
                            c = '.';
                        }
                        srlbuf[txgoal] = c;
                        txgoal++;
                    }
                }
                rxmode = 0;
                /* send as unwrapped data */
                txcnt = 0;
                txmode = 129;
                SCI0DRL = srlbuf[0]; // SCIxDRL
                SCI0CR2 |= 0x88; // xmit enable & xmit interrupt enable // SCIxCR2
                flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
#endif

            } else if ((rxmode == 2) && (cmd == 'r') && (srlbuf[1] == 0x00) && (srlbuf[2] == 0x04)) {
                /* Megaview request config data command */
                ad = 0;
                size = 7;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 2) && (cmd == 'a') && (srlbuf[1] == 0x00) && (srlbuf[2] == 0x06)) {
                /* Megaview read command */
                ad = 0;
                srlbuf[0] = 'A'; /* fake A command */
                size = 1;
                compat = 2; // MV read
                rxmode = 0;
                goto SERIAL_OK;
            } else {
                /* had some undefined under-run situation */
                srlbuf[2] = 0x80;
                txgoal = 0;

                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                rxmode = 0;
                goto EXIT_SERIAL;
            }
        }
    }

    /* SERIAL_BURN response removed for MS2 */

    if (flagbyte14 & FLAGBYTE14_SERIAL_OK) {
        flagbyte14 &= ~FLAGBYTE14_SERIAL_OK;
        goto RETURNOK_SERIAL;
    }

    if (flagbyte15 & FLAGBYTE15_CANRX) {
        flagbyte15 &= ~FLAGBYTE15_CANRX;
        srlbuf[2] = 0x06; // OK code
        txgoal = *(unsigned int*)&srlbuf[0]; // what was stored there previously
        flagbyte3 &= ~flagbyte3_getcandat;
        cp_targ = 0;
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

    if (flagbyte3 & FLAGBYTE3_REMOTEBURNSEND) {
        /* Got an ACK packet, let TS know */
        flagbyte3 &= ~(flagbyte3_getcandat | FLAGBYTE3_REMOTEBURN | FLAGBYTE3_REMOTEBURNSEND);
        goto RETURNOK_SERIAL;
    }

    if (flagbyte3 & flagbyte3_getcandat) {
        unsigned int ti;
        DISABLE_INTERRUPTS;
        ti = (unsigned int)lmms - cp_time;
        ENABLE_INTERRUPTS;

        if ((flagbyte3 & FLAGBYTE3_REMOTEBURN) && (ti > 469)) { /* 60ms */
            /* If an ack packet was received, the flag would be cleared
                no method to know if other end supports it, so have to say ok now
               This forces TS to wait some time for remote burn to complete */
            flagbyte3 &= ~(flagbyte3_getcandat | FLAGBYTE3_REMOTEBURN);
            goto RETURNOK_SERIAL;
        }

        if (ti > 1000) {
            /* taking too long to get a reply... bail on it */
            srlbuf[2] = 0x8a; // CAN timeout
            txgoal = 0;
            flagbyte3 &= ~flagbyte3_getcandat;
            cp_targ = 0;
            if (can_getid < 15) { // sanity check
                can_err_cnt[can_getid]++; // increment error counter
            }
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (flagbyte14 & FLAGBYTE14_CP_ERR) {
            flagbyte3 &= ~flagbyte3_getcandat;
            flagbyte14 &= ~FLAGBYTE14_CP_ERR;
            srlbuf[2] = 0x8b; // CAN failure
            txgoal = 0;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
        if (flagbyte14 & FLAGBYTE14_SERIAL_PROCESS) {
            goto BUSY_SERIAL;
        }
    }

    if (flagbyte3 & flagbyte3_sndcandat) {
        unsigned int ti;
        DISABLE_INTERRUPTS;
        ti = (unsigned int)lmms - cp_time;
        ENABLE_INTERRUPTS;

        if (ti > 1000) {
            srlbuf[2] = 0x8a;
            txgoal = 0;
            flagbyte3 &= ~flagbyte3_sndcandat;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (flagbyte14 & FLAGBYTE14_CP_ERR) {
            flagbyte3 &= ~flagbyte3_sndcandat;
            flagbyte14 &= ~FLAGBYTE14_CP_ERR;
            srlbuf[2] = 0x8b; // CAN failure - not sure that TS will be expecting this reply now...
            txgoal = 0;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
    }

    if (!(flagbyte14 & FLAGBYTE14_SERIAL_PROCESS)) {
        if (flagbyte14 & FLAGBYTE14_SERIAL_FWD) {
            /* pass on forwarded CAN data if not doing any other transaction */
            flagbyte14 &= ~FLAGBYTE14_SERIAL_FWD;
            txgoal = canbuf[16];
            srlbuf[2] = 6; /* CAN data */
            for (x = 0; x < txgoal; x++) {
                srlbuf[3 + x] = canbuf[x];
            }
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else {
            return;
        }
    }

    size = *(unsigned int*)&srlbuf[0]; // size of payload
    /* calc CRC of data */
    crc = crc32buf(0, (unsigned int)&srlbuf[2], size);    // incrc, buf, size

    if (crc != *(unsigned long*)&srlbuf[2 + size]) {
        /* CRC did not match - build return error packet */
        srlbuf[2] = 0x82; // CRC failed code
        txgoal = 0;
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

    /* Get here when packet looks ok */
SERIAL_OK:;

/*
**************************************************************************
; **
; ** SCI Communications - within size/CRC32 wrapper
; **
; ** Communications is established when the PC communications program sends
; ** a command character - the particular character sets the mode:
; **
; *  "a"+<canid>+<table id> = send all of the realtime display variables (outpc structure) via txport.
; *  "A" send all realtime vars
; *  "b"+<canid>+<table id> =  burn a ram data block into a flash data block.
; *  "c" = Test communications - echo back U16 Seconds
; ** "D" = Display 'debug buffer'
;    "e" = removed
; *  "F" = return serial version in ASCII e.g. 001
;    "h"+<0/1> = Broadcast a CAN 'halt' or 'unhalt' 1 = halt, 0 = unhalt.
; *  "f"+<canid> = return U08 serial version, U08 blocking factor table, U16 blocking factor write
; *  "I" = return CANid in binary
; 'k'+<canid>+<table id>+offset+size = return CRC of data page
;    "M" = return two byte monitor version
; *  "Q" = Send over Embedded Code Revision Number
; *  "r"+<canid>+<table id>+<offset lsb>+<offset msb>+<nobytes> = read and
;     send back value of a data parameter(s) in offset location within table
; *  "S" = Send program title.
;    "T" = removed
;    "t" = removed
;    "w"+<canid>+<table id>+<offset lsb>+<offset msb>+<nobytes>+<newbytes> =
;     receive updated data parameter(s) and write into offset location
;     relative to start of data block
;    "y" = removed
; 
; Generally commands require the 'newserial' wrapper.
; * = supported wrapped or unwrapped for compatability.
;** = supported unwrapped only - intended for use in Miniterminal
;
; **
; **************************************************************************
*/

    cmd = srlbuf[ad];
    if (size == 1) {
        /* single character commands */
        if (cmd == 'A') {
            if (!compat) {
                if (conf_err) {
                    /* if there's a configuration error, then we'll send that back instead of the realtime data */
                    srlbuf[2] = 0x03;    /* config error packet */
                    txgoal = conf_str(); // function copies config error string to serial buffer
                    if (conf_err >= 200) {
                        conf_err = 0;
                    }
                    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                    goto EXIT_SERIAL;
                } else {
                    srlbuf[2] = 1;    /* realtime data packet */
                    ad++;
                }
            }

            x = ad;
            if (compat < 2) {
                txgoal = sizeof(outpc);
            } else {
                txgoal = 112;
            }
            /* No MegaView compatability mode */
            // takes ~50us to copy the lot over (MS3 timing)
            memcpy((unsigned char*)&srlbuf[3], (unsigned char *)&outpc, txgoal);

            if (compat < 2) {
            } else {
                /* fill rest of compatability area with zeros */
                for (x = (3 + 72) ; x <= (3 + 112) ; x += 2) {
                    *(unsigned int*)&srlbuf[x] = 0;
                }
            }

            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else if (cmd == 'C') {
            /* test data packet */
            if (!compat) {
                srlbuf[ad] = 0; /* OK */
                ad++;
            }
            x = ad;
            *(unsigned int*)&srlbuf[ad] = outpc.seconds;
            txgoal = 2;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else if (cmd == 'Q') {
            /* serial format */
            if (!compat) {
                srlbuf[ad] = 0; /* OK */
                ad++;
            }
            for (x = 0; x < SIZE_OF_REVNUM; x++) {
                srlbuf[x + ad] = RevNum[x];
            }
            txgoal = SIZE_OF_REVNUM;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'S') {
            /* string for title bar */
            if (!compat) {
                srlbuf[ad] = 0;
                ad++;
            }
            for (x = 0; x < SIZE_OF_SIGNATURE; x++) {
                srlbuf[x + ad] = Signature[x];
            }
            txgoal = SIZE_OF_SIGNATURE;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'F') {
            /* format i.e. 001 = newserial */
            if (!compat) {
                srlbuf[ad] = 0;
                ad++;
            }
            srlbuf[ad++] = '0';
            srlbuf[ad++] = '0';
            srlbuf[ad++] = SRLVER + 48;
            txgoal = 3;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'I') {
            /* return my CANid */
            if (!compat) {
                srlbuf[ad] = 0;
                ad++;
            }
            srlbuf[ad] = CANid;
            txgoal = 1;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'M') {
            /* return monitor version */
            srlbuf[ad] = 0;
            ad++;
            *(unsigned int*)&srlbuf[ad] = *(unsigned int*)0xfefe;
            txgoal = 2;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 3) && (cmd == 'b')) {
        /* need a way of avoiding clashes between remotely and locally initiated burn */
        r_canid = srlbuf[3] & 0x0f;
        r_table = srlbuf[4];

        if ((r_canid >= MAX_CANBOARDS) || (r_table >= NO_CANTBLES)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int r;
            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            DISABLE_INTERRUPTS;
            r = can_sendburn(r_table, r_canid);
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }

            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            if (r_table >= NO_TBLES) {
                goto INVALID_OUTOFRANGE;
            }

            /* local burn */

            if (flagbyte1 & flagbyte1_tstmode) {
                /* no more burning during test mode */
                goto BUSY_SERIAL;
            }

            chknewpage(r_table); // must be sure that this page IS loaded to RAM in case of out of order serial commands
            resetholdoff = 15;
            burn_idx = r_table;
            Flash_Init(); // check FDIV written to (should be at reset) likely safe?
            flocker = 0xcc; // set semaphore to prevent burning flash thru runaway code
            outpc.status1 &= ~(STATUS1_NEEDBURN | STATUS1_LOSTDATA);
            burntbl(0);
            flocker = 0;
            /* After a Burn, TunerStudio will ask for a CRC, but that's only
             of the RAM copy, so it does not check the flash is correct.
             Force a re-copy from flash to RAM so we really know it is ok. */
            page = 0;
            chknewpage(r_table);
            burn = 1;
            srlbuf[2] = 0x04; // burn OK code
            txgoal = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;

            goto EXIT_SERIAL;
        }

    } else if ((size == 7) && (cmd == 'k')) {
        /* CRC of 1k ram page */
        r_canid = srlbuf[3] & 0x0f;
        r_table = srlbuf[4];
        /* ignore the other 4 bytes */

        if ((r_canid >= MAX_CANBOARDS) || (r_table >= NO_CANTBLES)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int r;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            canrxad = 3;
            canrxgoal = 4;
            *(unsigned int*)&srlbuf[0] = canrxgoal;

            DISABLE_INTERRUPTS;
            r = can_crc32(r_table, r_canid);
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            /* local CRC */
            if (r_table >= NO_TBLES) {
                goto INVALID_OUTOFRANGE;
            }
            srlbuf[2] = 0;
            ad++;

            if (r_table >= 4 ) {
                chknewpage(r_table); // copies from flash to RAM if needed
                crc = crc32buf(0, (unsigned int)&ram_data, tables[r_table].n_bytes);    // incrc, buf, size
            } else {
                /* CRC of calibration pages, function in same PPAGE as data */
                crc = crc32buf(0, (unsigned int)tables[r_table].addrFlash, tables[r_table].n_bytes);    // incrc, buf, size
            }

            *(unsigned long*)&srlbuf[ad] = crc;
            txgoal = 4;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 7) && (cmd == 'r')) {
        ad++;
        r_canid = srlbuf[ad];
        ad++;
        r_table = srlbuf[ad];
        ad++;
        r_offset = *(unsigned int*)&srlbuf[ad];
        ad += 2;
        if (compat == 1) {
            r_size = 256;
        } else {
            r_size = *(unsigned int*)&srlbuf[ad];
        }

        if ((r_canid >= MAX_CANBOARDS)
            || ((r_table >= NO_CANTBLES) && ((r_table < 0xf0) || (r_table > 0xf4)))
            || (r_size > SRLDATASIZE) ) { // 256 bytes is the max size on MS2/Extra
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            unsigned int by, r = 0;

            canrxad = 3;
            canrxgoal = r_size;
            *(unsigned int*)&srlbuf[0] = canrxgoal;

            if (r_size <= 8) {
                by = r_size;
            } else {
                by = 8;
                /* set up other passthrough read, variables handled in CAN TX ISR */
                cp_id = r_canid;
                cp_table = r_table;
                cp_targ = r_size;
                cp_cnt = 0;
                cp_offset = r_offset;
            }

            DISABLE_INTERRUPTS;
            r = can_reqdata(r_table, r_canid, r_offset, by);
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                cp_targ = 0;
                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }

            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            unsigned int ram_ad;

            if ((r_table >= 0xf0) && (r_table <= 0xf4)) {
                    /* not supported */
                    goto INVALID_OUTOFRANGE;
            } else {
                /* local read */
                if ( ((r_table >= NO_TBLES)
                    || ((r_table != 7) && ((r_offset + r_size) > tables[r_table].n_bytes))
                    || ((r_table == 7) && (r_offset < 0x200) && ((r_offset + r_size) > tables[7].n_bytes))
                    || ((r_table == 7) && (r_offset >= 0x200) && ((r_offset + r_size) > (0x200 + sizeof(datax1))))
                    )) {
                    /* last two lines to handle outpc vs. datax1 in same 'page' */
                    goto INVALID_OUTOFRANGE;
                }


                if ((r_table == 7) && (r_offset >= 0x200)) {
                    ram_ad = (unsigned int)&datax1;
                    r_offset -= 0x200;
                } else {
                    ram_ad = (unsigned int)tables[r_table].addrRam;
                }
            }

            if (ram_ad == 0) {
                /* No valid RAM copy to read back
                    This will happen if user tries to read calibration tables. */
                goto INVALID_OUTOFRANGE;
            }

            chknewpage(r_table); // copies from flash to RAM if needed

            if (!compat) {
                srlbuf[2] = 0; // OK
                ad = 3;
            } else {
                ad = 0;
            }
            memcpy((unsigned int*)&srlbuf[ad], (unsigned int*)(ram_ad + r_offset), r_size);

            txgoal = r_size;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
        
    } else if ((size > 7) && ((cmd == 'w') || (cmd == 'x'))) {
        r_canid = srlbuf[3] & 0x0f;
        r_table = srlbuf[4];
        r_offset = *(unsigned int*)&srlbuf[5];
        r_size = *(unsigned int*)&srlbuf[7];

        if ((r_canid >= MAX_CANBOARDS)
            || (r_size > 0x4000) || (r_offset >= 0x4000) /* Prevent wrap-around overflow */
            || (r_table >= NO_CANTBLES)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int by, r = 0;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            if (r_size <= 8) {
                by = r_size;
            } else {
                by = 8;
                /* set up a passthrough write, handled in CAN TX ISR */
                /* when all packets in queue, flag will be set to tell PC */
                flagbyte3 |= flagbyte3_sndcandat;
                cp_id = r_canid;
                cp_table = r_table;
                cp_targ = r_size;
                cp_cnt = 8;
                cp_offset = r_offset;
            }
            DISABLE_INTERRUPTS;
            r = can_snddata(r_table, r_canid, r_offset, by, (unsigned int)&srlbuf[9]);
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }

            if (r_size <= 8) {
                /* for a single packet say ok now */
                if (cmd == 'x') {
                    /* no-ACK command */
                    flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
                    goto EXIT_SERIAL;
                } else {
                    goto RETURNOK_SERIAL;
                }
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else {
            unsigned char ch = 0;
            unsigned int ram_ad;
            /* local write */
            if (r_table >= NO_TBLES) {
                goto INVALID_OUTOFRANGE;
            }
            if (cmd != 'w') {
                goto INVALID_SERIAL;
            }

            if ((r_table > NO_TBLES)
                || ((r_table != 7) && ((r_offset + r_size) > tables[r_table].n_bytes))
                || ((r_table == 7) && (r_offset < 0x200) && ((r_offset + r_size) > tables[7].n_bytes))
                || ((r_table == 7) && (r_offset >= 0x200) && ((r_offset + r_size) > (0x200 + sizeof(datax1))))
                ) {
                goto INVALID_OUTOFRANGE;
            }

            /* copy to ram page */
            /* FIXME - validate page numbers using a field in tables? */
            if (r_table != 6) { 

                if (r_table == 7) {
                    if (r_offset >= 0x200) {
                        ram_ad = (unsigned int)&datax1 + r_offset - 0x200;
                    } else {
                        ram_ad = (unsigned int)tables[r_table].addrRam + r_offset;
                    }
                } else if (r_table <= 3) { // calibration data
                    if ((pg4_ptr->flashlock & 1) == 0) {
                        srlbuf[2] = 0x86; // invalid command code
                        txgoal = 1;
                        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                        goto EXIT_SERIAL;
                    }
                    ram_ad = (unsigned int)&ram_data + (r_offset & 0x3ff);
                    chknewpage(r_table);
                } else {
                    ram_ad = (unsigned int)tables[r_table].addrRam + r_offset;
                    chknewpage(r_table);
                }

                ch = g_read_copy((unsigned int)&srlbuf[9], r_size, ram_ad);// src, count, dest

                if (r_table <= 3) { // calibration data
                    /* At end of each 1k block perform a burn automatically. */
                    if ((r_size == 0x100) && ((r_offset == 0x300) || (r_offset == 0x700))) {
                        /* Note that this does allow misbehaving tuning software to cause a partial burn. */
                        resetholdoff = 15;
                        burn_idx = r_table;
                        Flash_Init(); // check FDIV written to (should be at reset) likely safe?
                        flocker = 0xcc; // set semaphore to prevent burning flash thru runaway code
                        burntbl(r_offset - 0x300);
                    }
                } else if ((r_table == 7) && (r_offset >= 0x200)
                        && ((r_offset - 0x200) <= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1))
                        && ((r_offset - 0x200 + r_size) >= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1) + 1)) {
                    /* test mode special code */
                    if (datax1.testmodelock == 12345) {
                        unsigned int ret;
                        ret = do_testmode();
                        if (ret == 0) {
                            goto RETURNOK_SERIAL;
                        } else if (ret == 1) {
                            goto BUSY_SERIAL;
                        } else if (ret == 2) {
                            goto INVALID_OUTOFRANGE;
                        } else if (ret >= 3) {
                            /* undefined, return error */
                            goto INVALID_OUTOFRANGE;
                        }
                    } else {
                        flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
                        outpc.status3 &= ~STATUS3_TESTMODE;
                    }
                }

                if (ch &&
                    ((r_table == 4) || (r_table == 5) || (r_table == 8)
                    || (r_table == 9) || (r_table == 10) || (r_table == 11)
                    || (r_table == 12) )) { /* only applies to tuning data pages */
                    if (!(flagbyte1 & flagbyte1_tstmode)) {
                        outpc.status1 |= STATUS1_NEEDBURN; /* flag burn needed if anything changed */
                    }
                }
                srlbuf[2] = 0x00; // ok code
            } else {
                goto INVALID_OUTOFRANGE;
            }

            txgoal = 0;

            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 2) && (cmd == 'f')) {
        r_canid = srlbuf[3] & 0x0f;
        if (r_canid != CANid) {
            unsigned int r = 0;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            canrxad = 3;
            canrxgoal = 5;
            *(unsigned int*)&srlbuf[0] = canrxgoal;

            DISABLE_INTERRUPTS;
            r = can_sndMSG_PROT(r_canid, 5); /* request long form of protocol */
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else {
            /* local
            <size><type><Prot><blockingFactorTable><blockingFactorWrite><CRC> */
            srlbuf[2] = 0;
            srlbuf[3] = SRLVER;
            *(unsigned int*)&srlbuf[4] = SRLDATASIZE;
            *(unsigned int*)&srlbuf[6] = SRLDATASIZE;
            txgoal = 5;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 2) && (cmd == 'h')) {
            unsigned int r = 0;

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            DISABLE_INTERRUPTS;
            r = can_sndMSG_SPND(srlbuf[3]);
            ENABLE_INTERRUPTS;
            if (r) {
                srlbuf[2] = 0x89; // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            goto RETURNOK_SERIAL;

    } else if ((size == 14)
        && (srlbuf[2] == '!')
        && (srlbuf[3] == '!')
        && (srlbuf[4] == '!')
        && (srlbuf[5] == 'S')
        && (srlbuf[6] == 'a')
        && (srlbuf[7] == 'f')
        && (srlbuf[8] == 'e')
        && (srlbuf[9] == 't')
        && (srlbuf[0xa] == 'y')
        && (srlbuf[0xb] == 'F')
        && (srlbuf[0xc] == 'i')
        && (srlbuf[0xd] == 'r')
        && (srlbuf[0xe] == 's')
        && (srlbuf[0xf] == 't')) {

        DISABLE_INTERRUPTS;
        PORTE &= ~0x10;            // turn off fuel pump.  User is instructed to have coils wired via the relay

        monitor();

        /* Note that legacy 't' and 'T' commands are not supported - use 'w' instead */
    }

/* ------------------------------------------------------------------------- */
INVALID_SERIAL:;
/* Should not be reached - must be invalid command - build return error packet */
    srl_err_cnt++;
    srlbuf[2] = 0x83; // invalid command code
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
DEAD_CAN:;
    const char errstr[] = "CAN device dead, stop sending requests.";
    srlbuf[2] = 0x92;

    memcpy((unsigned char*)&srlbuf[3], (unsigned char*)&errstr, sizeof(errstr));
    txgoal = sizeof(errstr);
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
BUSY_SERIAL:;
    srlbuf[2] = 0x85; // busy
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
RETURNOK_SERIAL:;
    srlbuf[2] = 0x00; // ok
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
INVALID_OUTOFRANGE:;
/* Should not be reached - must be invalid command - build return error packet */
    srl_err_cnt++;
    srlbuf[2] = 0x84; // out of range command code
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */

EXIT_SERIAL:;
    flagbyte14 &= ~FLAGBYTE14_SERIAL_PROCESS;

    if (flagbyte14 & FLAGBYTE14_SERIAL_SEND) {
        if (srl_err_cnt > 3) {
            next_txmode = 7;
            rxmode = 0;
            txmode = 0;
            (void)SCI0DRL; // read assumed garbage data to clear flags
            SCI0CR2 &= ~0xAC;   // rcv, xmt disable, interrupt disable
            flagbyte3 |= flagbyte3_kill_srl;
            srl_timeout = (unsigned int)lmms;
            return;
        }

        if (!compat) {
            txgoal++; /* 1 byte return code */

            *(unsigned int*)&srlbuf[0] = txgoal;
            *(unsigned long*)&srlbuf[txgoal + 2] = crc32buf(0, (unsigned int)srlbuf +  2, txgoal);
            txgoal += 6; /* 2 bytes size, 4 bytes CRC */
        }
        next_txmode = 0;
        txcnt = 0;
        txmode = 129;
        SCI0DRL = srlbuf[0]; // SCIxDRL
        SCI0CR2 |= 0x88; // xmit enable & xmit interrupt enable // SCIxCR2
        flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
    }
    return;
}

/* ------------------------------------------------------------------------- */
#if DB_ENABLE
void debug_init(void)
{
    db_ptr = 0;
    flagbyte15 &= ~FLAGBYTE15_DB_WRAP;
    debug_str("Debug buffer ");
    debug_inthex(MONVER);
    debug_str("\r");
    debug_str("======================\r");
}

void debug_str(unsigned char* str)
{
    unsigned char c, m = 0;
    if (page == 99) { /* Debug magic number */
        do {
            c = *str;
            str++;
            if (c) {
                debug_byte(c);
            }
            m++;
        } while ((c != 0) && (m < 80));
    }
}

void debug_byte(unsigned char c)
{
    if (page == 99) { /* Debug magic number */
        *(unsigned char*)(DB_BUFADDR + db_ptr) = c;
        db_ptr++;
        if (db_ptr >= DB_BUFSIZE) {
            db_ptr = 0;
            flagbyte15 |= FLAGBYTE15_DB_WRAP;
        }
    }
}

void debug_bytehex(unsigned char b)
{
    /* logs byte b in hex */
    unsigned char c;

    c = b >> 4;
    if (c < 10) {
        c += 48;
    } else {
        c += 55;
    }
    debug_byte(c);

    c = b & 0x0f;
    if (c < 10) {
        c += 48;
    } else {
        c += 55;
    }
    debug_byte(c);
}

void debug_bytedec(unsigned char b)
{
    /* logs byte b in decimal, 3 digits */
    unsigned char c;

    c = b / 100;
    c += 48;
    debug_byte(c);

    c = (b / 10) % 10;
    c += 48;
    debug_byte(c);

    c = b % 10;
    c += 48;
    debug_byte(c);
}

void debug_byte2dec(unsigned char b)
{
    /* logs byte b in decimal, 2 digits */
    unsigned char c;

    c = (b / 10) % 10;
    c += 48;
    debug_byte(c);

    c = b % 10;
    c += 48;
    debug_byte(c);
}

void debug_inthex(unsigned int b)
{
    debug_bytehex((b >> 8) & 0xff);
    debug_bytehex((b >> 0) & 0xff);
}

void debug_longhex(unsigned long b)
{
    debug_bytehex((b >> 24) & 0xff);
    debug_bytehex((b >> 16) & 0xff);
    debug_bytehex((b >> 8) & 0xff);
    debug_bytehex((b >> 0) & 0xff);
}
#endif
