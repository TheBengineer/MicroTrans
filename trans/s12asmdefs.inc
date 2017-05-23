;gcc port (C) 2006 James Murray
; $Id: s12asmdefs.inc,v 1.2 2014/12/20 19:59:53 jsm Exp $

;until someone can find a simple way to share definitions between asm and C we have to
;define everything twice.. yuk

;HCS12
;****************************************************************************************
;* Title: HCS12_Serial_Monitor.inc                            copyright (c) Motorola 2003
;****************************************************************************************
;* Author:  Rat579
;*
;* Description:  register and bit name definitions for HCS12_Serial_Monitor.asm
;*
;* Documentation:
;*
;* Include files:  none
;*
;* Assembler:
;*
;* Revision History:
;* Rev #      Date       Who     Comments
;* ------  -----------   ------  -----------------------------------------------------
;* 1.0      08-Aug-03    JPW     initial release
;* 1.1      03-Dec-03    JPW     Added MC9S12NE64 Support
;*
;****************************************************************************************
;****************************************************************************************
;*
;*   Motorola reserves the right to make changes without further notice to any product
;*   herein to improve reliability, function, or design.  Motorola does not assume any
;*   liability arising out of the application or use of any product, circuit, or software
;*   described herein; neither does it convey any license under its patent rights nor the
;*   rights of others.  Motorola products are not designed, intended, or authorized for
;*   use as components in systems intended for surgical implant into the body, or other
;*   applications intended to support life, or for any other application in which the
;*   failure of the Motorola product could create a situation where personal injury or
;*   death may occur.  Should buyer purchase or use Motorola products for any such
;*   intended or unauthorized application, buyer shall indemnify and hold Motorola and
;*   its officers, employees, subsidiaries, affiliates, and distributors harmless against
;*   all claims, costs, damages, and expenses, and reasonable attorney fees arising out
;*   of, directly or indirectly, any claim of personal injury or death associated with
;*   such unintended or unauthorized use, even if such claim alleges that motorola was
;*   negligent regarding the design or manufacture of the part.
;*
;*   Motorola and the Motorola logo are registered trademarks of Motorola Ltd.
;****************************************************************************************

.equ CORE1,      0x0000         ;ports A, B, E, modes, inits, test
.equ DEVID,      0x001A         ;device ID register
.equ CORE2,      0x001C         ;memsiz, IRQ, HPRIO
.equ CORE3,      0x0020         ;debug register
.equ CORE4,      0x0030         ;PPAGE, port K
.equ CRGV4,      0x0034         ;clock and reset generator
.equ SCI0V3,     0x00C8         ;serial communications interface SCI0
.equ FTS,        0x0100         ;Flash control registers
.equ EE,         0x0110		  ;EEPROM control registers
.equ PIM,        0x0240         ;port integration module

;****  Input/Output (I/O) Ports  *******************************************************
;*
;****  Input/Output (I/O) Ports  *******************************************************
;*
.equ PORTA,      CORE1+0x00     ;I/O port A 8-bit data register
;
.equ PTA7,       0b10000000     ;port A bit 7
.equ PTA6,       0b01000000     ;port A bit 6
.equ PTA5,       0b00100000     ;port A bit 5
.equ PTA4,       0b00010000     ;port A bit 4
.equ PTA3,       0b00001000     ;port A bit 3
.equ PTA2,       0b00000100     ;port A bit 2
.equ PTA1,       0b00000010     ;port A bit 1
.equ PTA0,       0b00000001     ;port A bit 0

.equ PORTB,      CORE1+0x01	    ;I/O port B 8-bit data register
;
.equ PTB7,       0b10000000     ;port B bit 7
.equ PTB6,       0b01000000     ;port B bit 6
.equ PTB5,       0b00100000     ;port B bit 5
.equ PTB4,       0b00010000     ;port B bit 4
.equ PTB3,       0b00001000     ;port B bit 3
.equ PTB2,       0b00000100     ;port B bit 2
.equ PTB1,       0b00000010     ;port B bit 1
.equ PTB0,       0b00000001     ;port B bit 0

.equ DDRA,       CORE1+0x02     ;I/O port A 8-bit data direction register
;
.equ DDRA7,      0b10000000     ;port A bit 7
.equ DDRA6,      0b01000000     ;port A bit 6
.equ DDRA5,      0b00100000     ;port A bit 5
.equ DDRA4,      0b00010000     ;port A bit 4
.equ DDRA3,      0b00001000     ;port A bit 3
.equ DDRA2,      0b00000100     ;port A bit 2
.equ DDRA1,      0b00000010     ;port A bit 1
.equ DDRA0,      0b00000001     ;port A bit 0

