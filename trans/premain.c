/* $Id: premain.c,v 1.1 2013/01/16 23:48:29 jsm Exp $ */
void __premain() 
{ 
	(*((volatile unsigned char*)(0x0030))) = 0x3d; //PPAGE set to 0x3C
} 

