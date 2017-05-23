/* $Id: trans_can.c,v 1.9 2015/01/19 22:32:06 jsm Exp $
 * Copyright 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013
 * James Murray and Kenneth Culver
 *
 * This file is a part of MS2/Extra.
 *
 * CanInit()
    Origin: Al Grippo.
    Minor: XEPisms by James Murray
    Majority: Al Grippo
 * can_sendburn()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_crc32()
    Origin: Code by James Murray using MS-CAN framework.
    Majority: James Murray
 * can_reqdata()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_snddata()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_sndMSG_PROT()
    Origin: James Murray
    Majority: James Murray
 * can_sndMSG_SPND()
    Origin: James Murray
    Majority: James Murray
 * send_can11bit()
    Origin: "stevevp" used with permission
    Majority: Complete re-write James Murray
 * send_can29bit()
    Origin: send_can11bit()
    Majority: Re-write to 29bit, James Murray
* can_do_tx()
    Origin: James Murray
    Majority: James Murray
* can_build_msg()
    Origin: James Murray
    Majority: James Murray
* can_build_msg_req()
    Origin: James Murray
    Majority: James Murray
* chk_crc
    Origin: James Murray
    Majority: James Murray

 *
 * You should have received a copy of the code LICENSE along with this source,
 * ask on the www.msextra.com forum if you did not.
 *
 */
#include "trans.h"

#define CAN_INC_TXRING \
    can_tx_num++; /* Increment count */\
    can_tx_in++; /* Next ring buffer slot */\
    if (can_tx_in >= NO_CANTXMSG) {\
        can_tx_in = 0;\
    }