.equ DDRB,       CORE1+0x03	  ;I/O port B 8-bit data direction register
;
.equ DDRB7, 	    0b10000000     ;port B bit 7
.equ DDRB6,      0b01000000     ;port B bit 6
.equ DDRB5,      0b00100000     ;port B bit 5
.equ DDRB4,      0b00010000     ;port B bit 4
.equ DDRB3,      0b00001000     ;port B bit 3
.equ DDRB2,      0b00000100     ;port B bit 2
.equ DDRB1,      0b00000010     ;port B bit 1
.equ DDRB0,      0b00000001     ;port B bit 0

; 0x04 to 0x07 reserved

.equ PORTE,      CORE1+0x08	  ;I/O port E 8-bit data register
;
.equ PTE7,       0b10000000     ;port E bit 7
.equ PTE6,       0b01000000     ;port E bit 6
.equ PTE5,       0b00100000     ;port E bit 5
.equ PTE4,       0b00010000     ;port E bit 4
.equ PTE3,       0b00001000     ;port E bit 3
.equ PTE2,       0b00000100     ;port E bit 2
.equ PTE1,       0b00000010     ;port E bit 1
.equ PTE0,       0b00000001     ;port E bit 0

.equ DDRE,       CORE1+0x09	  ;I/O port E 8-bit data direction register
;
.equ DDRE7,      0b10000000     ;port E bit 7
.equ DDRE6,      0b01000000     ;port E bit 6
.equ DDRE5,      0b00100000     ;port E bit 5
.equ DDRE4,      0b00010000     ;port E bit 4
.equ DDRE3,      0b00001000     ;port E bit 3
.equ DDRE2,      0b00000100     ;port E bit 2
.equ DDRE1,      0b00000010     ;port E bit 1
.equ DDRE0,      0b00000001     ;port E bit 0

.equ PUCR,       CORE1+0x0C     ;pullup control register
;
.equ PUPKE,      0b10000000     ;pullup port K enable
.equ PUPEE, 	    0b00010000     ;pullup port E enable
.equ PUPBE,	    0b00000010     ;pullup port B enable
.equ PUPAE,      0b00000001     ;pullup port A enable

.equ INITRM,     CORE1+0x10     ;initialization of internal RAM position register
.equ INITRG,     CORE1+0x11     ;initialization of internal registers position register
.equ INITEE,     CORE1+0x12     ;initialization of internal EEPROM registers position register

;****  Device ID Registers (PARTID)  ************************************************
;*
.equ PARTIDH,    DEVID+0x00     ;device ID high register
.equ PARTIDL,    DEVID+0x01     ;device ID low register

;****  CORE2 (MEMSIZ, IRQ, HPRIO)  ************************************************
;*
.equ MEMSIZ0,    CORE2+0x00     ;memory size register 0
.equ eep_sw1,    0b00100000     ;allocated system EEPROM memory space
.equ eep_sw0,    0b00010000     ;allocated system EEPROM memory space

;****  Debug Module    (DBG)  ***********************************************
;*
.equ DBGC1,      CORE3+0x00     ;debug control register 1
.equ DBGEN,      0b10000000     ;DBG mode enable bit
.equ ARM,        0b01000000     ;arm bit

.equ DBGSC,      CORE3+0x01     ;debug control and status register

.equ DBGTBH,     CORE3+0x02     ;debug trace buffer high register

.equ DBGTBL,     CORE3+0x03     ;debug trace buffer low register

.equ DBGCNT,     CORE3+0x04     ;debug count register

.equ DBGCCX,     CORE3+0x05     ;debug comparator C expansion register

.equ DBGCCH,     CORE3+0x06     ;debug comparator C high register

.equ DBGCCL,     CORE3+0x07     ;debug comparator C low register

.equ DBGC2,      CORE3+0x08     ;debug control register 2

.equ DBGC3,      CORE3+0x09     ;debug control register 3

.equ DBGACX,     CORE3+0x0A     ;debug comparator A expansion register
; ??? Not pre lastest specification
.equ DBGACH,     CORE3+0x0B     ;debug comparator A high register

.equ DBGACL,     CORE3+0x0C     ;debug comparator A low register

.equ DBGBCX,     CORE3+0x0D     ;debug comparator B expansion register

.equ DBGBCH,     CORE3+0x0E     ;debug comparator B high register

.equ DBGBCL,     CORE3+0x0F     ;debug comparator B low register

