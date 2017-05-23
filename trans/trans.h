/* This header should be included by all other source files in the
 * ms2-extra project.
 */

/* $Id: trans.h,v 1.23 2015/03/11 15:23:39 jsm Exp $ */
#ifndef _MS2_EXTRA_H
#define _MS2_EXTRA_H

#include "hcs12def.h"      /* common defines and macros */
#include "flash.h"         /* flashburner defines, structures */
#include <string.h>

#include "trans_defines.h"

extern void _start(void);       /* Startup routine */

#define LOOKUP_ATTR __attribute__ ((section (".lookup")))
#define EEPROM_ATTR __attribute__ ((section (".eeprom")))
#define TEXT3_ATTR __attribute__ ((section (".text3")))
#define INTERRUPT
#define POST_INTERRUPT __attribute__((interrupt))
#define POST_INTERRUPT_TEXT3 __attribute__ ((section (".text3"))) __attribute__((interrupt))
#define FAR_TEXT3c_ATTR __attribute__ ((far)) __attribute__ ((section (".text3c")))
#define ENABLE_INTERRUPTS __asm__ __volatile__ ("cli");
#define DISABLE_INTERRUPTS __asm__ __volatile__ ("sei");
/* These next two require "unsigned char maskint_tmp, maskint_tmp2" in function */
#define MASK_INTERRUPTS __asm__ __volatile__ ("stab %0\n" "tfr ccr,b\n" "stab %1\n" "ldab %0\n" "sei\n" : :"m"(maskint_tmp),"m"(maskint_tmp2) ); /* Must be used with RESTORE */
#define RESTORE_INTERRUPTS __asm__ __volatile__ ("stab %0\n" "ldab %1\n" "tfr b,ccr\n" "ldab %0\n" : :"m"(maskint_tmp),"m"(maskint_tmp2) ); /* Must be used with MASK */
#define NEAR
#define VECT_ATTR __attribute__ ((section (".vectors")))

extern const int cltfactor_table[1024] LOOKUP_ATTR;
extern const int matfactor_table[1024] LOOKUP_ATTR;

INTERRUPT void UnimplementedISR(void) POST_INTERRUPT;
INTERRUPT void ISR_tach5(void) POST_INTERRUPT;
INTERRUPT void ISR_tach2(void) POST_INTERRUPT;
INTERRUPT void ISR_vss(void) POST_INTERRUPT;
INTERRUPT void ISR_TimerOverflow(void) POST_INTERRUPT;
INTERRUPT void ISR_Timer_Clock(void) POST_INTERRUPT;
INTERRUPT void ISR_SCI_Comm(void) POST_INTERRUPT;
INTERRUPT void CanTxIsr(void) POST_INTERRUPT;
INTERRUPT void CanRxIsr(void) POST_INTERRUPT;

typedef void (* NEAR tIsrFunc)(void);

int intrp_1dctable(int x, unsigned char n, int * x_table, char sgn,
		unsigned char * z_table);
int intrp_1dctableu(unsigned int x, unsigned char n, int * x_table, char sgn,
		unsigned char * z_table);
int intrp_1ditable(int x, unsigned char n, int *x_table, char sgn,
		unsigned int *z_table);
int intrp_2dctable(unsigned int x, int y, unsigned char nx, unsigned char ny,
		unsigned int * x_table, int * y_table, unsigned char * z_table,
		unsigned char hires);
int intrp_2dctable_signed(unsigned int x, int y, unsigned char nx,
		unsigned char ny, unsigned int * x_table, int * y_table,
		char * z_table);
int intrp_2ditable(unsigned int x, int y, unsigned char nx, unsigned char ny,
		unsigned int * x_table, int * y_table, int * z_table);

typedef struct {
   unsigned int *addrRam;
   unsigned int *addrFlash;
   unsigned int  n_bytes;
} tableDescriptor;

extern const tableDescriptor tables[NO_TBLES];
extern const unsigned int twopow[16];
extern const unsigned int builtingears[16][10];

