/* $Id: trans_vars.h,v 1.18 2015/01/11 16:01:11 jsm Exp $ */
/* variable definitions */

extern char ram_data[1024];
extern variables outpc;
extern unsigned char canbuf[17];
#define SRLDATASIZE 256
#define SRLHDRSIZE 13
extern char srlbuf[SRLDATASIZE + SRLHDRSIZE]; // 3 byte header + command + 256 + CRC32
#define SRLVER 1
extern datax datax1;
extern unsigned int db_ptr;

#define SIZE_OF_REVNUM 20
#define SIZE_OF_SIGNATURE 55

extern page4_data *pg4_ptr;

// General variables
extern unsigned int TC_ovflow;
extern unsigned long lmms,t_enable_IC, Rpm_Coeff, vss1_coeff, inpRpm_Coeff, outRpm_Coeff, ltch_lmms,rcv_timeout,adc_lmms;

extern unsigned char next_adc,first_adc,txmode,tble_idx,burn_idx,crc_idx,mms,millisec,tenthsec,flocker, burn;
extern unsigned int txcnt,txgoal,rxoffset,rxnbytes;
extern unsigned int srl_timeout;
extern unsigned char rxmode, srl_err_cnt;

// CAN variables
extern unsigned int cansendclk, cansendclk2;
extern unsigned int can_status, can_txtime;
extern unsigned int canrxad, canrxgoal;
extern unsigned int cp_targ, cp_cnt, cp_offset, cp_time;
extern unsigned char cp_id, cp_table, can_scanid;
extern unsigned int can_bcast_last;
extern unsigned char can_err_cnt[16], can_getid;
extern unsigned int can_outpc_bcast_ptr;
extern unsigned int can_boc_tim_all;
extern volatile unsigned char can_rcv_in, can_rcv_proc, can_tx_in, can_tx_out, can_tx_num;
extern can_tx_msg can_tx_buf[NO_CANTXMSG]; /* Transmit buffer */
#define CAN_DEAD_THRESH 5

// allocate space in ram for flash burner core
extern volatile unsigned char RamBurnPgm[36];

// vars added for MS2/Extra
extern unsigned char page;  // which flash data page is presently in ram
extern unsigned char conf_err; // set if configuration error

extern unsigned char CANid,ibuf, next_txmode;
extern unsigned char flagbyte0;
#define flagbyte0_vss      1   // use bits of this byte
#define flagbyte0_tach5    2
#define flagbyte0_tach2    4
#define FLAGBYTE0_SAMPLE_VSS1     8
#define FLAGBYTE0_SHIFTRET  0x10
#define FLAGBYTE0_SHIFTRETTX 0x20

extern unsigned char flagbyte1;
#define flagbyte1_tstmode  2   // test mode enable - set to 1 turns off normal functions
#define flagbyte1_ovfclose 4   // nearly going to overflow

extern unsigned char flagbyte3;
#define FLAGBYTE3_REMOTEBURN 0x01
#define FLAGBYTE3_REMOTEBURNSEND 0x02
#define flagbyte3_can_reset 4    // from MS2 2.87
#define flagbyte3_getcandat 8    // from MS2 2.87
#define flagbyte3_sndcandat 0x10 // from MS2 2.87
#define flagbyte3_kill_srl  0x20    // from MS2 2.87

extern unsigned char flagbyte6;
#define FLAGBYTE6_CRC           0x08
#define FLAGBYTE6_CRC_CAN       0x10

extern unsigned char flagbyte14;
//#define FLAGBYTE14_SERIAL_TL      0x01
#define FLAGBYTE14_SERIAL_FWD     0x02

#define FLAGBYTE14_CP_ERR         0x08
#define FLAGBYTE14_SERIAL_PROCESS 0x10
#define FLAGBYTE14_SERIAL_SEND    0x20
//#define FLAGBYTE14_SERIAL_BURN    0x40
#define FLAGBYTE14_SERIAL_OK      0x80

extern unsigned char flagbyte15;
//#define FLAGBYTE15_DPCNT        0x01
#define FLAGBYTE15_CANSUSP      0x02
#define FLAGBYTE15_CANRX        0x04
#define FLAGBYTE15_DB_WRAP      0x40 // debug buffer has wrapped
#define FLAGBYTE15_OD_ENABLE    0x80 /* OD button is enable vs. cancel */

#define STATUS1_NEEDBURN    1   // need burn
#define STATUS1_LOSTDATA    2  // lost data
#define status1_conferr     4    // config error
#define STATUS1_BRAKE 0x10
#define STATUS1_RACE  0x20
#define STATUS1_LOCKUP 0x40
#define STATUS1_ODCANCEL 0x80

#define STATUS3_TESTMODE 8 // only for indication

extern unsigned char adc_ctr;
extern unsigned int lowres, lowres_ctr; // 0.128ms period counters (like MS1) for tacho