;****  CORE4 (PPAGE, Port K)  ************************************************
;*
.equ PPAGE,      CORE4+0x00     ;program page index register
;
;****  Clock and Reset Generator (PLL, RTI, COP)  ************************************************
;*
.equ SYNR,       CRGV4+0x00     ;CRG synthesizer register

.equ REFDV,      CRGV4+0x01     ;CRG reference divider register

.equ CRGFLG,     CRGV4+0x03     ;CRG flags register
.equ LOCK,       0b00001000     ;lock status bit

.equ CLKSEL,     CRGV4+0x05     ;CRG clock select register
.equ PLLSEL,     0b10000000     ;PLL select bit

.equ PLLCTL,     CRGV4+0x06     ;CRG PLL control register
.equ PLLON,      0b01000000     ;phase lock loop on bit

.equ COPCTL,     CRGV4+0x08     ;CRG COP control register
.equ RSBCK,      0b01000000     ;COP and RTI stop in active BDM mode bit
.equ CR2,        0b00000100     ;COP watchdog timer rate select
.equ CR1,        0b00000010     ;COP watchdog timer rate select
.equ CR0,        0b00000001     ;COP watchdog timer rate select
.equ ARMCOP,     CRGV4+0x0B     ;CRG COP arm/timer reset register


;****  Serial Communication Interface 0 (SCI0)  ****************************************
;*
.equ SCI0BDH,    SCI0V3+0x00    ;SCI baud rate registers

.equ SCI0BDL,    SCI0V3+0x01    ;SCI baud rate registers

.equ SCI0CR1,    SCI0V3+0x02    ;SCI control register 1
.equ WAKE,       0b00001000     ;wakeup condition bit
.equ ILT,        0b00000100     ;idle line type bit
.equ RSRC,       0b00100000     ;rec source bit

.equ SCI0CR2,    SCI0V3+0x03    ;SCI control register 2
.equ RIE,        0b00100000     ;receiver full interrupt enable bit
.equ SBK,        0b00000001     ;send break bit

.equ SCI0SR1,    SCI0V3+0x04    ;SCI status register 1
.equ TDRE,       0b10000000     ;transmit data register empty flag
.equ RDRF,       0b00100000     ;receive data register full flag

.equ SCI0DRL,    SCI0V3+0x07    ;SCI data register

;****  Flash Control Register  *********************************************************
;*
.equ FCLKDIV,    FTS+0x00 ;flash clock divider register

.equ FSEC,       FTS+0x01 ;flash security register

.equ FCNFG,      FTS+0x03 ;flash configuration register
.equ CBEIE,      0b10000000     ;command buffer empty interrupt enable
.equ CCIE,       0b01000000     ;command complete interrupt enable

.equ FSTAT,      FTS+0x05 ;flash status register
;
.equ CBEIF,      0b10000000     ;command buffer empty interrupt flag
.equ CCIF,       0b01000000     ;command complete interrupt flag
.equ PVIOL,      0b00100000     ;protection violation
.equ ACCERR,     0b00010000     ;flash access error

.equ FCMD,       FTS+0x06 ;flash command register
;
.equ CMDB6,      0b01000000     ;valid flash user mode commands
.equ CMDB5,      0b00100000     ;valid flash user mode commands
.equ CMDB2,      0b00000100     ;valid flash user mode commands
.equ CMDB0,      0b00000001     ;valid flash user mode commands

.equ MassErase,  CMDB6+CMDB0
.equ SecErase,   CMDB6
.equ ProgWord,   CMDB5

;****  EEPROM Control Register  ********************************************************
;*
.equ ECLKDIV,    EE+0x00  ;EEPROM Clock Divider Register

.equ ECNFG,      EE+0x03  ;EEPROM configuration register
;
.equ ESTAT,      EE+0x05  ;EEPROM status register
;
.equ ECMD,       EE+0x06  ;EEPROM command buffer and register

;****  Port Integration Module (PIM)  *******************************************************
;*
.equ PORTT,        PIM+0x00 ;port T I/O register
.equ PTT,        PIM+0x00 ;port T I/O register
.equ PTT6,       0b01000000
.equ PTT5,       0b00100000
.equ PTT4,       0b00010000
.equ PTT3,       0b00001000
.equ PTT2,       0b00000100
.equ PTT1,       0b00000010
.equ PTT0,       0b00000001

.equ PORTS,        PIM+0x08 ;port S I/O register
.equ PTS,        PIM+0x08 ;port S I/O register
.equ PTS6,       0b01000000
.equ PTS5,       0b00100000
.equ PTS4,       0b00010000
.equ PTS3,       0b00001000
.equ PTS2,       0b00000100
.equ PTS1,       0b00000010
.equ PTS0,       0b00000001

