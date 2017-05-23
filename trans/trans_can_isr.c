/* $Id: trans_can_isr.c,v 1.5 2015/01/19 22:32:06 jsm Exp $
 * Copyright 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013
 * James Murray and Kenneth Culver
 *
 * This file is a part of MS2/Extra.
 *
 * CanTxIsr()
    Majority: James Murray
 * CanRxIsr()
    Origin: Al Grippo.
    Moderate: Additions by James Murray / Jean Belanger. Rewrite TX method James Murray.
    Majority: Al Grippo / James Murray / Jean Belanger
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/

#include "trans.h"

INTERRUPT void CanTxIsr(void)
{
    CANTIER = 0; /* Do not want any interrupts. */
    return;
}

INTERRUPT void CanRxIsr(void)
{
    unsigned char rcv_id,msg_type,var_blk,var_byt,jx,kx;
    unsigned short var_off;
    unsigned char tmp_data[8];
    unsigned char rem_table, rem_bytes;
    unsigned int rem_offset, ad_tmp, dest_addr;

    /* CAN Recv Interrupt */
    if (CANRFLG & 0x01)  {
        var_off = ((unsigned short)CAN_RB_IDR0 << 3) |
            ((CAN_RB_IDR1 & 0xE0) >> 5);
        msg_type = (CAN_RB_IDR1 & 0x07);
        if(msg_type == MSG_XTND)
            msg_type = CAN_RB_DSR0;
        rcv_id = CAN_RB_IDR2 >> 4;		// message from device rcv_id
        var_blk = (CAN_RB_IDR3 >> 4) | ((CAN_RB_IDR3 & 0x08) << 1);
        var_byt = (CAN_RB_DLR & 0x0F);

        if (var_off >= 0x4000) {
            /* Impossible! */
            /* in ver 2 protocol will send back error reply */
            goto CAN_DONERXSW;
        }

        switch(msg_type)  {
            case MSG_CMD:  // msg for this ecu to set a variable to val in msg
            case MSG_RSP:  // msg in reply to this ecu's request for a var val
                // value in the data buffer in recvd msg
                if ((msg_type == MSG_RSP) && (flagbyte3 & flagbyte3_getcandat) && (var_blk == 6))  {
                    // only forward on via serial if doing CAN stuff and requested to dump into txbuf
                    // set up for serial xmit of the recvd CAN bytes (<= 8)
                    //  MT getting back data requested from aux board

                    if (var_byt && (var_byt <= 8)) {
                        for (jx = 0; jx < var_byt; jx++) {
                            srlbuf[canrxad++] = *(&CAN_RB_DSR0 + jx);
                            if (canrxgoal) {
                                canrxgoal--;
                            }
                        }

                        if (canrxgoal == 0) {
                            /* not expecting any more data */
                            flagbyte15 |= FLAGBYTE15_CANRX; /* tell mainloop to do something with the data */
                        } else {
                            unsigned char num2;
                            unsigned int r;

                            cp_cnt += var_byt; /* number of bytes received */
                            num2 = canrxgoal; /* number of bytes left */

                            if (num2 > 8) {
                                num2 = 8;
                            }

                            r = can_reqdata(cp_table, cp_id, cp_offset + cp_cnt, num2);

                            if (r) {
                                /* flag up the error so serial.c can act*/
                                flagbyte14 |= FLAGBYTE14_CP_ERR;
                            }
                        }
                    } else {
                        /* bogus message received */
                        /* turn off CAN receive mode */
                        flagbyte3 &= ~flagbyte3_getcandat;
                    }

                } else  {
                    unsigned char ck = 0;
                    dest_addr = 0;
                    ad_tmp = 0;

                    if (var_blk <= 3) {
                        if ((pg4_ptr->flashlock & 1) == 0) {
                            dest_addr = 0;
                            /* ideally want to send an error code back to the sender */
                        } else {
                            chknewpage(var_blk);
                            dest_addr = (unsigned int)&ram_data;
                            ad_tmp = dest_addr + (var_off & 0x3ff);
                        }
                    } else if (var_blk == 6) {
                        dest_addr = (unsigned int)&canbuf;
                        ad_tmp = dest_addr + var_off;

                    } else if (var_blk == 7) {
                        // special case for receiving CAN data
                        if (var_off >= 0x200) { // datax1 sharing same block no. as outpc
                            var_off -= 0x200; // but at addresses 0x200 and beyond
                            if ((var_off + var_byt) > sizeof(datax1)) {
                                flagbyte3 |= flagbyte3_can_reset;
                            } else {
                                dest_addr = (unsigned int) &datax1;
                            }
                        } else {
                            if ((var_off + var_byt) > sizeof(outpc)) {
                                flagbyte3 |= flagbyte3_can_reset;
                            } else {
                                dest_addr = (unsigned int) &outpc;
                            }
                        }
                        ad_tmp = dest_addr + var_off;

                    } else {
                        // concurrent access to tuning data via serial and CAN will cause corruption
                        chknewpage(var_blk);
                        dest_addr = (unsigned int)tables[var_blk].addrRam;
                        ck = 1;
                        ad_tmp = dest_addr + var_off;
                    }

                    if ((var_blk != 7) && ((var_off + var_byt) > tables[var_blk].n_bytes)) {
                        flagbyte3 |= flagbyte3_can_reset;
                        dest_addr = 0;
                    }

                    if (dest_addr) {

                        for (jx = 0;jx < var_byt;jx++)  {
                            if (ck && (*((unsigned char *)(ad_tmp + jx)) != *(&CAN_RB_DSR0 + jx) )) {
                                outpc.status1 |= STATUS1_NEEDBURN; // we changed some tuning data
                            }
                            *((unsigned char *)(ad_tmp + jx)) = *(&CAN_RB_DSR0 + jx);
                        }

                        if (var_blk <= 3) { // calibration data
                            /* At end of each 1k block perform a burn automatically. */
                            if ((var_off == 0x3f8) || (var_off == 0x7f8)) {
                                /* Note that this does allow misbehaving tuning software to cause a partial burn. */
                                resetholdoff = 15;
                                burn_idx = var_blk;
                                Flash_Init(); // check FDIV written to (should be at reset) likely safe?
                                flocker = 0xcc; // set semaphore to prevent burning flash thru runaway code
                                burntbl(var_off - 0x3f8); /* burn stumble is inevitable, so handling this in the interrupt is 'ok' */
                            }
                        } else if ((var_blk == 7) && (dest_addr == (unsigned int)&datax1)
                            && (var_off <= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1))
                            && ((var_off + var_byt) >= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1) + 1)) {
                            /* was the write to datax1 and covering the testmodelock.
                             (To ensure data capture doesn't disturb testmode.) */
                            /* test mode special code */
                            if (datax1.testmodelock == 12345) {
                                unsigned int ret;
                                ret = do_testmode();
                                /* no method of acting upon return value presently with protocol 0 */
                            } else {
                                flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
                                outpc.status3 &= ~STATUS3_TESTMODE;
                            }
                        }
                    }
                }
                break;

            case MSG_REQX:  // extended
            case MSG_REQ:  // msg to send back current value of variable(s)
                /* Validate incoming addresses etc, then build return RSP message */
                if (msg_type == MSG_REQ) {
                    rem_table = CAN_RB_DSR0 & 0x1f;
                    rem_bytes = CAN_RB_DSR2 & 0x0f;
                    rem_offset = ((unsigned short)CAN_RB_DSR1 << 3) | ((CAN_RB_DSR2 & 0xe0) >> 5);
                } else {
                    rem_table = CAN_RB_DSR1;
                    rem_bytes = CAN_RB_DSR3 & 0x0f;
                    rem_offset = ((unsigned int)CAN_RB_DSR2 << 3) | ((CAN_RB_DSR3 & 0xe0) >> 5);
                    var_blk = CAN_RB_DSR4;
                }
                // put variable value(s) in xmit ring buffer
                dest_addr = 0;
                ad_tmp = 0;

                /* removed ability to read calibration tables - not being used */

                if ((var_blk > 3) && (var_blk < 16)) {
                    if (((var_blk != 7) && ((var_off + rem_bytes) > tables[var_blk].n_bytes))
                        || ((var_blk == 7) && (var_off < 0x200)  && ((var_off + rem_bytes) > sizeof(outpc)))
                        || ((var_blk == 7) && (var_off >= 0x200) && (var_off < 0x400) && ((var_off + rem_bytes) > (sizeof(datax1) + 0x200)))
                        || ((var_blk == 7) && (var_off >= 0x400) && ((var_off + rem_bytes) > 0x4ff))
                        ) {
                        flagbyte3 |= flagbyte3_can_reset;
                    } else {

                        if ((var_blk == 7) && (var_off >= 0x400)) {
                            if (var_off <= 0x4ff) {
                                dest_addr = 0xf700;
                                var_off -= 0x400;
                            }
                        } else if ((var_blk == 7) && (var_off >= 0x200)) {
                            dest_addr = (unsigned int)&datax1;
                            var_off -= 0x200;
                        } else {
                            dest_addr = (unsigned int)tables[var_blk].addrRam;
                        }
                    }
                } else if ((var_blk >= 0xf0) && (var_blk <= 0xf3)) {
                    dest_addr = (unsigned int)&ram_data; // will be...
                }
                chknewpage(var_blk);

                if (dest_addr == 0xf700) {
                    unsigned int n;
//                    n = conf_str_cp((unsigned char*)&tmp_data[0], rem_bytes, var_off);
                    n = 0; /* Config error not supported on trans */
                    n = rem_bytes - n;
                    for(kx = n;kx < rem_bytes;kx++)  {
                        tmp_data[kx] = 0x00; // pad with zeros
                    }
                    ad_tmp = (unsigned int)&tmp_data[0];

                } else if (dest_addr) {
                    ad_tmp = dest_addr + var_off;
                }

                if (ad_tmp) {
                    can_build_msg(MSG_RSP, rcv_id, rem_table, rem_offset, rem_bytes, (unsigned char*)ad_tmp, 0);
                }

                break;

//          case OUTMSG_REQ: // Not implemented
//          case OUTMSG_RSP: // Not implemented
//          case MSG_XSUB:   // Not implemented

            case MSG_BURN:  // msg to burn data table
                resetholdoff = 15;
                Flash_Init(); //check FDIV written to (should be at reset)
                flocker = 0xcc;
                burn_idx = var_blk;
                burntbl(0);
                outpc.status1 &= ~(STATUS1_LOSTDATA | STATUS1_NEEDBURN);
                burn = 1;
                break;
            case MSG_FWD:  // msg to forward data to the serial port
				// set up for serial xmit of the recvd CAN bytes (<= 7)
				// Unsolicited data
				if ((var_byt > 0) && (var_byt <= 7))  {
                    canbuf[16] = var_byt - 1; // serial.c will use this
					for (jx = 0;jx < canbuf[16];jx++)  {
						canbuf[jx] = *(&CAN_RB_DSR1 + jx);
					}
                    flagbyte14 |= FLAGBYTE14_SERIAL_FWD; // mainloop should send when safe
				}

                break;

            case MSG_CRC:  // msg to calc CRC32 of page - actually done in misc
                if ((var_blk == 4) || (var_blk == 5) || (var_blk == 8) || (var_blk == 9) || (var_blk == 10) || (var_blk == 11) || (var_blk == 12)) {
                    // jump to common code used by serial code
                    // if tuning remotely still need to copy flash to RAM etc.
                    chknewpage(var_blk);
                }
                crc_idx = var_blk;
                flagbyte6 |= FLAGBYTE6_CRC_CAN; // set up to do the biz from the mainloop
                // canbuf seems a fair place to store this data. Hoping nothing else tries to write it.
                canbuf[4] = CAN_RB_DSR1; // dest var blk
                canbuf[5] = rcv_id;
                break;

            case MSG_PROT:  // msg to send back current protocols
                rem_table = CAN_RB_DSR1;
                rem_bytes = CAN_RB_DSR3 & 0x0f;
                rem_offset = ((unsigned short)CAN_RB_DSR2 << 3) |
                        ((CAN_RB_DSR3 & 0xE0) >> 5);
                tmp_data[0] = SRLVER;
                if (rem_bytes == 5) {
                    *(unsigned int*)&tmp_data[1] = SRLDATASIZE;
                    *(unsigned int*)&tmp_data[3] = 1024; // size of ramdata
                }
                can_build_msg(MSG_RSP, rcv_id, rem_table, rem_offset, rem_bytes, (unsigned char*)tmp_data, 0);
 
                break;

            case MSG_SPND:
                if (CAN_RB_DSR1 == 1) {
                    flagbyte15 |= FLAGBYTE15_CANSUSP;
                } else {
                    flagbyte15 &= ~FLAGBYTE15_CANSUSP;
                }
                break;
            case MSG_BURNACK:
                flagbyte3 |= FLAGBYTE3_REMOTEBURNSEND;
                break;
            default:
                ;
        }					 // end msg_type switch
        can_status &= CLR_RCV_ERR;
    }
CAN_DONERXSW:;
    if ((CANRFLG & 0x72) != 0)  {
        // Rcv error or overrun on receive
        can_status |= RCV_ERR;
        //can_reset = 1;
    }
    /* clear RX buf full, err flags to ready for next rcv int */
    /*  (Note: can't clear err count bits) */
    CANRFLG = 0xC3;

    return;
}

