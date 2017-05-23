/* $Id: trans_main_vectors.h,v 1.2 2014/09/11 14:48:06 jsm Exp $ */
const tIsrFunc _vect[] VECT_ATTR = {      /* Interrupt table */
    UnimplementedISR,                 /* vector 63 */
    UnimplementedISR,                 /* vector 62 */
    UnimplementedISR,                 /* vector 61 */
    UnimplementedISR,                 /* vector 60 */
    UnimplementedISR,                 /* vector 59 */
    UnimplementedISR,                 /* vector 58 */
    UnimplementedISR,                 /* vector 57 */
    UnimplementedISR,                 /* vector 56 */
    UnimplementedISR,                 /* vector 55 */
    UnimplementedISR,                 /* vector 54 */
    UnimplementedISR,                 /* vector 53 */
    UnimplementedISR,                 /* vector 52 */
    UnimplementedISR,                 /* vector 51 */
    UnimplementedISR,                 /* vector 50 */
    UnimplementedISR,                 /* vector 49 */
    UnimplementedISR,                 /* vector 48 */
    UnimplementedISR,                 /* vector 47 */
    UnimplementedISR,                 /* vector 46 */
    UnimplementedISR,                 /* vector 45 */
    UnimplementedISR,                 /* vector 44 */
    UnimplementedISR,                 /* vector 43 */
    UnimplementedISR,                 /* vector 42 */
    UnimplementedISR,                 /* vector 41 */
    UnimplementedISR,                 /* vector 40 */
    CanTxIsr,                         /* vector 39 */
    CanRxIsr,                         /* vector 38 */
    CanRxIsr,                         /* vector 37 */
    UnimplementedISR,                 /* vector 36 */
    UnimplementedISR,                 /* vector 35 */
    UnimplementedISR,                 /* vector 34 */
    UnimplementedISR,                 /* vector 33 */
    UnimplementedISR,                 /* vector 32 */
    UnimplementedISR,                 /* vector 31 */
    UnimplementedISR,                 /* vector 30 */
    UnimplementedISR,                 /* vector 29 */
    UnimplementedISR,                 /* vector 28 */
    UnimplementedISR,                 /* vector 27 */
    UnimplementedISR,                 /* vector 26 */
    UnimplementedISR,                 /* vector 25 */
    UnimplementedISR,                 /* vector 24 */
    UnimplementedISR,                 /* vector 23 */
    UnimplementedISR,                 /* vector 22 */
    UnimplementedISR,                 /* vector 21 */
    ISR_SCI_Comm,                     /* vector 20 */
    UnimplementedISR,                 /* vector 19 */
    UnimplementedISR,                 /* vector 18 */
    UnimplementedISR,                 /* vector 17 */
    ISR_TimerOverflow,                /* vector 16 */
    UnimplementedISR,                 /* vector 15 timer 7*/
    UnimplementedISR,                 /* vector 14 timer 6*/
    ISR_tach5,                        /* vector 13 timer 5*/
    UnimplementedISR,                 /* vector 12 timer 4*/
    UnimplementedISR,                 /* vector 11 timer 3*/
    ISR_tach2,                        /* vector 10 timer 2*/
    UnimplementedISR,                 /* vector 09 timer 1*/
    ISR_vss,                          /* vector 08 timer 0*/
    ISR_Timer_Clock,                  /* vector 07 */
    UnimplementedISR,                 /* vector 06 */
    UnimplementedISR,                 /* vector 05 */
    UnimplementedISR,                 /* vector 04 */
    UnimplementedISR,                 /* vector 03 */
    UnimplementedISR,                 /* vector 02 */
    UnimplementedISR,                 /* vector 01 */
    _start                            /* Reset vector */
};

