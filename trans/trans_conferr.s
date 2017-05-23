;*********************************************************************
; config error   (C) 2006 James Murray
;*********************************************************************
; $Id: trans_conferr.s,v 1.4 2014/12/20 21:03:31 jsm Exp $

.sect .text3c
.globl configerror, conf_str

             nolist               ;turn off listing
             include "ms2extrah.inc"
             list                 ;turn listing back on

;kill just about everything and send config error down serial port,
; then sit in loop turning fuel pump on and off
;*********************************************************************
; on entry A contains the config error
;*********************************************************************
configerror:
            ldaa        conf_err
            cmpa        #200
            bhs         ce_nosei
            bset        outpc.status1, #status1_conferr;
            sei                        ; disable ints until we've sent the message
ce_nosei:
            movb        #0x00,SCI0CR1
            movb        #0x08,SCI0CR2  ; disable SCI ints, enable tx
			ldx			#20
			bsr			delaylp1		; short delay to ensure everything is ready
;send CR,NL as pre-amble to avoid missed first bytes
            ldx         #cr
            bsr         sendstr        ; uses X

            ldab        conf_err
            decb
            clra
            lslb
            rola
            tfr         d,x
            ldx         errtbl,x
            bsr         sendstr        ; uses X
            cli
            movb        #0x00,SCI0CR1
            movb        #0x24,SCI0CR2  ; set back to normal

            ldab        conf_err
            cmpb        #200
            blo         ce_rpm
            clr         conf_err
            rtc

ce_rpm:
;fake rpm with conf_err in it
            clra
;            addd        #65000   ; rpm = 65000 + conf_err
;            std         outpc.rpm

            leas        -1,sp
            movb        #3, 0,sp
celoop:
;            bset        PORTE,PTE4   ; pulse fuel pump
            bsr         delay       ; on for ~1s
;            bclr        PORTE,PTE4
            bsr         delay       ; off for ~3s
            bsr         delay
            bsr         delay
            dec         0,sp
            bne         celoop
            leas        +1,sp
            ldaa        conf_err
            cmpa        #190
            blo         dead_loop
            clr         conf_err
            bclr        outpc.status1, #status1_conferr
            rtc                     ; if a soft config error then we _can_ return
dead_loop:
            bsr         delay
            bra         dead_loop

;*********************************************************************
;sendstr:      ; send text string down serial port
; on entry X points to start of zero terminated string
;*********************************************************************
sendstr:
            ldaa        SCI0SR1    ; ensure stat reg read as part of sequence
sendstrlp:
            ldaa        1,x+
            beq         ssdone
            staa        SCI0DRL
sswait:
            brset       SCI0SR1,#0x80,sendstrlp ; wait for byte to be sent
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            bra         sswait
ssdone:
            rts ; ok because called with bsr

;*********************************************************************
;delay:      ; busy wait
;*********************************************************************
delay:
            ldx         #0
delaylp1:
            ldy         #16
delaylp2:
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            nop
            nop
            nop
            nop
            dbne        y,delaylp2
            pshx
            pshy
            call        serial       ; be prepared to send realtime data
            puly
            pulx
            dbne        x,delaylp1
            rts ; ok because called with bsr
;*********************************************************************
;conf_str      ; copy config error string to serial buffer
; returns D as data size
;*********************************************************************
conf_str:
            ldy     #0

            ldab        conf_err
            decb
            clra
            lslb
            rola
            tfr         d,x
            ldx         errtbl,x

confstrlp:
            ldaa        1,x+
            beq         confstrdone
            staa        SCI0DRL

            staa        srlbuf+3,y       ; record to conf error buffer
            iny
            cmpy        #SRLDATASIZE     ; prevent overflow
            blo         confstrlp           

confstrdone:
            tfr     y,d
            rtc
;*********************************************************************
errtbl:
            fdb         err1

cr:         fcb         0x0d,0x0a,0

err1:
            fcc         "Unknown configuration error"
            fcb         0x0d,0x0a,0


.nolist                      ;skip the symbol table