extern unsigned int swtimer, vss1_stall, stall_ess, stall_timeout_vss, stall_timeout_ess, vssout_match, vssout_cnt;
#define VSS_STALL_TIMEOUT   7912 /* May need fine tuning */
#define VSS_TIME_THRESH 30000 /* 20ms sample interval */
extern unsigned char bl_timer, resetholdoff, lockupcount, lockupmode, burn;
extern unsigned int last_rpm;
extern unsigned long vss,ess;
extern volatile unsigned char *port_sol[6], *port_brake, *port_paddle_en, *port_paddle_out, *port_vssout, *port_gearsw[4],
     *port_manual, *port_odout;
extern unsigned char pin_sol[6], pin_brake, pin_brake_match, pin_paddle_en, pin_paddle_en_match, pin_paddle_out,
    pin_vssout, pin_gearsw[4], pin_gearsw_match[4], pin_manual, pin_odout;
extern unsigned char *pwm_sol[6];
extern unsigned short *port_gearsel, *port_paddle_in, *port_enginetemp, *port_tps, *port_transtemp;

#define NUM_SWPWM 2
extern volatile unsigned char *port_swpwm[NUM_SWPWM];
extern unsigned char pin_swpwm[NUM_SWPWM];
extern unsigned int gp_clk[NUM_SWPWM], gp_max_on[NUM_SWPWM], gp_max_off[NUM_SWPWM];
extern unsigned char gp_stat[NUM_SWPWM], sw_pwm_num, sw_pwm_freq[NUM_SWPWM];
extern unsigned char *sw_pwm_duty[NUM_SWPWM], tcc_pwm, sol32_pwm;

extern unsigned char last_gear, gear_41te, shift_phase, max_gear, paddle_phase;
extern unsigned int timer_41te, shift_timer, paddle_timer, vss_timer, shiftret_timer;
extern unsigned char dummyReg;
extern unsigned long vss1_time_sum;
extern unsigned int shift_delay, internal_vss, vss1_teeth;

extern unsigned char testmode_glob, testmode_gear, trans, gearsel, outputmode;
#define TRANS_4L80E 0
#define TRANS_4L60E 1
#define TRANS_A341E 2
#define TRANS_41TE  3
#define TRANS_4R70WA 4 /* analog gear pos */
#define TRANS_4R70WS 5 /* switch gear pos */
#define TRANS_4T40E 6
#define TRANS_5L40E 7
#define TRANS_E4ODA 8 /* analog gear pos */
#define TRANS_E4ODS 9 /* switch gear pos */
#define TRANS_W4A33 10 /* analog gear pos */

/* output mode defines must align with trans types */
#define OUTPUTMODE_4L80E 0
#define OUTPUTMODE_4L60E 1
#define OUTPUTMODE_A340E 2
#define OUTPUTMODE_41TE  3
/* 4 not used at this time */
/* 5 not used at this time */
/* 6 not used at this time */
#define OUTPUTMODE_5L40E 7
#define OUTPUTMODE_E4OD 8
/* 9 not used at this time */
#define OUTPUTMODE_W4A33 10

#define NO_TBLES        16 /* locally held */
#define NO_CANTBLES     32 /* protocol spec */
#define SER_TOUT  5
#define MSG_CMD 0
#define MSG_REQ 1
#define MSG_RSP 2
#define MSG_XSUB        3
#define MSG_BURN        4
#define OUTMSG_REQ  5
#define OUTMSG_RSP  6
#define MSG_XTND    7
// define xtended msgs from 8 on
#define MSG_FWD     8
#define MSG_CRC     9
#define MSG_REQX 12 /* like REQ but for tables > 31 */
#define MSG_BURNACK 14 /* ACK from remote that a Burn completed */
#define MSG_PROT 0x80 /* new extended CAN command for getting the protocol version number */
#define MSG_WCR 0x81 /* new extended CAN command for sending the CRC32 after a series of CMD messages */
#define MSG_SPND 0x82 /* new extended CAN command for suspending and resuming CAN polling to a device */

#define	NO_VAR_BLKS	16
#define MAX_CANBOARDS 16
//#define NO_CANRCVMSG 16
#define NO_CANTXMSG 8 /* May need tweaking */
#define NO_CANTXMSG_POLLMAX 6 /* Maximum slots to be used for polling / broadcast */
#define CANTFLG_MASK 1 /* Use a single TX buffer only, otherwise hardware re-arranges packet order */
// Error status words:
//    -bits 0-7 are current errors
//    -bits 8-15 are corresponding latched errors
#define	XMT_ERR			0x0101
#define	CLR_XMT_ERR		0xFFFE
#define	XMT_TOUT		0x0202
#define	CLR_XMT_TOUT	0xFFFD
#define	RCV_ERR			0x0404
#define	CLR_RCV_ERR		0xFFFB
#define	SYS_ERR		0x0808

#define DATAX1_OFFSET 512 /* This is the fake offset within outpc where datax1 is used */

 #define MONVER (*(unsigned int *) 0xFEFE)
extern unsigned char od_state, gearsel_count;
extern unsigned int od_timer;
signed char old_selector;
unsigned int gear_ratio[10];