.equ PERS,       PIM+0x0C ;port S pull device enable register
.equ PERS0,      0b00000001

.equ PPSS,       PIM+0x0D ;port S polarity select register
.equ PPSS0,      0b00000001

.equ PORTM,        PIM+0x10 ;port M I/O register
.equ PTM,        PIM+0x10 ;port M I/O register
.equ PTM6,       0b01000000
.equ PTM5,       0b00100000
.equ PTM4,       0b00010000
.equ PTM3,       0b00001000
.equ PTM2,       0b00000100
.equ PTM1,       0b00000010
.equ PTM0,       0b00000001

.equ PERM,       PIM+0x13 ;port G pull device enable register
.equ PERM0,      0b00000001

.equ PPSM,       PIM+0x15 ;port G polarity select register
.equ PPSM0,      0b00000001

.equ PTP,        PIM+0x18 ;port P I/O register
;
.equ PTP7,       0b00100000
.equ PTP6,       0b00010000
.equ PTP5,       0b00100000
.equ PTP4,       0b00010000
.equ PTP3,       0b00001000
.equ PTP2,       0b00000100
.equ PTP1,       0b00000010
.equ PTP0,       0b00000001

.equ PTIP,       PIM+0x19 ;port P input register
;
.equ DDRP,       PIM+0x1A ;port P data direction register
;
.equ RDRP,       PIM+0x1B ;port P reduced drive register
;
.equ PERP,       PIM+0x1C ;port P pull device enable register
;
.equ PPSP,       PIM+0x1D ;port P polarity select register
;
.equ PTQ,        PIM+0x20 ;port Q I/O register
;
.equ PTQ7,       0b10000000
.equ PTQ6,       0b01000000
.equ PTQ5,       0b00100000
.equ PTQ4,       0b00010000
.equ PTQ3,       0b00001000
.equ PTQ2,       0b00000100
.equ PTQ1,       0b00000010
.equ PTQ0,       0b00000001

.equ PTIQ,       PIM+0x21 ;port Q input register
;
.equ DDRQ,       PIM+0x22 ;port Q data direction register
;
.equ RDRQ,       PIM+0x23 ;port Q reduced drive register
;
.equ PERQ,       PIM+0x24 ;port Q pull device enable register
;
.equ PPSQ,       PIM+0x25 ;port Q polarity select register
;
.equ PTL,        PIM+0x28 ;port S I/O register
.equ PTL6,       0b01000000
.equ PTL5,       0b00100000
.equ PTL4,       0b00010000
.equ PTL3,       0b00001000
.equ PTL2,       0b00000100
.equ PTL1,       0b00000010
.equ PTL0,       0b00000001

.equ PERL,       PIM+0x2C ;port S pull device enable register
.equ PERL0,      0b00000001

.equ PPSL,       PIM+0x2D ;port S polarity select register
.equ PPSL0,      0b00000001


; and the rest that we need
.equ CRGINT,     0x0038

/* Timer definitions */
.equ TIOS,   0x0040
.equ CFORC,  0x0041
.equ OC7M,   0x0042
.equ OC7D,   0x0043
.equ TCNT,   0x0044
.equ TSCR1,  0x0046
.equ TTOV,   0x0047
.equ TCTL1,  0x0048
.equ TCTL2,  0x0049
.equ TCTL3,  0x004A
.equ TCTL4,  0x004B
.equ TIE,    0x004C
.equ TSCR2,  0x004D
.equ TFLG1,  0x004E
.equ TFLG2,  0x004F
.equ TC0,    0x0050
.equ TC1,    0x0052
.equ TC2,    0x0054
.equ TC3,    0x0056
.equ TC4,    0x0058
.equ TC5,    0x005A
.equ TC6,    0x005C
.equ TC7,    0x005E
.equ PACTL,  0x0060
.equ PAFLG,  0x0061
.equ MCCTL,  0x0066
.equ MCFLG,  0x0067
.equ MCCNT,  0x0076