// User inputs - 1 set in flash, 1 in ram
typedef struct {
    unsigned char divider, x1;
    unsigned int customgear[10]; /* 'gear' in ini file */
    unsigned int maxrpm[10];
    unsigned int wheeldia, fdratio, tps0, tpsmax, map0, mapmax;
    unsigned int lockuphyst;
    unsigned char vss_pos, lockrace;
    int lockupmintps, lockuptps, lockrace_tps;
    unsigned int  lockrace_delay;
    unsigned char brakesw, spare67;
    unsigned int tcc_opt;
#define BRAKESW_ON 1
#define BRAKESW_POL 2
    unsigned long vssout_scale;
    unsigned char manual, shiftret_table;
#define MANUAL_MASK 0xc0
#define MANUAL_AUTO 0x00
#define MANUAL_MAN  0x40
#define MANUAL_MANSW 0x80
    int rpm_shift_tps;
    int xxshiftretard;
    unsigned int shiftret_time, shiftret_off, vss1_can_scale;

    unsigned char spare[8];
    unsigned char inteeth, reluctorteeth;
    unsigned int xxlockupvss;
    unsigned char lockupstart, lockupfull, lockupend, setting3;
#define SETTING3_LUF 0x01
#define SETTING3_GEARCUSTOM 0x02
	unsigned int  lockupwait, lockupontime,lockupofftime; // 0.01s units
    unsigned char mycan_id, setting1, setting2, can_poll, can_poll_id;
#define SETTING1_CPUMASK 0x07
#define SETTING1_NOTSET 0x00
#define SETTING1_MS2 0x01
#define SETTING1_MICROSQUIRTV1 0x02
#define SETTING1_GPIO 0x03
#define SETTING1_MICROSQUIRTV3 0x04
#define SETTING1_LOADMAP 0x08
#define SETTING1_LINEP 0x10
#define SETTING1_EPCFREQ 0x20
#define SETTING1_RPM_SHIFT 0x40
#define SETTING1_RETARD 0x80
#define CANPOLL_DEGC 0x80

//test inputs
    unsigned char tst_set;
    unsigned int tst_lock;
    signed char tst_gear; 
    unsigned char tst_epc, tst_tcc, tst_sol32;
    unsigned char spare2[6];
    unsigned int gearv[9];
    unsigned int peak_time, refresh_period;
    unsigned char hold_duty, pad149;
    unsigned int shift_delay[2][8][5];
    unsigned int shift_pause;
    unsigned char paddle_en, paddle_in, paddle_out, vssout_opt;
    unsigned int paddle_upv, paddle_downv, paddle_outv;
    unsigned char od_out, od_mode;
#define OD_MODE_MOMENTARY 1
//#define OD_MODE_ENABLE 2
//#define OD_MODE_LIGHTENABLE 4
    unsigned char flashlock, spare325;
#define NUM_LINEP 6
    unsigned int linep[11][NUM_LINEP];
    int lineload[NUM_LINEP];
#define NUM_SHIFT 6
    unsigned int shiftvss[2][9][NUM_SHIFT]; /* up and down */
    int shiftload[9][NUM_SHIFT];
#define NUM_TCC_RPMVSS 10
    unsigned int tcc_rpm[NUM_TCC_RPMVSS], tcc_vss[NUM_TCC_RPMVSS];
#define NUM_SRETS 6
    int sret_retard[NUM_SRETS], sret_load[NUM_SRETS];

    unsigned char spares[178];
} page4_data;

extern const page4_data flash4 EEPROM_ATTR;

extern const char RevNum[SIZE_OF_REVNUM];
extern const char Signature[SIZE_OF_SIGNATURE];

// rs232 Outputs to pc
typedef struct {
    unsigned int seconds, rpm, vss1;
    int tps, map, transtemp,enginetemp;
    signed char selector, gear;
    unsigned int adc[8];
    unsigned char status1, status2, status3, status4;
    unsigned char epc,tcc;
    unsigned long status5,status6;
    unsigned char sol32, solstat;
    unsigned int inprpm, outrpm;
    unsigned char gearselin;
    char slip_conv, slip_trans;
    char spare;
    int shift_retard, load;
} variables;