void CanInit(void)
{
    unsigned int ix;
    /* Set up CAN communications */
    /* Enable CAN, set Init mode so can change registers */
    CANCTL1 |= 0x80;
    CANCTL0 |= 0x01;

    can_status = 0;
    flagbyte3 &= ~(flagbyte3_can_reset | flagbyte3_sndcandat | flagbyte3_getcandat);

    while(!(CANCTL1 & 0x01));  // make sure in init mode

    /* Set Can enable, use IPBusclk (24 MHz),clear rest */
    CANCTL1 = 0xC0;  
    /* Set timing for .5Mbits/ sec */
    CANBTR0 = 0xC2;  /* SJW=4,BR Prescaler= 3(24MHz CAN clk) */
    CANBTR1 = 0x1C;  /* Set time quanta: tseg2 =2,tseg1=13 
                        (16 Tq total including sync seg (=1)) */
    CANIDAC = 0x00;   /* 2 32-bit acceptance filters */
    /* CAN message format:
     * Reg Bits: 7 <-------------------- 0
     * IDR0:    |---var_off(11 bits)----|  (Header bits 28 <-- 21)
     * IDR1:    |cont'd 1 1 --msg type--|  (Header bits 20 <-- 15)
     * IDR2:    |---From ID--|--To ID---|  (Header bits 14 <--  7)
     * IDR3:    |--var_blk-|--spare--rtr|  (Header bits  6 <-- 0,rtr)
     */  
    /* Set identifier acceptance and mask registers to accept 
       messages only for can_id or device #15 (=> all devices) */
    /* 1st 32-bit filter bank-to mask filtering, set bit=1 */
    CANIDMR0 = 0xFF;           // anything ok in IDR0(var offset)
    CANIDAR1 = 0x18;           // 0,0,0,SRR=IDE=1
    CANIDMR1 = 0xE7;		   // anything ok for var_off cont'd, msgtype
    CANIDAR2 = CANid;     // rcv msg must be to can_id, but
    CANIDMR2 = 0xF0;			 // can be from any other device
    CANIDMR3 = 0xFF;           // any var_blk, spare, rtr
    /* 2nd 32-bit filter bank */
    CANIDMR4 = 0xFF;           // anything ok in IDR0(var offset)
    CANIDAR5 = 0x18;           // 0,0,0,SRR=IDE=1
    CANIDMR5 = 0xE7;		   // anything ok for var_off cont'd, msgtype
    CANIDAR6 = 0x0F;			 // rcv msg can be to everyone (id=15), and
    CANIDMR6 = 0xF0;			 // can be from any other device
    CANIDMR7 = 0xFF;           // any var_blk, spare, rtr

    /* clear init mode */
    CANCTL0 &= 0xFE;  
    /* wait for synch to bus */
    ix = 0;
    while ((!(CANCTL0 & 0x10)) && (ix < 0xfffe)) {
        ix++;
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
    if (ix == 0xfffe) { // Did not sync in time
        // CAN broken
        conf_err = 54;
    }

    CANTIER = 0; /* Do not enable TX interrupts */
    /* clear RX flag to ready for CAN recv interrupt */
    CANRFLG = 0xC3;
    CANRIER = 0x01; /* set CAN rcv full interrupt bit */

    can_tx_in = 0;
    can_tx_out = 0;
    can_tx_num = 0;
    return;
}

unsigned int can_sendburn(unsigned int table, unsigned int id)
{
    unsigned int ret;
    unsigned char tmp_data[8];

    /* Must be called with interrupts disabled */
    ret = can_build_msg(MSG_BURN, id, table, 0, 0, (unsigned char *)&tmp_data, 0);
    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    flagbyte3 |= FLAGBYTE3_REMOTEBURN;

    return ret;
}

unsigned int can_crc32(unsigned int table, unsigned int id)
{
    unsigned int ret;
    unsigned char tmp_data[8];
    /* Must be called with interrupts disabled */
    tmp_data[0] = MSG_CRC;
    tmp_data[1] = 6; /* Send back to table 6 */
    tmp_data[2] = 0; /* Send back to offset 0 at this time */
    tmp_data[3] = 4; /* Always 4 bytes */

    ret = can_build_msg(MSG_XTND, id, table, 0, 4, (unsigned char *)&tmp_data, 0);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_reqdata(unsigned int table, unsigned int id, unsigned int offset, unsigned char num)
{
    /* Must be called with interrupts disabled */
    /* Create the first read in a series of passthrough requests */
    unsigned int ret;

    ret = can_build_msg_req(id, table, offset, 6, 0, num);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_snddata(unsigned int table, unsigned int id, unsigned int offset, unsigned char num, unsigned int bufad)
{
    unsigned int ret;
    /* Must be called with interrupts disabled */
    /* Send data */
    /* called from serial.c and can.c  */

    ret = can_build_msg(MSG_CMD, id, table, offset, num, (unsigned char *)bufad, 1); /* passthrough action */

    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_sndMSG_PROT(unsigned int id, unsigned char sz)
{
    unsigned int ret;

    unsigned char tmp_data[8];

    tmp_data[0] = MSG_PROT;
    tmp_data[1] = 6; /* Send back to table 6 */
    tmp_data[2] = 0; /* Send back to offset 0 at this time */
    tmp_data[3] = sz; /* # bytes - either 1 (just protocol no.) or 5 (block sizes as well) */

    ret = can_build_msg(MSG_XTND, id, 0, 0, 4, (unsigned char *)&tmp_data, 0);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_sndMSG_SPND(unsigned int onoff)
{
    /* Send a message to CANid=15. This may not be supported on receiving end. */
    unsigned int ret;
    unsigned char tmp_data[8];

    tmp_data[0] = onoff;
    ret = can_build_msg(MSG_SPND, 15, 0, 0, 1, (unsigned char *)&tmp_data, 0);

    return ret;
}

void send_can11bit(unsigned int id, unsigned char *data, unsigned char bytes) {
    can_tx_msg *this_msg;
    unsigned char maskint_tmp, maskint_tmp2;

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
        this_msg = &can_tx_buf[can_tx_in];
        /* Store 11bit ID into Freescale raw format */
        this_msg->id = (unsigned long)id << 21;

        /* Store data and length */
        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&data[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&data[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&data[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&data[6];
        this_msg->sz = bytes;
        CAN_INC_TXRING;
    } else {
        /* fail silently */
    } 
    RESTORE_INTERRUPTS;
}

void send_can29bit(unsigned long id, unsigned char *data, unsigned char bytes) {
    /* Sends 29bit CAN message header and chosen data, bypassing Al-CAN
        Beware of sending messages that correspond to Al-CAN messages
        as there is no error trapping.
    */
    unsigned long a, b;
    unsigned int l;
    can_tx_msg *this_msg;
    unsigned char maskint_tmp, maskint_tmp2;

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
        l = bytes; /* To workaround ICE. */
        this_msg = &can_tx_buf[can_tx_in];

        /* Store 29bit ID into Freescale raw format */
        a = (id << 1) & 0x0007ffff;
        b = (id << 3) & 0xffe00000;
        this_msg->id = a | b | 0x00180000;

        /* Store data and length */

        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&data[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&data[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&data[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&data[6];
        this_msg->sz = l;

        CAN_INC_TXRING;
    } else {
        /* fail silently */
    }
    RESTORE_INTERRUPTS;
}

void can_poll(void)
{
    /***************************************************************************
    **
    **  CAN comms
    **
    **************************************************************************/
#define CAN_FETCH_INTERVAL 78 /* 10ms */
#define CAN_SEND_INTERVAL 781 /* 100ms */
    unsigned int off, dest;
    unsigned char tmp_data[8];

    /* Fetching data */
    if ( (flash4.can_poll & 0x03) 
        && (((unsigned int)lmms - cansendclk) > CAN_FETCH_INTERVAL)
        && (!(flagbyte3 & (flagbyte3_getcandat | flagbyte3_sndcandat))) && (txmode == 0) )  {
        // only if enabled and NOT presently transmitting. The last clause added
        // because pass-through data was clashing with data fetches
        cansendclk = (unsigned int)lmms;

        /* get the rpm transferred to our memory */
        off = (unsigned int)&outpc.rpm - (unsigned int)&outpc; // this is offset of where to store it
        dest = 6; // fetch from RPM at known offset of 6 in MS2 + MS3
        (void)can_build_msg_req(pg4_ptr->can_poll_id, 7, dest, 7, off, 2);

        /* get the clt+tps transferred to our memory - pita because different type*/
        off = (unsigned int)&datax1.map_can - (unsigned int)&datax1 + DATAX1_OFFSET; // this is offset of where to store it
        dest = 18; // fetch from MAP,MAT,CLT,TPS
        (void)can_build_msg_req(pg4_ptr->can_poll_id, 7, dest, 7, off, 8);
    }

    /* Sending data */
    if ( (flash4.can_poll & 0x03) && (flash4.setting1 & SETTING1_RETARD)
        && ((((unsigned int)lmms - cansendclk2) > CAN_SEND_INTERVAL) || (flagbyte0 & FLAGBYTE0_SHIFTRETTX))
        && (!(flagbyte3 & (flagbyte3_getcandat | flagbyte3_sndcandat))) && (txmode == 0) )  {
        /* Change in shift retard forces an immediate TX */
        cansendclk2 = (unsigned int)lmms;
        flagbyte0 &= ~FLAGBYTE0_SHIFTRETTX;

        if (flagbyte0 & FLAGBYTE0_SHIFTRET) {
            *(unsigned int*)&tmp_data[0] = -outpc.shift_retard;
        } else {
            *(unsigned int*)&tmp_data[0] = 0;
        }

        (void)can_build_msg(MSG_CMD, flash4.can_poll_id, flash4.shiftret_table, flash4.shiftret_off, 2,
            (unsigned char *)tmp_data, 0);
    }
}

/* Removed bcast_outpc support */

void can_do_tx()
{
    /* Take a message from our queue and put it into the CPU TX buffer if space */
    /* Interrupts MUST be masked by calling code */
    unsigned long id;
    unsigned char action;
    unsigned char *d;
    can_tx_msg *this_msg;

    this_msg = &can_tx_buf[can_tx_out]; /* grab a pointer */
    id = this_msg->id;

    if (can_tx_num) {
        can_tx_num--; /* Declare that we sent a packet */
    }
    can_tx_out++; /* Ring buffer pointer */
    if (can_tx_out >= NO_CANTXMSG) {
        can_tx_out = 0;
    }

    if (id) {
        CANTBSEL = CANTFLG; /* Select the free TX buffer */
        /* Tx buffer holds raw Freescale format ID registers */
        *(unsigned long*)&CAN_TB0_IDR0 = id;
        this_msg->id = 0; /* Mark as zero to say we've handled it */
        CAN_TB0_DLR = this_msg->sz;
        action = this_msg->action;

        d = (unsigned char*)&CAN_TB0_DSR0;
        *(unsigned int*)&d[0] = *(unsigned int*)&this_msg->data[0];
        *(unsigned int*)&d[2] = *(unsigned int*)&this_msg->data[2];
        *(unsigned int*)&d[4] = *(unsigned int*)&this_msg->data[4];
        *(unsigned int*)&d[6] = *(unsigned int*)&this_msg->data[6];

        CAN_TB0_TBPR = 0x02; /* Arbitrary message priority */

        can_status &= CLR_XMT_ERR;

        CANTFLG = CANTBSEL; /* Send it off */

        if (action == 1) { /* check for any passthrough write messages */
            if ((flagbyte3 & flagbyte3_sndcandat) && cp_targ) {
                int num;
                unsigned int r;

                num = cp_targ - cp_cnt;
                if (num > 8) {
                    num = 8;
                } else {
                    /* final block, prevent sending any more*/
                    flagbyte14 |= FLAGBYTE14_SERIAL_OK; // serial code to return ack code that all bytes were stuck into pipe
                    flagbyte3 &= ~flagbyte3_sndcandat;
                    cp_targ = 0;
                }

                r = can_snddata(cp_table, cp_id, cp_offset + cp_cnt, num, (unsigned int)&srlbuf[9] + cp_cnt);

                if (r) {
                    /* flag up the error so serial.c can act*/
                    flagbyte14 |= FLAGBYTE14_CP_ERR;
                }
                cp_cnt += num;
            }
        }
        /* Removed OUTMSG support */
    }
}

unsigned int can_build_msg(unsigned char msg, unsigned char to, unsigned char tab, unsigned int off,
                             unsigned char by, unsigned char *dat, unsigned char action) {
    /* Build a 29bit CAN message in processor native format */
    unsigned long id;
    can_tx_msg *this_msg;
    unsigned int ret;
    unsigned char maskint_tmp, maskint_tmp2;

#define CALC_ID_ASM

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
#ifdef CALC_ID_ASM
        unsigned int idh, idl;
        __asm__ __volatile__ (
            "ldd %1\n"
            "ldy #32\n"
            "emul\n"
            "orab   #0x18\n"
            "orab   %2\n"
            :"=d"(idh)
            :"m"(off), "m"(msg)
            :"y","x"
        );

        __asm__ __volatile__ (
            "ldaa %1\n"
            "lsla\n"
            "lsla\n"
            "lsla\n"
            "lsla\n"
            "oraa   %2\n"
            "ldab   %3\n"
            "lslb\n"
            "lslb\n"
            "lslb\n"
            "lslb\n"
            "bcc    no_bit4\n"
            "orab   #0x08\n"
            "no_bit4:"
            :"=d"(idl)
            :"m"(flash4.mycan_id), "m"(to), "m"(tab)
            :"y","x"
        );

        id = ((unsigned long)idh << 16) | idl;
#else
        /* This C implementation looks cleaner but takes 11us instead of 1us */
        id = 0;
        id |= (unsigned long)off << 21;
        id |= (unsigned long)msg << 16;
        id |= (unsigned long)flash4.mycan_id << 12;
        id |= (unsigned long)to << 8;
        id |= (tab & 0x0f) << 4;
        id |= (tab & 0x10) >> 1;
        id |= 0x180000; /* SRR and IDE bits */
#endif
        this_msg = &can_tx_buf[can_tx_in]; /* grab a pointer */
        this_msg->id = id;
        this_msg->sz = by;
        this_msg->action = action;
        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&dat[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&dat[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&dat[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&dat[6];
        CAN_INC_TXRING;
        ret = 0;
    } else {
        ret = 1;
    }
    RESTORE_INTERRUPTS;
    return ret;
}

unsigned int can_build_msg_req(unsigned char to, unsigned char tab, unsigned int off,
                     unsigned char rem_table,
                     unsigned int rem_off, unsigned char rem_by) {
    /* Build a MSQ_REQ message */
    /* Must be called with interrupts off */
    unsigned char tmp_data[8];
    unsigned int ret;

    if (tab < 32) {
        tmp_data[0] = rem_table; /* Send back to table x */
        tmp_data[1] = rem_off >> 3; /* offset */
        tmp_data[2] = ((rem_off & 0x07) << 5) | rem_by; /* offset, # bytes */
        ret = can_build_msg(MSG_REQ, to, tab, off, 3, (unsigned char *)&tmp_data, 0);
    } else {
        tmp_data[0] = MSG_REQX;
        tmp_data[1] = rem_table; /* Send back to table x */
        tmp_data[2] = rem_off >> 3; /* offset */
        tmp_data[3] = ((rem_off & 0x07) << 5) | rem_by; /* offset, # bytes */
        tmp_data[4] = tab; /* Large table number */
        ret = can_build_msg(MSG_XTND, to, tab, off, 5, (unsigned char *)&tmp_data, 0);
    }
    return ret;
}

void chk_crc(void)
{
    unsigned long crc = 0;
    unsigned char tmp_data[8];
    /* if required, calc crc of ram page */
    /* local CRC handled in serial.c now */
    if (flagbyte6 & FLAGBYTE6_CRC_CAN) {
        flagbyte6 &= ~FLAGBYTE6_CRC_CAN;
// only check ram copy, irrespective of page number

        if (crc_idx < 4) {
            unsigned char save_PPAGE;
            /* check CRC of flash page */
            save_PPAGE = PPAGE;
            PPAGE = 0x3c;
            crc = crc32buf(0, (unsigned int)tables[crc_idx].addrFlash, tables[crc_idx].n_bytes);    // incrc, buf, size
            PPAGE = save_PPAGE;
        } else {
            crc = crc32buf(0, (unsigned int)&ram_data, 0x400);    // incrc, buf, size
        }
        *(unsigned long*)&tmp_data[0] = crc;

        can_build_msg(MSG_RSP, canbuf[5], canbuf[4], 0, 4, (unsigned char*)tmp_data, 0);
    }
}

/* Removed OUTMSG support */