/* PWM definitions */
.equ PWME,   0x00E0
.equ PWMPOL, 0x00E1
.equ PWMCLK, 0x00E2
.equ PWMPRCLK, 0x00E3
.equ PWMCAE,  0x00E4
.equ PWMCNT0, 0x00EC
.equ PWMCNT1, 0x00ED
.equ PWMCNT2, 0x00EE
.equ PWMCNT3, 0x00EF
.equ PWMCNT4, 0x00F0
.equ PWMPER0, 0x00F2
.equ PWMPER1, 0x00F3
.equ PWMPER2, 0x00F4
.equ PWMPER3, 0x00F5
.equ PWMPER4, 0x00F6
.equ PWMDTY0, 0x00F8
.equ PWMDTY1, 0x00F9
.equ PWMDTY2, 0x00FA
.equ PWMDTY3, 0x00FB
.equ PWMDTY4, 0x00FC
.equ PWMSCLA, 0x00E8
.equ PWMSCLB, 0x00E9

 ; CAN section */
  .equ CAN_BASE, 0x0140

  .equ CANCTL0,		CAN_BASE+0x0
  .equ CANCTL1,		CAN_BASE+0x1
  .equ CANBTR0,		CAN_BASE+0x2
  .equ CANBTR1,		CAN_BASE+0x3
  .equ CANRFLG,		CAN_BASE+0x4
  .equ CANRIER,		CAN_BASE+0x5
  .equ CANTFLG,		CAN_BASE+0x6
  .equ CANTIER,		CAN_BASE+0x7
  .equ CANTARQ,		CAN_BASE+0x8
  .equ CANTAAK,		CAN_BASE+0x9
  .equ CANTBSEL,		CAN_BASE+0xA
  .equ CANIDAC,		CAN_BASE+0xB
  .equ CANRXERR,		CAN_BASE+0xE
  .equ CANTXERR,		CAN_BASE+0xF

  .equ CANIDAR0,		CAN_BASE+0x10
  .equ CANIDAR1,		CAN_BASE+0x11
  .equ CANIDAR2,		CAN_BASE+0x12
  .equ CANIDAR3,		CAN_BASE+0x13
  .equ CANIDAR4,		CAN_BASE+0x18
  .equ CANIDAR5,		CAN_BASE+0x19
  .equ CANIDAR6,		CAN_BASE+0x1A
  .equ CANIDAR7,		CAN_BASE+0x1B

  .equ CANIDMR0,		CAN_BASE+0x14
  .equ CANIDMR1,		CAN_BASE+0x15
  .equ CANIDMR2,		CAN_BASE+0x16
  .equ CANIDMR3,		CAN_BASE+0x17
  .equ CANIDMR4,		CAN_BASE+0x1C
  .equ CANIDMR5,		CAN_BASE+0x1D
  .equ CANIDMR6,		CAN_BASE+0x1E
  .equ CANIDMR7,		CAN_BASE+0x1F

  .equ CAN_RB_IDR0,	CAN_BASE+0x20
  .equ CAN_RB_IDR1,	CAN_BASE+0x21
  .equ CAN_RB_IDR2,	CAN_BASE+0x22
  .equ CAN_RB_IDR3,	CAN_BASE+0x23

  .equ CAN_RB_DSR0,	CAN_BASE+0x24
  .equ CAN_RB_DSR1,	CAN_BASE+0x25
  .equ CAN_RB_DSR2,	CAN_BASE+0x26
  .equ CAN_RB_DSR3,	CAN_BASE+0x27
  .equ CAN_RB_DSR4,	CAN_BASE+0x28
  .equ CAN_RB_DSR5,	CAN_BASE+0x29
  .equ CAN_RB_DSR6,	CAN_BASE+0x2A
  .equ CAN_RB_DSR7,	CAN_BASE+0x2B

  .equ CAN_RB_DLR,	    CAN_BASE+0x2C

  .equ CAN_RB_TBPR,	CAN_BASE+0x2D

  .equ CAN_TB0_IDR0,	CAN_BASE+0x30
  .equ CAN_TB0_IDR1,	CAN_BASE+0x31
  .equ CAN_TB0_IDR2,	CAN_BASE+0x32
  .equ CAN_TB0_IDR3,	CAN_BASE+0x33

  .equ CAN_TB0_DSR0,	CAN_BASE+0x34
  .equ CAN_TB0_DSR1,	CAN_BASE+0x35
  .equ CAN_TB0_DSR2,	CAN_BASE+0x36
  .equ CAN_TB0_DSR3,	CAN_BASE+0x37
  .equ CAN_TB0_DSR4,	CAN_BASE+0x38
  .equ CAN_TB0_DSR5,	CAN_BASE+0x39
  .equ CAN_TB0_DSR6,	CAN_BASE+0x3A
  .equ CAN_TB0_DSR7,	CAN_BASE+0x3B

  .equ CAN_TB0_DLR,	CAN_BASE+0x3C

  .equ CAN_TB0_TBPR,	CAN_BASE+0x3D