typedef struct {
    unsigned int testmodelock; // must always be written as 12345 for valid command
    unsigned char testmodemode; // write a 0 to disable, 1 to enable, then 2 etc. for run modes
    unsigned char can_proto[15];
    int map_can, mat_can, clt_can, tps_can; /* These do not need to be shared with the outside world */
} datax;

typedef struct {
    unsigned long id;
    unsigned char sz;
    unsigned char action;
    unsigned char data[8];
} can_tx_msg;

// Prototypes - Note: ISRs prototyped above.
int main(void);

void main_init(void) TEXT3_ATTR;
void get_adc(char chan1, char chan2);
void CanInit(void) ;
void can_xsub01(void) ;
unsigned int can_sendburn(unsigned int, unsigned int);
unsigned int can_reqdata(unsigned int, unsigned int, unsigned int, unsigned char);
unsigned int can_snddata(unsigned int, unsigned int, unsigned int, unsigned char, unsigned int);
unsigned int can_crc32(unsigned int, unsigned int);
unsigned int can_sndMSG_PROT(unsigned int, unsigned char) FAR_TEXT3c_ATTR;
unsigned int can_sndMSG_SPND(unsigned int) FAR_TEXT3c_ATTR;
void can_do_tx(void);
unsigned int can_build_msg(unsigned char, unsigned char, unsigned char, unsigned int,
                 unsigned char, unsigned char*, unsigned char);
unsigned int can_build_msg_req(unsigned char, unsigned char, unsigned int, unsigned char,
                 unsigned int, unsigned char);
void can_poll(void);

void Flash_Init(void) TEXT3_ATTR;
void serial(void) FAR_TEXT3c_ATTR;
void debug_init() FAR_TEXT3c_ATTR;
void debug_str(unsigned char*) FAR_TEXT3c_ATTR;
void debug_byte(unsigned char) FAR_TEXT3c_ATTR;
void debug_bytehex(unsigned char) FAR_TEXT3c_ATTR;
void debug_bytedec(unsigned char) FAR_TEXT3c_ATTR;
void debug_byte2dec(unsigned char) FAR_TEXT3c_ATTR;
void debug_inthex(unsigned int) FAR_TEXT3c_ATTR;
void debug_longhex(unsigned long) FAR_TEXT3c_ATTR;
#define shift_lookup(gear,updown) intrp_1ditable(outpc.load, NUM_SHIFT, (int *)pg4_ptr->shiftload[gear], 0, (int *)pg4_ptr->shiftvss[updown][gear])
unsigned int shift_delay_calc(int, unsigned char, unsigned char)  TEXT3_ATTR;
unsigned int do_testmode(void) TEXT3_ATTR;
void chknewpage(unsigned char) FAR_TEXT3c_ATTR;
void chk_crc(void);
void calc_coeff(void) TEXT3_ATTR;
void ck_log_clr(void) TEXT3_ATTR;
void generic_pwm_outs(void) TEXT3_ATTR;
void do_lockup(void) FAR_TEXT3c_ATTR;
void od_button(void) FAR_TEXT3c_ATTR;
void calc_vss(void) FAR_TEXT3c_ATTR;

// for ASM routines the memory allocation is set in the source file
void ResetCmd(void) ;
void monitor(void) ;
void SpSub(void) ;
void NoOp(void) ;
void burntbl(unsigned int);
unsigned char g_read_copy(unsigned int, unsigned int, unsigned int);
unsigned long crc32buf(unsigned long, unsigned int, unsigned int) FAR_TEXT3c_ATTR;
void configerror(void) FAR_TEXT3c_ATTR;
unsigned int conf_str(void) FAR_TEXT3c_ATTR;
// end ASM

#include "trans_vars.h"
#endif
